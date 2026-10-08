# Third-party software and data

BetterButterBot builds on other people's work. This file lists what is included or downloaded, and under which terms.

## Included in this repository

| Component | Where | Licence |
|---|---|---|
| ButterBot firmware, by CircuitMess d.o.o. (the base of the robot firmware) | `firmware/` | MIT. See `firmware/LICENSE-CircuitMess.md` |
| CMF framework, by CircuitMess | `firmware/components/CMF/` | MIT. See its `LICENSE`; we changed `Thread/Threaded.cpp` (thread-stack leak fix) |
| ButterBot-Common, by CircuitMess (incl. the command list `Scenarios.h`) | `firmware/components/ButterBot-Common/`, copied to `gateway/Scenarios.h` | MIT. See its `LICENSE` |
| Flite speech synthesiser, by Carnegie Mellon University and contributors | `firmware/components/flite/` | CMU/Flite licence (permissive; notices in each source file). Not modified by BetterButterBot |
| GLM maths library | `firmware/components/CMF/lib/glm/` | MIT / Happy Bunny. See `copying.txt` |
| LIS3DH driver, by STMicroelectronics | `firmware/main/src/Drivers/lis3dh-pid/` | BSD 3-Clause |
| "butterbot" wake-word model | `firmware/butterbot_wakeword.tflite` | **Personal, non-commercial use.** See below |

## Downloaded when you build or run

| Component | Fetched by | Licence |
|---|---|---|
| ESP-IDF 5.5.3 and Espressif components (esp-sr, esp-dl, esp-tflite-micro, esp32-camera, esp_websocket_client, …) | `setup_idf` / `idf.py build` (versions in `firmware/dependencies.lock`) | mostly Apache-2.0; each component ships its licence. **Note:** esp-sr's speech models carry Espressif's own terms for use on Espressif chips |
| esp-micro-speech-features (ESPHome) | `idf.py build` | Apache-2.0 |
| faster-whisper, CTranslate2 | `pip` | MIT |
| Whisper model weights (OpenAI), via Hugging Face | first gateway start | MIT |
| Silero VAD (bundled in faster-whisper) | `pip` | MIT |
| websockets, httpx, numpy, PyYAML, RapidFuzz, icalendar, onnxruntime, anthropic | `pip` (`gateway/requirements.txt`) | BSD / MIT |
| recurring-ical-events, x-wr-timezone | `pip` | LGPL-3.0-or-later (used unmodified, as libraries) |

## The wake-word model

The "butterbot" model was trained with microWakeWord (Apache-2.0) on:
- synthetic speech from the Piper `en_US-libritts_r-medium` voice;
- background and negative audio from AudioSet, the Free Music Archive (FMA) and microWakeWord's precomputed negative features.

Those datasets come under a mix of licences, some of them non-commercial. **Treat the model as for personal, non-commercial use.** To use BetterButterBot commercially, train your own model with microWakeWord on data you're licensed to use.

## The Butter Song

The alarm song (`firmware/spiffs/alarm/butter_song.aac`) and the timer song (`firmware/spiffs/alarm/toast_song.aac`) have original lyrics and melodies written for this project. They're sung by the flite voice.

## Trademarks

ButterBot and CircuitMess are trademarks of CircuitMess d.o.o. BetterButterBot is an independent community project, not affiliated with or endorsed by CircuitMess. Other product names (Google, Gemini, Claude, OpenAI, …) belong to their owners and are used only to describe compatibility.
