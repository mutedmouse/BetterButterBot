"""End-of-utterance detection for the robot's raw mic stream.

The robot streams un-processed 16 kHz mono PCM and relies on the gateway to say when the user has
finished talking. Two detectors:

  silero (default)  Silero VAD v6 (the ONNX model bundled with faster-whisper), run frame by frame
                    on 32 ms frames with its recurrent state carried between frames. It answers
                    "is this speech?", so fans, music, clatter and hum don't hold the window open.
                    ~1 ms of CPU per frame; adds no latency.
  energy            the original loudness-over-noise-floor detector (fallback / no onnxruntime).

Room levels (noise floor, peak) are tracked in both modes for logging.
"""
import logging
import math

import numpy as np

log = logging.getLogger("endpoint")

SAMPLE_RATE = 16000


class SileroStream:
    """Per-utterance streaming wrapper around a shared Silero ONNX session."""

    FRAME = 512      # samples per inference at 16 kHz (32 ms)
    CONTEXT = 64     # samples of the previous frame the model expects in front of each frame

    def __init__(self, session):
        self.session = session
        self.h = np.zeros((1, 1, 128), dtype=np.float32)
        self.c = np.zeros((1, 1, 128), dtype=np.float32)
        self.context = np.zeros(self.CONTEXT, dtype=np.float32)

    def prob(self, frame: np.ndarray) -> float:
        x = np.concatenate([self.context, frame]).astype(np.float32)[None, :]
        out, self.h, self.c = self.session.run(None, {"input": x, "h": self.h, "c": self.c})
        self.context = frame[-self.CONTEXT:]
        return float(np.ravel(out)[0])


def load_silero():
    """Returns the shared ONNX session, or None if the model / onnxruntime isn't available."""
    try:
        from faster_whisper.vad import get_vad_model
        return get_vad_model().session
    except Exception as e:  # pragma: no cover - depends on install
        log.warning("Silero VAD unavailable (%s); using the energy endpointer", e)
        return None


class Endpointer:
    def __init__(self, silero_session=None, start_threshold=0.3, end_threshold=0.3,
                 calib_ms=200, onset_ms=None, margin_db=10.0, min_db=-58.0,
                 end_silence_ms=None, min_speech_ms=None, no_speech_ms=6000, max_ms=8500,
                 short_speech_ms=700, short_end_silence_ms=850):
        """Defaults were tuned on real ButterBot mic recordings (tune_endpoint.py): with Silero,
        0.3/0.3 thresholds, 64 ms onset and 128 ms minimum speech catch a short quiet "butterbot"
        (peak p~0.6) and reject empty windows and synthetic noise. Closing silence is 500 ms after
        real sentences (~0.13 s sooner on average than the energy detector's 750 ms; 400 ms cut
        "Hey Butterbot, | pass the butter" in two) and 850 ms while under 700 ms of speech."""
        self.vad = SileroStream(silero_session) if silero_session is not None else None
        if onset_ms is None:
            onset_ms = 64 if self.vad else 90
        if end_silence_ms is None:
            end_silence_ms = 500 if self.vad else 750
        # People pause after addressing the robot ("Hey Butterbot, ... pass the butter"): while
        # only a short burst has been heard, wait longer before deciding they've finished.
        self.short_speech_ms = short_speech_ms if self.vad else 0
        self.short_end_silence_ms = short_end_silence_ms
        if min_speech_ms is None:
            min_speech_ms = 128 if self.vad else 240
        self.frame_samples = SileroStream.FRAME if self.vad else 480
        self.frame_ms = self.frame_samples * 1000 // SAMPLE_RATE
        self.frame_bytes = self.frame_samples * 2
        self.start_threshold = start_threshold
        self.end_threshold = end_threshold
        self.calib_frames = max(1, calib_ms // self.frame_ms)
        self.onset_frames = max(1, math.ceil(onset_ms / self.frame_ms))
        self.margin_db = margin_db
        self.min_db = min_db
        self.end_silence_ms = end_silence_ms
        self.min_speech_ms = min_speech_ms
        self.no_speech_ms = no_speech_ms
        self.max_ms = max_ms

        self.pending = bytearray()
        self.frames = 0
        self.noise_db = None
        self.calib = []
        self.in_speech = False
        self.run = 0
        self.speech_ms = 0
        self.silence_ms = 0
        self.peak_db = -120.0
        self.peak_prob = 0.0

    @property
    def mode(self) -> str:
        return "silero" if self.vad else "energy"

    @staticmethod
    def level_db(frame: np.ndarray) -> float:
        x = frame.astype(np.float32)
        x -= x.mean()  # PDM mics often carry a DC offset
        rms = math.sqrt(float(np.mean(x * x))) / 32768.0
        return 20 * math.log10(rms + 1e-9)

    def _is_speech(self, frame: np.ndarray, db: float) -> bool:
        if self.vad:
            p = self.vad.prob(frame.astype(np.float32) / 32768.0)
            self.peak_prob = max(self.peak_prob, p)
            # Hysteresis: harder to start speech than to stay in it.
            return p >= (self.end_threshold if self.in_speech else self.start_threshold)
        threshold = max(self.noise_db + self.margin_db, self.min_db)
        return db > threshold

    def feed(self, data: bytes) -> str | None:
        """Returns 'end', 'nospeech' or 'maxlen' once the utterance is over, else None."""
        self.pending.extend(data)
        while len(self.pending) >= self.frame_bytes:
            frame = np.frombuffer(bytes(self.pending[:self.frame_bytes]), dtype=np.int16)
            del self.pending[:self.frame_bytes]
            self.frames += 1
            elapsed = self.frames * self.frame_ms
            db = self.level_db(frame)
            self.peak_db = max(self.peak_db, db)

            if self.frames <= self.calib_frames:
                self.calib.append(db)
                if self.frames == self.calib_frames:
                    self.noise_db = float(np.median(self.calib))
                if self.vad:
                    self.vad.prob(frame.astype(np.float32) / 32768.0)  # warm the recurrent state
                continue

            speechy = self._is_speech(frame, db)

            if not self.in_speech:
                self.run = self.run + 1 if speechy else 0
                if self.run >= self.onset_frames:
                    self.in_speech = True
                    self.speech_ms = self.run * self.frame_ms
                    self.silence_ms = 0
                elif not speechy:
                    self.noise_db = 0.95 * self.noise_db + 0.05 * db
                if not self.in_speech and elapsed >= self.no_speech_ms:
                    return "nospeech"
            else:
                if speechy:
                    self.speech_ms += self.frame_ms
                    self.silence_ms = 0
                else:
                    self.silence_ms += self.frame_ms
                    needed = (self.short_end_silence_ms if self.speech_ms < self.short_speech_ms
                              else self.end_silence_ms)
                    if self.silence_ms >= needed:
                        if self.speech_ms >= self.min_speech_ms:
                            return "end"
                        self.in_speech = False  # a click or bump, not speech
                        self.run = 0

            if elapsed >= self.max_ms:
                return "maxlen"
        return None

    def summary(self) -> str:
        noise = f"{self.noise_db:.0f}" if self.noise_db is not None else "?"
        vad = f", vad peak {self.peak_prob:.2f}" if self.vad else ""
        return f"{self.mode}: noise {noise} dBFS, peak {self.peak_db:.0f} dBFS, speech {self.speech_ms} ms{vad}"


def _mulaw_table() -> np.ndarray:
    u = ~np.arange(256, dtype=np.int32) & 0xFF
    sign = u & 0x80
    exponent = (u >> 4) & 0x07
    mantissa = u & 0x0F
    sample = (((mantissa << 3) + 0x84) << exponent) - 0x84
    return np.where(sign != 0, -sample, sample).astype(np.int16)


MULAW = _mulaw_table()


def decode(data: bytes, codec: str) -> bytes:
    """Robot audio -> s16le. 'mulaw' is G.711 (8-bit), anything else is already s16le."""
    if codec == "mulaw":
        return MULAW[np.frombuffer(data, dtype=np.uint8)].tobytes()
    return data


def normalize(audio: np.ndarray, target_peak=0.9, max_gain=30.0) -> np.ndarray:
    """Remove DC and bring quiet raw-mic audio up to a level Whisper is comfortable with."""
    audio = audio - float(np.mean(audio)) if audio.size else audio
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak <= 0:
        return audio
    gain = min(target_peak / peak, max_gain)
    return audio * gain if gain > 1.0 else audio
