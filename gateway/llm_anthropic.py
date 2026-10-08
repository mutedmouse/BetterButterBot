"""Claude backend for free-form answers, via the official Anthropic Python SDK.

Config (config.yaml, llm.provider: anthropic):
  model       default claude-opus-5-5. For snappier replies on a voice robot owners often pick
              claude-sonnet-5-5 or claude-haiku-4-5 - their choice.
  effort      low|medium|high|xhigh|max (sent as output_config.effort; not sent for Haiku 4.5,
              which doesn't support it). Default low: short spoken chat doesn't need deep thinking.
  fallbacks   true (default) - on models that support server-side fallback, a request declined
              by the safety classifiers is re-run on Anthropic's recommended fallback model.
  api_key_env name of the env var holding the key (default ANTHROPIC_API_KEY).
"""
import json
import logging
import os

import anthropic

from llm import Chat, History, call_tool, spoken, system_prompt

log = logging.getLogger("llm.anthropic")

# Models that accept output_config.effort / the server-side fallbacks "default" form.
NO_EFFORT = ("claude-haiku-4-5",)
FALLBACK_MODELS = ("claude-fable-5-1", "claude-opus-5-5", "claude-opus-5", "claude-sonnet-5-5")
FALLBACK_BETA = "server-side-fallback-2026-07-01"
LATENCY_HINT = " Latency-sensitive; begin your visible answer immediately."


def to_anthropic_tools(tools: list | None) -> list:
    """OpenAI-style function tools (agent.TOOLS) -> Anthropic tool definitions."""
    out = []
    for t in tools or []:
        fn = t["function"]
        out.append({"name": fn["name"], "description": fn.get("description", ""),
                    "input_schema": fn.get("parameters", {"type": "object", "properties": {}})})
    return out


class AnthropicChat(Chat):
    def __init__(self, cfg: dict, persona: str = ""):
        self.model = cfg.get("model", "claude-opus-5-5")
        self.effort = cfg.get("effort", "low")
        # Thinking tokens count toward max_tokens on adaptive-thinking models; leave room.
        self.max_tokens = int(cfg.get("max_tokens", 2048))
        self.use_fallbacks = bool(cfg.get("fallbacks", True)) and self.model in FALLBACK_MODELS
        self.persona = persona
        key = os.environ.get(cfg.get("api_key_env", "ANTHROPIC_API_KEY"), "")
        # Zero-arg resolution also works (ANTHROPIC_API_KEY / `ant auth login` profile).
        self.client = anthropic.AsyncAnthropic(api_key=key or None,
                                               timeout=float(cfg.get("timeout", 15)), max_retries=1)
        self.history = History()
        self.enabled = True

    def describe(self) -> str:
        extras = []
        if self.model not in NO_EFFORT:
            extras.append(f"effort {self.effort}")
        if self.use_fallbacks:
            extras.append("fallbacks default")
        return f"anthropic {self.model} ({', '.join(extras) or 'defaults'})"

    async def _create(self, **kwargs):
        if self.model not in NO_EFFORT and self.effort:
            kwargs["output_config"] = {"effort": self.effort}
        if self.use_fallbacks:
            return await self.client.beta.messages.create(betas=[FALLBACK_BETA], fallbacks="default", **kwargs)
        return await self.client.messages.create(**kwargs)

    async def ask(self, device: str, question: str, tools: list | None = None,
                  run_tool=None, now_text: str = "") -> str:
        hist = self.history.get(device)
        system = system_prompt(self.persona, now_text, bool(tools)) + LATENCY_HINT
        messages = [*hist, {"role": "user", "content": question}]
        anthropic_tools = to_anthropic_tools(tools)

        try:
            for _ in range(4):  # initial call + up to 3 tool rounds
                kwargs = {"model": self.model, "max_tokens": self.max_tokens, "system": system,
                          "messages": messages}
                if anthropic_tools:
                    kwargs["tools"] = anthropic_tools
                response = await self._create(**kwargs)

                if response.stop_reason == "refusal":
                    log.info("declined (%s)", getattr(response.stop_details, "category", None))
                    answer = "Sorry, I can't help with that one."
                    break

                if response.stop_reason == "tool_use" and run_tool is not None:
                    # Append the assistant turn unchanged (thinking blocks included), then every
                    # tool result in a single user message.
                    messages.append({"role": "assistant", "content": response.content})
                    results = []
                    for block in response.content:
                        if block.type == "tool_use":
                            result = await call_tool(run_tool, block.name, block.input if isinstance(block.input, dict)
                                                     else json.loads(block.input or "{}"))
                            log.info("tool %s(%s) -> %s", block.name, block.input, result)
                            results.append({"type": "tool_result", "tool_use_id": block.id,
                                            "content": result})
                    messages.append({"role": "user", "content": results})
                    continue

                answer = " ".join(b.text for b in response.content if b.type == "text")
                break
            else:
                answer = ""
        except anthropic.RateLimitError as e:
            raise RuntimeError(f"rate limited: {e.message}") from e
        except anthropic.APIStatusError as e:
            raise RuntimeError(f"HTTP {e.status_code}: {e.message}") from e
        except anthropic.APIConnectionError as e:
            raise RuntimeError(f"connection error: {e}") from e

        answer = spoken(answer)
        hist.append({"role": "user", "content": question})
        hist.append({"role": "assistant", "content": answer})
        return answer
