"""Free-form answers for anything that isn't a ButterBot command - shared pieces plus the
OpenAI-compatible backend.

The OpenAI-compatible backend talks to any POST /chat/completions server, so one implementation
covers hosted APIs and local servers; `preset` only fills in the usual base URL:

  gemini      https://generativelanguage.googleapis.com/v1beta/openai   (key: GEMINI_API_KEY)
  openrouter  https://openrouter.ai/api/v1                               (key: OPENROUTER_API_KEY)
  openai      https://api.openai.com/v1                                  (key: OPENAI_API_KEY)
  groq        https://api.groq.com/openai/v1                             (key: GROQ_API_KEY)
  ollama      http://127.0.0.1:11434/v1                                  (no key)
  lmstudio    http://127.0.0.1:1234/v1                                   (no key)
  vllm        http://127.0.0.1:8000/v1                                   (key optional)
  hermes      http://127.0.0.1:8642/v1  (Nous Research Hermes Agent API server; key: API_SERVER_KEY)

Claude lives in llm_anthropic.py (official Anthropic SDK).
"""
import asyncio
import inspect
import json
import logging
import os
import re
import time
from collections import deque

import httpx

log = logging.getLogger("llm")

SYSTEM_PROMPT = (
    "You are ButterBot, a small desk robot. Your replies are spoken aloud by a robotic "
    "text-to-speech voice, so answer in one to three short, plain sentences. No lists, markdown, "
    "emoji, URLs or special symbols; write numbers and units the way they are spoken. Be helpful "
    "and a little dry and self-aware about being a butter-passing robot, but answer the question "
    "first. If you don't know something current, say so briefly."
)
MAX_SPOKEN_CHARS = 400
HISTORY_TURNS = 3          # remembered question/answer pairs per robot
HISTORY_TTL_S = 300        # forget the conversation after 5 minutes of silence

PRESETS = {
    "gemini": ("https://generativelanguage.googleapis.com/v1beta/openai", "GEMINI_API_KEY"),
    "openrouter": ("https://openrouter.ai/api/v1", "OPENROUTER_API_KEY"),
    "openai": ("https://api.openai.com/v1", "OPENAI_API_KEY"),
    "groq": ("https://api.groq.com/openai/v1", "GROQ_API_KEY"),
    "ollama": ("http://127.0.0.1:11434/v1", ""),
    "lmstudio": ("http://127.0.0.1:1234/v1", ""),
    "vllm": ("http://127.0.0.1:8000/v1", "VLLM_API_KEY"),
    "hermes": ("http://127.0.0.1:8642/v1", "API_SERVER_KEY"),
}


def spoken(text: str) -> str:
    """Strip anything a speech synthesizer would read out literally, and cap the length."""
    text = re.sub(r"[*_#`>|~]+", " ", text)
    text = re.sub(r"https?://\S+", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    if len(text) > MAX_SPOKEN_CHARS:
        cut = text[:MAX_SPOKEN_CHARS]
        end = max(cut.rfind(". "), cut.rfind("? "), cut.rfind("! "))
        text = cut[: end + 1] if end > 80 else cut.rsplit(" ", 1)[0] + "."
    return text


class History:
    """Short per-robot conversation memory (plain text turns only)."""

    def __init__(self):
        self.turns: dict[str, deque] = {}
        self.last_seen: dict[str, float] = {}

    def get(self, device: str) -> deque:
        now = time.time()
        if now - self.last_seen.get(device, 0) > HISTORY_TTL_S:
            self.turns[device] = deque(maxlen=HISTORY_TURNS * 2)
        self.last_seen[device] = now
        return self.turns.setdefault(device, deque(maxlen=HISTORY_TURNS * 2))


async def call_tool(run_tool, name: str, args: dict) -> str:
    """Tools may be plain functions or coroutines (e.g. the calendar fetch)."""
    result = run_tool(name, args)
    if inspect.isawaitable(result):
        result = await result
    return result


# What the robot actually is, added even with a custom persona so it never denies its own hardware.
HARDWARE = (
    " About your body: you are a small wheeled robot with a camera on the front (you can recognise "
    "objects, detect and recognise faces, and remember your owner's face), a microphone and "
    "speaker, one button, front and bottom proximity sensors, a motion sensor, Bluetooth to a "
    "phone app and a remote controller, and a slot for add-on modules such as temperature and "
    "humidity, gas, motion, infrared remote and LED modules."
)


def system_prompt(persona: str, now_text: str, has_tools: bool) -> str:
    system = (persona or SYSTEM_PROMPT) + HARDWARE
    if has_tools:
        system += (" If asked to do something with your camera or wheels (find the user or come "
                   "to them, look at or identify something, recognise or remember a face, dance, start or "
                   "stop the live camera stream), "
                   "call the robot_action tool. Never say you are coming, looking, moving or "
                   "dancing unless robot_action returned ok in this turn; if it failed, say you can't "
                   "right now."
                   " You can set timers, alarms and reminders with your tools; when you use one, "
                   "confirm it in one short sentence. An alarm is for a clock time ('set an alarm "
                   "for 7', 'wake me up at 6:30'); a timer is for a duration. If you can read the "
                   "user's calendar, answer questions about their day with a short spoken synopsis: "
                   "how many events, when the first starts, anything notable, and when they are "
                   "free. Read out every event only if there are three or fewer, and say so plainly "
                   "if the day is empty. If you have calendar editing tools, they only draft a "
                   "change: read the draft back and ask 'Should I add it?' (or move/delete it), "
                   "and never claim it is done until the confirm tool says so. Only say you drafted, "
                   "added or changed something if a tool did it in this turn - to draft another "
                   "change, call the tool again. For the same event on several days, use one "
                   "add_calendar_event with also_on. You can only move or delete events you added "
                   "yourself.")
    if now_text:
        system += f" The current local date and time is {now_text}."
    return system


class Chat:
    """Interface every LLM backend implements."""
    enabled = False

    def describe(self) -> str:
        return "disabled"

    def forget(self, device: str):
        """Drop the remembered conversation with this robot ("start over")."""
        history = getattr(self, "history", None)
        if history is not None:
            history.turns.pop(device, None)

    async def ask(self, device: str, question: str, tools: list | None = None,
                  run_tool=None, now_text: str = "") -> str:
        raise NotImplementedError


class OpenAICompatChat(Chat):
    def __init__(self, cfg: dict, persona: str = ""):
        preset_url, preset_key_env = PRESETS.get(cfg.get("preset", ""), ("", ""))
        self.base_url = (cfg.get("base_url") or preset_url).rstrip("/")
        self.api_key = os.environ.get(cfg.get("api_key_env") or preset_key_env, "") \
            if (cfg.get("api_key_env") or preset_key_env) else ""
        self.model = cfg.get("model", "")
        # Thinking models spend seconds reasoning by default; spoken small talk doesn't need it.
        self.reasoning = cfg.get("reasoning_effort", "")
        self.max_tokens = int(cfg.get("max_tokens", 300))
        self.persona = persona
        self.extra_headers = cfg.get("headers", {}) or {}   # e.g. OpenRouter HTTP-Referer / X-Title
        self.timeout = float(cfg.get("timeout", 10))
        # Optional: if no answer has arrived after hedge_after seconds, send the same request again
        # and use whichever answers first. Off by default: on rate-limited free tiers (where slow
        # answers usually *are* the rate limit) the extra request makes things worse. Useful on paid
        # or self-hosted servers that occasionally stall a single request.
        self.hedge_after = float(cfg.get("hedge_after", 0))
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(self.timeout, connect=3.0))
        self.history = History()
        self.enabled = bool(self.base_url and self.model)

    def describe(self) -> str:
        if not self.enabled:
            return "disabled (set llm.openai.model and preset/base_url)"
        return f"{self.model} @ {self.base_url} (key {'set' if self.api_key else 'none'})"

    async def _complete(self, messages: list, tools: list | None) -> dict:
        """One chat completion, hedged: a second identical request goes out if the first is slow."""
        first = asyncio.ensure_future(self._post(messages, tools))
        if self.hedge_after <= 0:
            return await first
        done, _ = await asyncio.wait({first}, timeout=self.hedge_after)
        if done:
            return first.result()
        log.info("LLM slow after %.1fs, sending a second request", self.hedge_after)
        tasks = {first, asyncio.ensure_future(self._post(messages, tools))}
        error = None
        try:
            while tasks:
                done, tasks = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
                for t in done:
                    if t.exception() is None:
                        return t.result()
                    error = t.exception()
            raise error
        finally:
            for t in tasks:
                t.cancel()

    async def _post(self, messages: list, tools: list | None) -> dict:
        headers = {"Authorization": f"Bearer {self.api_key}"} if self.api_key else {}
        headers.update(self.extra_headers)
        body = {"model": self.model, "messages": messages, "max_tokens": self.max_tokens,
                "temperature": 0.7}
        if tools:
            body["tools"] = tools
        if self.reasoning:
            body["reasoning_effort"] = self.reasoning
        r = await self.client.post(f"{self.base_url}/chat/completions", headers=headers, json=body)
        if r.status_code >= 400:
            raise RuntimeError(f"HTTP {r.status_code}: {r.text[:300]}")
        return r.json()["choices"][0]["message"]

    async def ask(self, device: str, question: str, tools: list | None = None,
                  run_tool=None, now_text: str = "") -> str:
        """run_tool(name, args_dict) -> str result, used when the model calls one of `tools`."""
        hist = self.history.get(device)
        messages = [{"role": "system", "content": system_prompt(self.persona, now_text, bool(tools))},
                    *hist, {"role": "user", "content": question}]

        msg = await self._complete(messages, tools)
        for _ in range(3):  # tool-call rounds
            calls = msg.get("tool_calls") or []
            if not calls or run_tool is None:
                break
            messages.append({"role": "assistant", "content": msg.get("content") or "",
                             "tool_calls": calls})
            for call in calls:
                fn = call["function"]
                try:
                    args = json.loads(fn.get("arguments") or "{}")
                except json.JSONDecodeError:
                    args = {}
                result = await call_tool(run_tool, fn["name"], args)
                log.info("tool %s(%s) -> %s", fn["name"], args, result)
                messages.append({"role": "tool", "tool_call_id": call.get("id", ""),
                                 "name": fn["name"], "content": result})
            msg = await self._complete(messages, tools)

        answer = spoken(msg.get("content") or "")
        hist.append({"role": "user", "content": question})
        hist.append({"role": "assistant", "content": answer})
        return answer

    async def models(self) -> list[str]:
        headers = {"Authorization": f"Bearer {self.api_key}"} if self.api_key else {}
        r = await self.client.get(f"{self.base_url}/models", headers=headers)
        r.raise_for_status()
        return [m["id"] for m in r.json().get("data", [])]
