# Changelog

## v1.0.1 (2026-10-06)

### Gateway
- **No more silent drops when the LLM is slow or failing.** The robot now says *"Sorry, my brain is a bit busy right now. Ask me again in a moment."* and the log names the reason (for example `ReadTimeout` or `HTTP 503`).
- **Follow-up answers can be one word** ("yes", "Friday"), and a "yes"/"no" right after "Should I add it?" is handled without a round trip to the LLM.
- **Calendar:** one request can add the same event on several days, and the robot only says it drafted an event when it actually did.
- **Optional hedged requests** (`llm.openai.hedge_after`, off by default) for paid or self-hosted servers.
- The version is logged at startup, and routine connection noise is no longer logged.

### Docker
- **Compose bundle** (`gateway/docker/`): runs the prebuilt image with no source checkout. It includes the setup wizard as a `setup` service, and `COMPOSE_PROFILES` in `.env` picks CPU or GPU once, so plain `docker compose up -d`, `logs` and `restart` work.
- **Health check:** `docker compose ps` shows **healthy** once the gateway is listening. Images carry version labels.
- Tested: CPU image, NVIDIA GPU image, and an arm64 build.

### Native installers
- **Upgrades in place:** re-running the installer stops the running gateway first and keeps `config.yaml`, `secret.env` and `data/`.
- **New `uninstall.sh` / `uninstall.ps1`.** They keep your settings unless you pass `--purge` / `-Purge`.
- **Unattended mode:** `BB_UNATTENDED=1` or `-Unattended`.
- **Log rotation:** the log rotates at 5 MB.
- **Fixed:** the NVIDIA GPU library path on Linux, and garbled Windows log files.

### Find me, the camera and movement
- **"Come find me"** (also "come here", "where am I"): starts the controller's Summon routine by voice. It turns in steps until its camera sees a face, then drives to you. The gateway handles these phrases itself, so it works even when the LLM is slow.
- **Live camera view:** "start the camera stream" / "stop the camera".
  - The robot streams 320×240 (JPEG compressed on the robot, about 2–3 frames a second) through the gateway, which serves it on a **sign-in page** (`http://<gateway>:8766/`, accounts from `python camera.py adduser`).
  - The stream runs until you stop it, survives restarts, and pauses while the robot listens or uses its camera for something else.
  - Docker publishes port 8766.
- **Camera actions pause the stream for their whole run.** "Come find me", "who is this", "remember my face" and "what do you see" stop the camera stream once, do their work with full use of the camera, then the stream resumes if it was on. The viewer page stays connected through the pause.
- **Find me reports each step to the gateway log** ("robot: find me: look 3 of 9: 8 frames, 1 detection"), so a run can be followed without a USB cable.
- **Find me reaches farther and aims better.**
  - Long-range detection: the camera switches to 320×240 for the search, and each frame is checked whole and as three full-detail 160×120 crops. The face detector shrinks everything to a fixed ~160×120, so the crops count faces twice as large.
  - Smaller, gentler steps: 30° at 70% speed (13 looks per circle). After each step it waits 0.4 s for the rocking to stop, then looks for 2 s.
  - Aiming: when it sees a face off-centre, it turns by the face's offset (about 60° field of view), stops, looks again, and drives once the face is within ±12% of the middle (up to 3 aims). Turning continuously while waiting for the slow detector overshot.
  - After the camera stream hands the camera back, the camera gets 0.7 s to settle its exposure.
  - "Who is this" and "remember this face" keep the normal 128×128 path, so registered faces still match.
- **The Toast Song for timers:** timers now sing their own song (original, in 6/8), then announce the timer until the button is pressed. A timer more than 10 minutes late is just spoken.
- **Sensors trigger on sustained readings, not blips.** The treads' speed difference makes the robot bounce, so single readings are unreliable while it moves:
  - "Floor lost" (bottom sensor) only counts after 4 seconds without a break, "something close" (front sensor) after 0.5 s. A return to normal is passed on at once.
  - The check before moving needs 3 "unsafe" readings in a row to refuse a start, and "upright" averages 8 accelerometer samples.
  - Applies to every movement: turns, driving, find me, wander.
- **Footing warnings clear as soon as the floor is back.** A momentary "no floor" reading when a movement started could leave the robot thinking it was still lifted, so it refused to move and read its own motion as being shaken or picked up, sometimes minutes later. Now "floor seen" always gets through at once, a one-off reading can't mark the robot lifted, and a motion warning that arrives after the floor is back is dropped.
- **Requests that arrive mid-conversation start right away** once the robot is idle. Before, a "find me" or a timer due during a conversation could wait minutes, until the next idle routine.
- **"Start over" / "forget our conversation"** clears the LLM's short-term memory.
- **The LLM knows the robot's hardware** (camera, sensors, modules) and can start find me, the camera stream, face recognition and dancing through a `robot_action` tool.
- **Fewer false command matches:** a sentence must share a command's meaningful words, not just filler ("what else can you do" no longer starts the camera's "what do you see").

### Firmware space
- About **255 KB of flash freed** (now about 450 KB free) by trimming diagnostics: debug-level log text, assert messages, the error-name table, the factory test-rig self-test, and some status logging. All documented in [firmware/DIAGNOSTICS.md](firmware/DIAGNOSTICS.md), with how to restore each.

### Robot
- **Quieter alarm song:** about 6 dB lower, with the deep bass removed and fades at the start and end, so the speaker no longer pops. Only the storage image changes; flash it with `idf.py -p <port> storage-flash` or the full flash.

## v1.0 (2026-10-06)

Released 2026-10-06. This is the first release.

### What's new compared with the stock ButterBot firmware

#### Voice
- **Network speech recognition.** The robot streams the microphone (16 kHz, µ-law) over Wi-Fi to the voice gateway. Whisper transcribes it there and matches it to ButterBot's 238 built-in command phrases.
  - Wording doesn't have to be exact, and the gateway's speech detector (Silero) decides when you've stopped talking.
  - If the gateway can't be reached, the robot falls back to its original on-device recognition.
- **Free-form questions.** Anything that isn't a command goes to an LLM and the answer is spoken by the robot.
  - Supported: Gemini, OpenRouter, OpenAI, Groq, Ollama, LM Studio, vLLM, Hermes Agent, Claude, or any OpenAI-compatible server.
  - It remembers the last 3 exchanges for 5 minutes.
- **"Butterbot" wake word,** running on the robot (a microWakeWord model in its own flash partition).
- **Follow-up window:** after an answer it listens for 3 more seconds, and saying "Butterbot" in that window restarts listening in under a second.
- **Spoken status:** "Network voice ready" at startup, and "Network voice is back online" after the connection recovers.

#### Assistant features (gateway)
- **Timers, reminders and alarms.** Alarms sing *The Butter Song*: original lyrics and melody, sung by the robot's own voice. They stop when you press the button.
- **Calendar**, from an iCal link (read-only) or Google sign-in (read and write):
  - Daily summaries, and questions about one named calendar.
  - Adding, moving and deleting events, with a spoken read-back and yes/no confirmation.
  - Voice changes are limited to events the robot created; this is enforced in code.
  - Meeting reminders at 15 and 5 minutes before each event ("Heads up…", then "Time to wrap up…"), changeable by voice.
- **Runs on Windows, Linux or macOS:** Docker for CPU or NVIDIA GPU, or native installers, plus a setup wizard (`setup.py`).

#### Robot firmware changes
- Wi-Fi station and WebSocket voice link (`VoiceLink`).
- Internal RAM budget reworked so Wi-Fi, the voice link and the wake word fit alongside the original features (about 30 KB of internal RAM free while running).
- New routines: `SpeakReplyRoutine` and `AlarmRoutine`.
- **Network reliability:**
  - Bluetooth advertising pauses while the gateway link is up (`CONFIG_BB_BLE_QUIET_ON_LAN`).
  - The WebSocket client sends and receives under separate locks (`CONFIG_ESP_WS_CLIENT_SEPARATE_TX_LOCK`).
- Flash layout: the app partition grew to 8880 KB (stock was 99.6% full), and a 64 KB `wakeword` partition was added.
- Fix in the bundled CMF framework: thread stacks are now freed with `vTaskDeleteWithCaps`, which fixes a memory leak.

### Test results (reference robot, 2026-10-06)

| | Before the network fixes | v1.0 |
|---|---|---|
| Connecting to the gateway at power-on | 30 s, 3 attempts | 0.4 s, first try |
| Dropped connections | several per session | 0 in 24 questions over 45 min |
| Listens stuck for more than 10 s | frequent, up to 27 s | none; median 3.7 s, longest 8.5 s |
| Wi-Fi round trip to the robot | spikes to 1.6 s | 63 ms average, 115 ms worst |

Also tested:
- Built-in commands, questions with follow-ups, timers and alarms.
- Calendar reading, writing with confirmation, and the ownership guard, against a real Google calendar.
- Meeting reminders, both live and with a simulated clock.
- **Building from the public source package:**
  - On Windows 11, in a clean folder.
  - On Ubuntu 24.04, from scratch: ESP-IDF installed by `tools/setup_idf.sh`, then configured with `tools/configure_robot.py`, then built.
  - Both builds produce the same image size, and the documented flash command resolves every image and address.
- **The gateway package on Linux:** the setup wizard (`setup.py --yes`) runs and all modules compile.
- **A privacy scan** of every shareable file, including the contents of the zips, found no Wi-Fi details, home addresses, account names, local paths or API keys.

The reference gateway is a Debian 12 VM on 2009 Xeon CPUs, using Whisper `base.en` on the CPU: transcription takes about 1.3 s, and Gemini Flash-Lite answers in 0.6–2.5 s.

### Known limitations

- **Wi-Fi and gateway address are built into the firmware.** Changing either means re-running `configure_robot.py`, rebuilding and re-flashing. There's no on-robot setup yet.
- **The phone app and RC controller can't discover the robot while voice is connected** (Bluetooth advertising is paused). Devices already connected stay connected.
- **English only.** The default speech models are English (`.en`).
- **The wake word was trained on synthetic voices.** Accuracy varies between people; sensitivity is adjustable (`BB_WAKEWORD_CUTOFF`).
- **Google sign-in expires every 7 days** while the Google Cloud app is in "Testing", which is the only option without your own web domain. The robot tells you when it expires; renewal is one command.
- **Alarms need the robot on and connected** at that time. Undeliverable alarms are dropped after 10 minutes.
- **One robot per gateway** is what has been tested. Background announcements go to whichever robot is connected.
- **Not yet tested:**
  - macOS: building the firmware and the native gateway installer.
  - The native gateway installer on a clean Windows machine.
  - LLM and speech providers other than Gemini and faster-whisper.
- **The app partition is 98% full.** About 217 KB is free for future features, so bigger additions will need the flash layout rebalanced (e.g. smaller face or object models).

### Versions

| Component | Version |
|---|---|
| ESP-IDF | 5.5.3 |
| esp-sr / esp-dl / esp-tflite-micro | 2.4.6 / 3.1.5 / 1.3.5 |
| esp_websocket_client | 1.8.0 |
| esp-micro-speech-features | 1.2.3 |
| Gateway: Python | 3.11 (3.10+ supported) |
| faster-whisper / ctranslate2 | 1.2.1 / 4.8.2 |
| websockets / httpx | 17.1 / 0.28.1 |
| onnxruntime (Silero speech detection) | 1.30.0 |
| icalendar | 7.3.0 |
| anthropic SDK | 1.11.0 |

Full lists: `firmware/dependencies.lock` (robot) and `requirements.txt` in the gateway source.
