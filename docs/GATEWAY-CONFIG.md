# Configuring the voice gateway

`python setup.py` writes a working configuration for most people. This guide covers everything you can change afterwards: speech recognition, the robot's personality, follow-up timing, the calendar, reminders, and remote "say this" pushes.

- **Choosing and configuring an LLM:** see [LLM-PROVIDERS.md](LLM-PROVIDERS.md).
- **The calendar:** see [CALENDAR.md](CALENDAR.md).

## Where the settings live

| File | Holds | Share it? |
|---|---|---|
| `gateway/config.yaml` | Your choices (no secrets). [`config.example.yaml`](../gateway/config.example.yaml) lists every option with its default. | fine |
| `gateway/secret.env` | API keys and the calendar link, as `NAME=value` lines | **never** |
| `gateway/data/` | `log.sqlite3` (transcripts, timers, settings changed by voice), `wav/` (recordings), `google_token.json` | **never** |

`setup.py` writes only `timezone`, `stt` and `llm`. Add any other section below by hand.

### Applying a change

| Gateway runs as | After editing `config.yaml` | After editing `secret.env` |
|---|---|---|
| Docker | `docker compose restart` | `docker compose up -d`: `restart` keeps the old environment |
| Linux service | `sudo systemctl restart bb-voice-gateway` | same |
| Windows native | `Stop-ScheduledTask "ButterBot Voice Gateway"; Start-ScheduledTask "ButterBot Voice Gateway"` | same |
| macOS native | `launchctl kickstart -k gui/$(id -u)/com.butterbot.voicegateway` | same |

The first lines of the log show what was loaded. Check them after every change:

<p align="center"><img src="images/gateway-log.svg" alt="Gateway startup log showing the STT, LLM, calendar and endpoint detector in use" width="900"></p>

---

## Speech recognition (`stt`)

| `provider` | Runs | Needs | Typical time per command |
|---|---|---|---|
| `faster-whisper` (default) | on the gateway | nothing | GPU 0.4–0.6 s · modern CPU 1.4–2 s · old CPU (`base.en`) 1.3 s |
| `openai` | any OpenAI-compatible `/audio/transcriptions`: **Groq**, OpenAI, Speaches, LocalAI, vLLM | a key for hosted ones | Groq usually under 0.5 s |
| `whispercpp` | a [whisper.cpp](https://github.com/ggml-org/whisper.cpp) `server` you run | its URL | depends on that machine |
| `deepgram` | Deepgram's hosted API | `DEEPGRAM_API_KEY` | fast |
| `mlx` | Apple Silicon GPU, native install only | a Mac with an M-series chip | fast |

### Local Whisper (`faster-whisper`)

```yaml
stt:
  provider: faster-whisper
  faster-whisper:
    model: auto          # see below
    device: auto         # auto | cuda | cpu
    compute_type: auto   # int8_float16 on GPU, int8 on CPU
    threads: auto        # CPU cores, max 8
    beam: 1              # 1 = fastest; 2-5 is slightly more accurate and slower
    language: en
    hotwords: true       # nudges recognition toward ButterBot's command words
```

`model: auto` picks:
- `large-v3-turbo` on an NVIDIA GPU;
- `small.en` on a CPU with AVX2;
- `base.en` on older CPUs.

AVX2 is only detected on Linux. On Windows and macOS, `auto` assumes it's there, so set `model: base.en` yourself on a pre-2013 CPU. The model downloads on the first start (75 MB to 1.6 GB) and is cached.

Pick by speed. On the 2009 Xeon reference VM (8 threads), each command took:

| model | time per command |
|---|---|
| tiny.en | 0.7 s |
| base.en | 1.4 s |
| small.en | 4.1 s |

### Groq or OpenAI hosted Whisper (`openai`)

```yaml
stt:
  provider: openai
  openai:
    base_url: https://api.groq.com/openai/v1
    model: whisper-large-v3-turbo
    api_key_env: GROQ_API_KEY        # put GROQ_API_KEY=... in secret.env
```

- **OpenAI:** set `base_url: https://api.openai.com/v1`, `model: gpt-4o-mini-transcribe`, `api_key_env: OPENAI_API_KEY`.
- **Self-hosted** (Speaches / faster-whisper-server / LocalAI / vLLM): set `base_url: http://<server-ip>:<port>/v1` and the model name that server uses. Leave out `api_key_env` unless the server needs a key; the default variable name is `BB_STT_API_KEY`.

### whisper.cpp server

```yaml
stt:
  provider: whispercpp
  whispercpp:
    url: http://192.168.1.60:8080
```

### Deepgram

```yaml
stt:
  provider: deepgram
  deepgram:
    model: nova-3
    api_key_env: DEEPGRAM_API_KEY
```

> **Language:** recognition, the 238 built-in commands and the robot's voice are all English. `language` only exists for local Whisper, mlx and the `openai` provider.

---

## Personality (`llm.persona`)

The robot's default personality is a dry, self-aware butter-passing robot that answers in one to three short spoken sentences. To replace it, add `persona` under `llm`:

```yaml
llm:
  provider: openai
  persona: >
    You are ButterBot, a cheerful kitchen robot who loves puns about dairy.
    Your replies are spoken aloud by a robotic voice, so answer in one or two short,
    plain sentences with no lists, markdown, emoji or URLs.
  openai:
    preset: gemini
    model: gemini-flash-lite-latest
```

- **Kept anyway:** the instructions for timers, alarms and the calendar, and the current date and time, are still added after your persona, so those features keep working.
- **Write for speech:** replies are cut at about 400 characters and read by a basic speech synthesiser, so keep the "short, plain sentences, no symbols" part of your persona.

---

## Conversation and listening

```yaml
conversation:
  followup_seconds: 3.0   # silence allowed after an answer before the robot stops listening (0 = no follow-ups)

endpoint:
  detector: silero        # silero (default): neural speech detector, ignores fans, music, clatter
                          # energy: loudness only (used automatically if Silero can't load)
```

**Fixed in this version** (not settable in config):

| Setting | Value |
|---|---|
| Silence that ends a sentence | about 0.5 s (0.85 s after very short phrases, so "Hey Butterbot, … pass the butter" isn't cut in two) |
| Longest single request | 8.5 s |
| Answer window when it asks "Should I add it?" | 6 s |
| LLM memory | last 3 exchanges, forgotten after 5 minutes of silence |
| Spoken answer length | about 400 characters max |

---

## Timezone

```yaml
timezone: Europe/Amsterdam    # any IANA name: Europe/Berlin, Asia/Tokyo, ...
```

It's used for alarms, reminders, the calendar and "what time is it". `setup.py` detects it. Without the setting, the gateway uses `$BB_TZ`, then UTC.

---

## Calendar and meeting reminders

```yaml
calendar:
  provider: google                # google (sign-in, read/write) | ics (secret link, read-only)
  token_file: google_token.json   # google: in gateway/data/
  calendars: selected             # google: selected | primary | [list of calendar ids]
  write_calendar: primary         # google: where "add an event" goes
  ics_url_env: BB_CALENDAR_ICS    # ics: env var with the link(s)
  meeting_reminders: true
  remind_minutes_before: [15, 5]  # one reminder per entry; the last one says "Time to wrap up"
```

Set-up steps are in [CALENDAR.md](CALENDAR.md). Reminder times changed by voice ("remind me 10 and 2 minutes before meetings") are saved in `data/log.sqlite3` and take priority over `config.yaml`.

---

## Explorer portal (`camera`)

The robot portal behind a sign-in on your network: the live camera, VC/RC mode with joystick and arrow-key driving, typed speech and one-shot listening, battery and Wi-Fi. What each control does is in the [user guide](USER-GUIDE.md#explorer-mode-the-robot-portal). The camera only streams once asked (the portal's CS switch, or "Butterbot, start the camera stream") and stays on until asked to stop; it is always off after a restart. Remote control and listening need v1.0.2 robot firmware; older firmware gets the camera and typed speech only.

1. **Create an account** on the gateway machine. It asks for the password twice; nothing goes on the command line:
   | Gateway runs as | Command |
   |---|---|
   | Native | `python camera.py adduser <name>` in the install folder (Linux: `sudo -u <service user> .venv/bin/python camera.py adduser <name>` in `/opt/bb-voice-gateway`) |
   | Docker | `docker compose exec gateway-cpu python camera.py adduser <name>` (or `gateway-gpu`) |

   `python camera.py list` and `python camera.py deluser <name>` manage accounts. Passwords are stored as salted scrypt hashes in `data/camera_users.json`; changes apply without a restart.
2. **Allow incoming TCP 8766** on the gateway computer (like 8765 in [INSTALL.md](INSTALL.md#open-the-firewall)).
3. **Open** `http://<gateway-ip>:8766/` and sign in. Sessions last 12 hours; five wrong passwords lock that address out for 5 minutes. Every signed-in account can drive, one at a time.

```yaml
camera:
  port: 8766          # default
  tls_cert: ""        # set both to serve HTTPS instead of HTTP
  tls_key: ""
  trusted_proxies: ["127.0.0.1", "::1"]   # default: reverse proxies on this machine (see below)
```

Plain HTTP sends the password unencrypted across your network. If that matters, give it a certificate (`tls_cert` / `tls_key`, PEM files) and use `https://`.

**Behind a reverse proxy:** if a proxy on the gateway machine serves the portal (for example to add HTTPS), it should pass `X-Forwarded-For` and `X-Forwarded-Proto`. The portal believes those headers only from the addresses in `trusted_proxies`, so sign-in lockouts count each visitor rather than the proxy, and the session cookie is marked `Secure` over HTTPS. Anyone else sending those headers is judged by their own address. Never put the gateway's port 8765 (the robot's voice link, which has no sign-in) behind a proxy.

---

## Making the robot say something from elsewhere (admin push)

Other programs on your network can make ButterBot speak, or run one of its routines: home automation, scripts, a doorbell.

1. Add a long random token to `secret.env`, then restart the gateway (`up -d` on Docker). Pushes are refused while this is unset.
   ```
   BB_ADMIN_TOKEN=pick-a-long-random-string
   ```
2. Send a push from any machine with Python and the `websockets` package:
   ```
   BB_ADMIN_TOKEN=pick-a-long-random-string python push.py "The laundry is done" ws://<gateway-ip>:8765/bb
   BB_ADMIN_TOKEN=... python push.py --index 9 ws://<gateway-ip>:8765/bb     # run built-in command #9 (a joke)
   ```
   Or send this WebSocket message yourself:
   ```json
   {"type": "push", "token": "pick-a-long-random-string", "text": "The laundry is done"}
   ```
   The reply is `{"type":"push","ok":true}` once the robot has the message. It speaks it the next time it's idle.

Treat the token like a password: anyone on your network who has it can make the robot talk.

---

## Command-line options and environment

Docker and the installers set these for you; they matter only if you run `python gateway.py` yourself.

| Option | Default | |
|---|---|---|
| `--config PATH` | `config.yaml` next to `gateway.py` | **A missing file isn't an error:** the gateway falls back to the legacy options below |
| `--host` / `--port` | `0.0.0.0` / `8765` | |
| `--threshold` | `80` | how close (0–100) a sentence must be to a built-in command to count as one |
| `--no-keep-audio` | recordings kept | stop saving each request's audio to `data/wav/` |
| `--model --device --compute-type --threads --beam` | `small.en cpu int8 8 1` | legacy speech settings, used only when there's no `config.yaml` |

| Environment variable | Meaning |
|---|---|
| `BB_DATA_DIR` | data folder (default `gateway/data`; `/data` in Docker) |
| `BB_TZ` | timezone if `config.yaml` doesn't set one |
| `BB_ADMIN_TOKEN` | enables admin push |
| `BB_CALENDAR_ICS` | iCal link(s) |
| `BB_SCENARIOS` | path to `Scenarios.h` (the command list); the copy in `gateway/` is used by default |
| `BB_LLM_BASE_URL`, `BB_LLM_MODEL`, `BB_LLM_API_KEY`, `BB_LLM_REASONING` | legacy LLM settings, used only without `config.yaml` |
| API key variables | whatever `api_key_env` names: see [LLM-PROVIDERS.md](LLM-PROVIDERS.md) |
