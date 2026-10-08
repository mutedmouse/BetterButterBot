# LLM providers

The LLM answers everything that isn't one of ButterBot's built-in commands: questions, small talk, and requests that use tools (timers, alarms, reminders, the calendar). Built-in commands never reach the LLM, so they keep working with any choice here, including none at all.

**Speed is what matters.** The robot speaks the answer, and you're waiting for it. Pick a small, fast model: answers are short, and a large "thinking" model adds seconds while barely improving them. On the reference setup, Gemini Flash-Lite answers in 0.6–2.5 s.

| Provider | Setting | Key | Runs | Good starting model |
|---|---|---|---|---|
| Google Gemini | `preset: gemini` | `GEMINI_API_KEY` | hosted, free tier | `gemini-flash-lite-latest` |
| Anthropic Claude | `provider: anthropic` | `ANTHROPIC_API_KEY` | hosted | `claude-haiku-4-5` (fastest) or `claude-sonnet-5-5` |
| OpenAI | `preset: openai` | `OPENAI_API_KEY` | hosted | a "mini" or "nano" chat model on your account |
| OpenRouter | `preset: openrouter` | `OPENROUTER_API_KEY` | hosted, hundreds of models | any fast model id, `vendor/model` |
| Groq | `preset: groq` | `GROQ_API_KEY` | hosted, very fast | a small Llama model from Groq's list |
| Ollama | `preset: ollama` | none | **your own machine** | `llama3.2:3b` |
| LM Studio | `preset: lmstudio` | none | **your own machine** | any small instruct model |
| vLLM | `preset: vllm` | `VLLM_API_KEY` (optional) | **your own server** | the model you serve |
| Hermes Agent | `preset: hermes` | `API_SERVER_KEY` | **your own server** | whatever its API server exposes |
| Anything OpenAI-compatible | `base_url: …` | your choice | anywhere | |

## How keys work

API keys never go in `config.yaml`. Each provider's config names an **environment variable** (`api_key_env`), and you put the value in `gateway/secret.env`:

```
GEMINI_API_KEY=AIza...your-key...
```

- **Typed for you:** `setup.py` asks for the key with hidden input and writes this file.
- **Docker:** after changing `secret.env`, run `docker compose up -d`, not `restart`.
- **Native installs:** restart the service.

The startup log confirms what was loaded. `(key set)` means the variable was found; `(key none)` means it's missing or empty:

```
INFO LLM: gemini-flash-lite-latest @ https://generativelanguage.googleapis.com/v1beta/openai (key set)
```

---

## Google Gemini

1. **Get a key.** Sign in at [Google AI Studio](https://aistudio.google.com) and open *Get API key*, then *Create API key*. The free tier is enough for a household.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: gemini
       model: gemini-flash-lite-latest
       api_key_env: GEMINI_API_KEY
   ```
3. **`secret.env`:** `GEMINI_API_KEY=...`

Tips:
- `gemini-flash-lite-latest` was the best fit in testing.
- Full "Flash" models think before answering and took 8 s or more. Some model names aren't available on every account and return HTTP 404.
- When Google is overloaded you'll see `LLM failed: HTTP 503` and "I didn't understand that". It passes.

## Anthropic Claude

Uses the official Anthropic Python SDK.

1. **Get a key:** Anthropic Console → *API Keys* → *Create Key*. API use is billed separately from a Claude.ai subscription.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: anthropic
     anthropic:
       model: claude-haiku-4-5        # fastest; claude-sonnet-5-5 or claude-opus-5-5 are smarter but slower
       effort: low                    # low | medium | high | xhigh | max (not used with Haiku 4.5)
       fallbacks: true                # Fable/Opus/Sonnet 5.x: a fallback model answers if a request is declined
       api_key_env: ANTHROPIC_API_KEY
   ```
3. **`secret.env`:** `ANTHROPIC_API_KEY=sk-ant-...`

Keep `effort: low` for spoken answers.

## OpenAI

1. **Get a key:** OpenAI Platform → *API keys* → *Create new secret key*. This needs billing set up.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: openai
       model: <a fast chat model on your account>
       api_key_env: OPENAI_API_KEY
   ```
3. **`secret.env`:** `OPENAI_API_KEY=sk-...`

For reasoning models, add `reasoning_effort: low` (or `minimal`, if the model supports it) to keep answers quick.

## OpenRouter

One key for hundreds of models from many vendors.

1. **Get a key:** [openrouter.ai](https://openrouter.ai) → *Keys* → *Create key*.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: openrouter
       model: vendor/model-name          # copy the id from the model's page
       api_key_env: OPENROUTER_API_KEY
       headers: {X-Title: "ButterBot"}   # optional: how the app shows in your OpenRouter activity
   ```
3. **`secret.env`:** `OPENROUTER_API_KEY=sk-or-...`

## Groq

Very low latency on open models.

1. **Get a key:** [console.groq.com](https://console.groq.com) → *API Keys*.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: groq
       model: <model id from Groq's models list>
       api_key_env: GROQ_API_KEY
   ```
3. **`secret.env`:** `GROQ_API_KEY=gsk_...`

Groq also does fast hosted speech recognition with the same key; see [GATEWAY-CONFIG.md](GATEWAY-CONFIG.md#groq-or-openai-hosted-whisper-openai).

---

## Local models: Ollama, LM Studio, vLLM

Nothing leaves your network. You need a machine with enough memory for the model; small 3–8B models are plenty for spoken answers.

> **Using Docker for the gateway?** Inside the container, `127.0.0.1` means the container itself, not your computer.
> - If the model server runs on another machine, or on the same machine but outside Docker, set `base_url` to that machine's **LAN IP**.
> - The model server must listen on the network, not just on localhost.

### Ollama

1. Install from [ollama.com](https://ollama.com), then `ollama pull llama3.2:3b`.
2. To make it reachable from other machines or from Docker, set `OLLAMA_HOST=0.0.0.0` in Ollama's environment and restart it.
3. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: ollama
       model: llama3.2:3b
       base_url: http://192.168.1.60:11434/v1     # leave out if Ollama is on the same machine and not in Docker
   ```

Tool use (timers, calendar) needs a model that supports tool calling. The Llama 3.x and Qwen 2.5+ families do.

### LM Studio

1. In LM Studio, download a small instruct model and load it.
2. In the *Developer* tab, start the server and turn on *Serve on Local Network*.
3. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: lmstudio
       model: <the model identifier LM Studio shows>
       base_url: http://192.168.1.60:1234/v1
   ```

### vLLM

1. Start the server: `vllm serve <model> --host 0.0.0.0 --port 8000`. Add `--api-key <key>` to require one.
2. **`config.yaml`:**
   ```yaml
   llm:
     provider: openai
     openai:
       preset: vllm
       model: <model name as served>
       base_url: http://192.168.1.60:8000/v1
       api_key_env: VLLM_API_KEY      # only if you used --api-key; put VLLM_API_KEY=<key> in secret.env
   ```

### Hermes Agent

ButterBot can talk to a [Hermes Agent](https://github.com/NousResearch) API server, for an assistant that keeps its own memory and skills.

```yaml
llm:
  provider: openai
  openai:
    preset: hermes                 # http://127.0.0.1:8642/v1 by default
    base_url: http://192.168.1.60:8642/v1
    model: <the model name your Hermes API server expects>
    api_key_env: API_SERVER_KEY
```

### Any other OpenAI-compatible server

Leave out `preset`, and give `base_url` (up to `/v1`), `model`, and `api_key_env` if the server needs a key. The gateway calls `POST {base_url}/chat/completions` with standard tool calling.

---

## All `llm` settings

```yaml
llm:
  provider: openai          # openai | anthropic | none
  persona: "..."            # optional: replaces the built-in personality, see GATEWAY-CONFIG.md
  openai:
    preset: gemini          # fills in base_url and the key variable
    base_url: https://...   # overrides the preset
    model: ...              # required
    api_key_env: ...        # overrides the preset's variable; no key is sent if empty
    reasoning_effort: low   # sent only if set
    max_tokens: 300
    timeout: 10             # seconds
    hedge_after: 0          # >0: if no answer after this many seconds, send the request again and
                            #   use whichever answers first (paid/self-hosted servers; not free tiers)
    headers: {}             # extra HTTP headers
  anthropic:
    model: claude-opus-5-5
    effort: low
    fallbacks: true
    api_key_env: ANTHROPIC_API_KEY
    max_tokens: 2048
    timeout: 15
```

Fixed in this version: temperature 0.7, up to 3 rounds of tool calls per question, and memory of the last 3 exchanges (forgotten after 5 minutes).

## When it goes wrong

When the LLM fails or times out, the robot says *"Sorry, my brain is a bit busy right now. Ask me again in a moment."* The gateway log shows the reason:

| Gateway log shows | Meaning |
|---|---|
| `LLM: disabled` | `provider: none`, or `model`/`base_url` is empty |
| `(key none)` at startup | the key variable isn't set; check `secret.env` and restart properly (`up -d` on Docker) |
| `LLM failed: HTTP 401` / `403` | wrong key, or the key lacks access to that model |
| `LLM failed: HTTP 404` | the model name doesn't exist for your account or server |
| `LLM failed: HTTP 429` | rate limit or free-tier quota. Free tiers cap requests per minute, and a calendar question can use 2–3 requests |
| `LLM failed: ReadTimeout` | no answer within `timeout`. On free tiers this often happens just before 429s start; otherwise the provider is stalling |
| `LLM failed: HTTP 503` | the provider is overloaded; temporary |
| `LLM failed: ... connect ...` | local server unreachable: check `base_url`, that the server listens on the network, and the firewall |
| Answers but never sets timers or reads the calendar | the model doesn't support tool calling; pick one that does |
