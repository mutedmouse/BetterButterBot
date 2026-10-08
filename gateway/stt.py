"""Speech-to-text backends. Pick one in config.yaml (stt.provider).

  faster-whisper  local Whisper (CPU or NVIDIA CUDA) via CTranslate2 - the default; any OS
  mlx             local Whisper on the Apple Silicon GPU (native macOS install)
  openai          any OpenAI-compatible POST /audio/transcriptions: OpenAI (whisper-1,
                  gpt-4o-mini-transcribe), Groq (whisper-large-v3-turbo, very fast), Speaches /
                  faster-whisper-server, LocalAI, vLLM
  whispercpp      a whisper.cpp `server` (POST /inference)
  deepgram        Deepgram's hosted API (nova-3)

All take 16 kHz mono float32 audio and return plain text.
"""
import asyncio
import io
import logging
import os
import time
import wave

import httpx
import numpy as np

log = logging.getLogger("stt")
SAMPLE_RATE = 16000


def wav_bytes(audio: np.ndarray) -> bytes:
    pcm = (np.clip(audio, -1.0, 1.0) * 32767).astype(np.int16).tobytes()
    buf = io.BytesIO()
    with wave.open(buf, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(pcm)
    return buf.getvalue()


def secret(cfg: dict, default_env: str) -> str:
    """API keys are never stored in config.yaml - only the name of the env var that holds them."""
    return os.environ.get(cfg.get("api_key_env", default_env), "")


class STT:
    name = "stt"

    async def warmup(self):
        pass

    async def transcribe(self, audio: np.ndarray) -> str:
        raise NotImplementedError

    def describe(self) -> str:
        return self.name


def cpu_has_avx2() -> bool:
    try:
        with open("/proc/cpuinfo") as f:
            return " avx2" in f.read()
    except OSError:  # Windows / macOS: assume a modern CPU
        return True


def cuda_devices() -> int:
    try:
        import ctranslate2
        return ctranslate2.get_cuda_device_count()
    except Exception:
        return 0


def resolve_hardware(cfg: dict) -> tuple[str, str, str, int]:
    """Fill in `auto` device / compute_type / model / threads from what this machine has.

    GPU (CUDA)          -> large-v3-turbo, int8_float16   (~0.2-0.4 s per command on an RTX 4060)
    CPU with AVX2       -> small.en, int8                 (~1-2 s on a recent desktop CPU)
    CPU without AVX2    -> base.en, int8                  (~1.4 s on a 2009 Xeon)
    """
    device = cfg.get("device", "auto")
    if device == "auto":
        device = "cuda" if cuda_devices() > 0 else "cpu"
    compute = cfg.get("compute_type", "auto")
    if compute == "auto":
        compute = "int8_float16" if device == "cuda" else "int8"
    model = cfg.get("model", "auto")
    if model == "auto":
        model = "large-v3-turbo" if device == "cuda" else ("small.en" if cpu_has_avx2() else "base.en")
    threads = cfg.get("threads", "auto")
    if threads == "auto":
        threads = max(1, min(8, os.cpu_count() or 4))
    return device, compute, model, int(threads)


class FasterWhisperSTT(STT):
    def __init__(self, cfg: dict, hotwords: str):
        from faster_whisper import WhisperModel  # heavy import only when used

        self.cfg = cfg
        self.hotwords = hotwords if cfg.get("hotwords", True) else None
        device, compute, self.model_name, threads = resolve_hardware(cfg)
        t = time.perf_counter()
        self.model = WhisperModel(self.model_name, device=device, compute_type=compute,
                                  cpu_threads=threads, download_root=cfg.get("download_root"))
        log.info("loaded %s on %s/%s in %.1fs", self.model_name, device, compute, time.perf_counter() - t)
        self.name = f"faster-whisper {self.model_name} ({device}/{compute})"

    def _run(self, audio: np.ndarray, vad: bool = True) -> str:
        segments, _ = self.model.transcribe(
            audio, language=self.cfg.get("language", "en"), beam_size=int(self.cfg.get("beam", 1)),
            vad_filter=vad, hotwords=self.hotwords, condition_on_previous_text=False,
            without_timestamps=True)
        return " ".join(s.text.strip() for s in segments if s.no_speech_prob < 0.6).strip()

    async def warmup(self):
        # Real-looking audio and no VAD so the encoder actually runs (a VAD-filtered silent clip
        # skips the model and leaves the first real request paying the init cost).
        noise = np.random.default_rng(0).normal(0, 0.05, 2 * SAMPLE_RATE).astype(np.float32)
        await asyncio.to_thread(self._run, noise, False)
        await asyncio.to_thread(self._run, noise)

    async def transcribe(self, audio: np.ndarray) -> str:
        return await asyncio.to_thread(self._run, audio)


class MLXWhisperSTT(STT):
    """Whisper on the Apple Silicon GPU via MLX (native macOS install only - Docker on a Mac has
    no GPU access). pip install mlx-whisper."""

    def __init__(self, cfg: dict, hotwords: str):
        import mlx_whisper  # only importable on Apple Silicon macOS

        self.mlx = mlx_whisper
        self.repo = cfg.get("model", "mlx-community/whisper-large-v3-turbo")
        self.prompt = hotwords if cfg.get("hotwords", True) else None
        self.language = cfg.get("language", "en")
        self.name = f"mlx-whisper {self.repo} (Apple GPU)"

    def _run(self, audio: np.ndarray) -> str:
        out = self.mlx.transcribe(audio, path_or_hf_repo=self.repo, language=self.language,
                                  initial_prompt=self.prompt, condition_on_previous_text=False)
        return (out.get("text") or "").strip()

    async def warmup(self):
        await asyncio.to_thread(self._run, np.zeros(SAMPLE_RATE, dtype=np.float32))

    async def transcribe(self, audio: np.ndarray) -> str:
        return await asyncio.to_thread(self._run, audio)


class OpenAITranscriptionSTT(STT):
    def __init__(self, cfg: dict, hotwords: str):
        self.base_url = cfg.get("base_url", "https://api.openai.com/v1").rstrip("/")
        self.model = cfg.get("model", "whisper-1")
        self.key = secret(cfg, "BB_STT_API_KEY")
        self.language = cfg.get("language", "en")
        self.prompt = hotwords if cfg.get("hotwords", True) else ""
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(float(cfg.get("timeout", 15)), connect=3.0))
        self.name = f"openai-compatible {self.model} @ {self.base_url}"

    async def transcribe(self, audio: np.ndarray) -> str:
        data = {"model": self.model, "language": self.language, "response_format": "json"}
        if self.prompt:
            data["prompt"] = self.prompt
        headers = {"Authorization": f"Bearer {self.key}"} if self.key else {}
        r = await self.client.post(f"{self.base_url}/audio/transcriptions", headers=headers, data=data,
                                   files={"file": ("audio.wav", wav_bytes(audio), "audio/wav")})
        if r.status_code >= 400:
            raise RuntimeError(f"STT HTTP {r.status_code}: {r.text[:200]}")
        return (r.json().get("text") or "").strip()


class WhisperCppSTT(STT):
    def __init__(self, cfg: dict, hotwords: str):
        self.url = cfg.get("url", "http://127.0.0.1:8080").rstrip("/")
        self.prompt = hotwords if cfg.get("hotwords", True) else ""
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(float(cfg.get("timeout", 15)), connect=3.0))
        self.name = f"whisper.cpp @ {self.url}"

    async def transcribe(self, audio: np.ndarray) -> str:
        data = {"response_format": "json", "temperature": "0.0", "language": "en"}
        if self.prompt:
            data["prompt"] = self.prompt
        r = await self.client.post(f"{self.url}/inference", data=data,
                                   files={"file": ("audio.wav", wav_bytes(audio), "audio/wav")})
        if r.status_code >= 400:
            raise RuntimeError(f"STT HTTP {r.status_code}: {r.text[:200]}")
        return (r.json().get("text") or "").strip()


class DeepgramSTT(STT):
    def __init__(self, cfg: dict, hotwords: str):
        self.model = cfg.get("model", "nova-3")
        self.key = secret(cfg, "DEEPGRAM_API_KEY")
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(float(cfg.get("timeout", 15)), connect=3.0))
        self.name = f"deepgram {self.model}"

    async def transcribe(self, audio: np.ndarray) -> str:
        r = await self.client.post(
            "https://api.deepgram.com/v1/listen",
            params={"model": self.model, "language": "en", "smart_format": "true"},
            headers={"Authorization": f"Token {self.key}", "Content-Type": "audio/wav"},
            content=wav_bytes(audio))
        if r.status_code >= 400:
            raise RuntimeError(f"STT HTTP {r.status_code}: {r.text[:200]}")
        alts = r.json()["results"]["channels"][0]["alternatives"]
        return (alts[0].get("transcript") or "").strip() if alts else ""


PROVIDERS = {
    "faster-whisper": FasterWhisperSTT,
    "mlx": MLXWhisperSTT,
    "openai": OpenAITranscriptionSTT,
    "whispercpp": WhisperCppSTT,
    "deepgram": DeepgramSTT,
}


def build(cfg: dict, hotwords: str) -> STT:
    provider = cfg.get("provider", "faster-whisper")
    if provider not in PROVIDERS:
        raise SystemExit(f"unknown stt.provider '{provider}'; choose one of {', '.join(PROVIDERS)}")
    return PROVIDERS[provider](cfg.get(provider, {}) or {}, hotwords)
