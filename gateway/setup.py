#!/usr/bin/env python3
"""ButterBot voice gateway setup wizard.

Asks how you want speech recognised and questions answered, detects your hardware, and writes
config.yaml (choices) and secret.env (API keys, private). Standard library only, so it runs
before anything is installed.

  python setup.py            interactive
  python setup.py --yes      no questions: local Whisper on the best hardware found, commands only
"""
import argparse
import getpass
import os
import platform
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent

IS_MAC = platform.system() == "Darwin"
IS_WINDOWS = os.name == "nt"
APPLE_SILICON = IS_MAC and platform.machine() == "arm64"

STT_CHOICES = [
    ("local", "Local Whisper on this machine (private, free; uses an NVIDIA GPU if there is one)"),
    ("groq", "Groq hosted Whisper (very fast, free tier, needs a key)"),
    ("openai", "OpenAI transcription API (needs a key)"),
    ("deepgram", "Deepgram (needs a key)"),
    ("server", "A transcription server I already run (whisper.cpp, Speaches, LocalAI, vLLM)"),
]

# (key, label, preset, default model, key env var or "" for none)
LLM_CHOICES = [
    ("gemini", "Google Gemini (fast, generous free tier)", "gemini", "gemini-flash-lite-latest", "GEMINI_API_KEY"),
    ("anthropic", "Anthropic Claude", "anthropic", "claude-opus-5-5", "ANTHROPIC_API_KEY"),
    ("openrouter", "OpenRouter (hundreds of hosted models)", "openrouter", "", "OPENROUTER_API_KEY"),
    ("openai", "OpenAI", "openai", "", "OPENAI_API_KEY"),
    ("ollama", "Ollama on my network (local, free)", "ollama", "llama3.2:3b", ""),
    ("lmstudio", "LM Studio on my network (local, free)", "lmstudio", "", ""),
    ("vllm", "vLLM server", "vllm", "", ""),
    ("none", "No LLM - built-in commands only", "", "", ""),
]


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    answer = input(f"{prompt}{suffix}: ").strip()
    return answer or default


def choose(title: str, options: list, default: int = 1) -> int:
    print(f"\n{title}")
    for i, opt in enumerate(options, 1):
        print(f"  {i}) {opt[1]}")
    while True:
        raw = ask("Choose", str(default))
        if raw.isdigit() and 1 <= int(raw) <= len(options):
            return int(raw) - 1
        print("  Please enter a number from the list.")


def detect_gpu() -> str:
    if not shutil.which("nvidia-smi"):
        return ""
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=name,memory.total", "--format=csv,noheader"],
                             capture_output=True, text=True, timeout=10).stdout.strip()
        return out.splitlines()[0] if out else ""
    except Exception:
        return ""


def cpu_summary() -> str:
    flags = ""
    try:
        flags = Path("/proc/cpuinfo").read_text()
    except OSError:
        pass
    avx2 = " avx2" in flags if flags else None
    name = platform.processor() or platform.machine()
    note = {True: "AVX2", False: "no AVX2 (older CPU: base.en model)", None: ""}[avx2]
    return f"{name} {note}".strip()


WINDOWS_TZ = {  # tzutil /g name -> IANA, for the common cases
    "Eastern Standard Time": "America/New_York", "Central Standard Time": "America/Chicago",
    "Mountain Standard Time": "America/Denver", "US Mountain Standard Time": "America/Phoenix",
    "Pacific Standard Time": "America/Los_Angeles", "Alaskan Standard Time": "America/Anchorage",
    "Hawaiian Standard Time": "Pacific/Honolulu", "Atlantic Standard Time": "America/Halifax",
    "GMT Standard Time": "Europe/London", "W. Europe Standard Time": "Europe/Berlin",
    "Romance Standard Time": "Europe/Paris", "Central Europe Standard Time": "Europe/Budapest",
    "Central European Standard Time": "Europe/Warsaw", "E. Europe Standard Time": "Europe/Bucharest",
    "FLE Standard Time": "Europe/Kyiv", "AUS Eastern Standard Time": "Australia/Sydney",
    "Tokyo Standard Time": "Asia/Tokyo", "India Standard Time": "Asia/Kolkata",
    "China Standard Time": "Asia/Shanghai", "UTC": "UTC",
}


def detect_timezone() -> str:
    if os.name == "nt":
        try:
            name = subprocess.run(["tzutil", "/g"], capture_output=True, text=True, timeout=5).stdout.strip()
            return WINDOWS_TZ.get(name, "UTC")
        except Exception:
            return "UTC"
    for cmd in (["timedatectl", "show", "-p", "Timezone", "--value"],):
        try:
            tz = subprocess.run(cmd, capture_output=True, text=True, timeout=5).stdout.strip()
            if "/" in tz:
                return tz
        except Exception:
            pass
    try:
        link = os.readlink("/etc/localtime")
        if "zoneinfo/" in link:
            return link.split("zoneinfo/", 1)[1]
    except OSError:
        pass
    return "UTC"


def write_secret(path: Path, values: dict):
    existing = {}
    if path.exists():
        for line in path.read_text().splitlines():
            if "=" in line and not line.lstrip().startswith("#"):
                k, v = line.split("=", 1)
                existing[k.strip()] = v
    existing.update({k: v for k, v in values.items() if v})
    path.write_text("".join(f"{k}={v}\n" for k, v in existing.items()))
    if os.name == "posix":
        os.chmod(path, 0o600)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--yes", action="store_true", help="accept defaults without asking")
    ap.add_argument("--dir", default=str(HERE), help="where to write config.yaml / secret.env")
    args = ap.parse_args()
    out_dir = Path(args.dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    interactive = not args.yes

    gpu = detect_gpu()
    print("ButterBot voice gateway setup")
    print(f"  GPU: {gpu or 'none found (NVIDIA)'}")
    print(f"  CPU: {cpu_summary()}")

    secrets = {}

    # ---- speech to text
    stt_options = list(STT_CHOICES)
    if APPLE_SILICON:
        stt_options.insert(0, ("mlx", "Local Whisper on the Apple Silicon GPU (fastest on a Mac; native install, not Docker)"))
    stt_key = choose("How should ButterBot hear you?", stt_options) if interactive else 0
    stt_kind = stt_options[stt_key][0]
    if stt_kind == "mlx":
        stt_yaml = "  provider: mlx\n  mlx:\n    model: mlx-community/whisper-large-v3-turbo\n"
    elif stt_kind == "local":
        stt_yaml = ("  provider: faster-whisper\n  faster-whisper:\n"
                    "    model: auto          # GPU: large-v3-turbo | CPU: small.en, or base.en without AVX2\n"
                    "    device: auto         # auto | cuda | cpu\n"
                    "    compute_type: auto\n    threads: auto\n    beam: 1\n")
    elif stt_kind in ("groq", "openai"):
        url, model, env = (("https://api.groq.com/openai/v1", "whisper-large-v3-turbo", "GROQ_API_KEY")
                           if stt_kind == "groq" else
                           ("https://api.openai.com/v1", "gpt-4o-mini-transcribe", "OPENAI_API_KEY"))
        model = ask("Transcription model", model) if interactive else model
        secrets[env] = getpass.getpass(f"{env} (input hidden, Enter to set later): ").strip() if interactive else ""
        stt_yaml = f"  provider: openai\n  openai:\n    base_url: {url}\n    model: {model}\n    api_key_env: {env}\n"
    elif stt_kind == "deepgram":
        secrets["DEEPGRAM_API_KEY"] = getpass.getpass("DEEPGRAM_API_KEY (hidden): ").strip()
        stt_yaml = "  provider: deepgram\n  deepgram:\n    model: nova-3\n    api_key_env: DEEPGRAM_API_KEY\n"
    else:
        kind = ask("Server type: whispercpp or openai-compatible", "openai-compatible")
        url = ask("Server URL (e.g. http://192.168.1.50:8000/v1 or http://192.168.1.50:8080)")
        if kind.startswith("whisper"):
            stt_yaml = f"  provider: whispercpp\n  whispercpp:\n    url: {url}\n"
        else:
            model = ask("Model name on that server", "Systran/faster-whisper-small.en")
            stt_yaml = f"  provider: openai\n  openai:\n    base_url: {url}\n    model: {model}\n"

    # ---- LLM
    default_llm = len(LLM_CHOICES)  # "none" for --yes
    llm_key = choose("What should answer questions that aren't built-in commands?", LLM_CHOICES, 1) \
        if interactive else default_llm - 1
    kind, label, preset, model, env = LLM_CHOICES[llm_key]
    if kind == "none":
        llm_yaml = "  provider: none\n"
    else:
        if interactive:
            hint = " (e.g. claude-sonnet-5-5 or claude-haiku-4-5 for quicker replies)" if kind == "anthropic" else ""
            model = ask(f"Model{hint}", model)
            while not model:
                model = ask("Model name (required for this provider)")
        base_url = ""
        if kind in ("ollama", "lmstudio", "vllm"):
            default_url = {"ollama": "http://127.0.0.1:11434/v1", "lmstudio": "http://127.0.0.1:1234/v1",
                           "vllm": "http://127.0.0.1:8000/v1"}[kind]
            print("  Note: inside Docker, 127.0.0.1 is the container itself - use the server's LAN IP.")
            base_url = ask("Server URL", default_url) if interactive else default_url
        if env and interactive:
            secrets[env] = getpass.getpass(f"{env} (input hidden, Enter to set later): ").strip()
        if kind == "anthropic":
            llm_yaml = (f"  provider: anthropic\n  anthropic:\n    model: {model}\n    effort: low\n"
                        f"    fallbacks: true\n    api_key_env: {env}\n")
        else:
            llm_yaml = f"  provider: openai\n  openai:\n    preset: {preset}\n    model: {model}\n"
            if base_url:
                llm_yaml += f"    base_url: {base_url}\n"
            if env:
                llm_yaml += f"    api_key_env: {env}\n"

    tz = detect_timezone()
    if interactive:
        tz = ask("Timezone (IANA name, for alarms, reminders and the calendar)", tz)

    # ---- calendar (optional, read-only)
    if interactive and kind != "none":
        print("\nCalendar (optional): ButterBot can summarise your day from an iCal link.")
        print("  Google Calendar: Settings > your calendar > Integrate calendar >")
        print("  'Secret address in iCal format'. Several calendars: separate links with spaces.")
        secrets["BB_CALENDAR_ICS"] = getpass.getpass("Calendar link(s) (input hidden, Enter to skip): ").strip()

    cfg = (f"# Written by setup.py - all options: config.example.yaml\ntimezone: {tz}\n\n"
           f"stt:\n{stt_yaml}\nllm:\n{llm_yaml}")
    (out_dir / "config.yaml").write_text(cfg)
    write_secret(out_dir / "secret.env", secrets)
    (out_dir / "data").mkdir(exist_ok=True)

    profile = "gpu" if gpu and stt_kind == "local" and not IS_MAC else "cpu"
    if os.environ.get("BB_DOCKER_BUNDLE"):      # running inside the docker/ deploy bundle's setup service
        print("\nWrote config.yaml, secret.env and data/ in this folder.")
        print("\nStart the gateway:\n  docker compose up -d      # NVIDIA GPU: first set COMPOSE_PROFILES=gpu in .env")
        print("\nThen point ButterBot at ws://<this machine's IP>:8765/bb and allow TCP 8765 through the firewall.")
        return
    if os.environ.get("BB_INSTALLER"):          # called by install.sh / install.ps1, which start it next
        print(f"\nWrote {out_dir / 'config.yaml'} and {out_dir / 'secret.env'}.")
        return
    if stt_kind == "local" and APPLE_SILICON:
        print("\nTip: on Apple Silicon the 'Apple GPU' option (rerun setup) is much faster than Docker's CPU Whisper.")
    print(f"\nWrote {out_dir / 'config.yaml'} and {out_dir / 'secret.env'}.")
    print("\nStart the gateway with ONE of:")
    native = (r"powershell -ExecutionPolicy Bypass -File .\install.ps1   # Windows native (startup task)"
              if IS_WINDOWS else
              "./install.sh                                         # macOS native (login service)"
              if IS_MAC else
              "sudo ./install.sh                                    # Linux native (systemd)")
    if stt_kind == "mlx":
        print(f"  {native}")
        print("  (The Apple GPU option needs the native install - Docker on a Mac can't reach the GPU.)")
    else:
        print(f"  docker compose --profile {profile} up -d --build   # Docker (Windows, Linux, macOS)")
        print(f"  {native}")
    print("\nThen point ButterBot at ws://<this machine's IP>:8765/bb and allow TCP 8765 through the firewall.")
    if IS_WINDOWS:
        print("  Windows firewall (admin PowerShell): New-NetFirewallRule -DisplayName \"ButterBot gateway\" "
              "-Direction Inbound -Protocol TCP -LocalPort 8765 -Action Allow -Profile Private")


if __name__ == "__main__":
    main()
