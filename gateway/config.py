"""Loads config.yaml (see config.example.yaml) and builds the STT and LLM backends.

Without a config.yaml the gateway keeps its original behaviour: faster-whisper from the command
line flags, and an OpenAI-compatible LLM from BB_LLM_BASE_URL / BB_LLM_MODEL / BB_LLM_API_KEY.
Secrets never go in config.yaml - only the names of the environment variables that hold them.
"""
import logging
import os
from pathlib import Path

import yaml

import stt as stt_mod
from llm import Chat, OpenAICompatChat

log = logging.getLogger("config")
HERE = Path(__file__).resolve().parent


def load(path: str | None, args) -> dict:
    p = Path(path) if path else HERE / "config.yaml"
    if p.exists():
        cfg = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
        log.info("config: %s", p)
        return cfg

    # Legacy: flags + environment.
    log.info("config: no config.yaml, using command line and BB_LLM_* environment")
    return {
        "stt": {"provider": "faster-whisper", "faster-whisper": {
            "model": args.model, "device": args.device, "compute_type": args.compute_type,
            "threads": args.threads, "beam": args.beam}},
        "llm": {"provider": "openai" if os.environ.get("BB_LLM_MODEL") else "none", "openai": {
            "base_url": os.environ.get("BB_LLM_BASE_URL", ""), "model": os.environ.get("BB_LLM_MODEL", ""),
            "api_key_env": "BB_LLM_API_KEY", "reasoning_effort": os.environ.get("BB_LLM_REASONING", "")}},
        "timezone": os.environ.get("BB_TZ", "UTC"),
    }


def build_stt(cfg: dict, hotwords: str) -> stt_mod.STT:
    return stt_mod.build(cfg.get("stt", {}) or {}, hotwords)


def build_chat(cfg: dict) -> Chat:
    llm_cfg = cfg.get("llm", {}) or {}
    provider = llm_cfg.get("provider", "none")
    persona = llm_cfg.get("persona", "")
    sub = llm_cfg.get(provider, {}) or {}
    if provider == "none":
        return Chat()
    if provider == "anthropic":
        from llm_anthropic import AnthropicChat  # only needs the anthropic package when chosen
        return AnthropicChat(sub, persona)
    if provider == "openai":
        return OpenAICompatChat(sub, persona)
    raise SystemExit(f"unknown llm.provider '{provider}'; choose openai, anthropic or none")
