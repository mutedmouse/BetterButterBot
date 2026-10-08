# Diagnostics removed to save space

The app partition was 98% full (about 210 KB free). These diagnostics were taken out on 2026-10-07 to make room for new features, which roughly doubles the free space. Each entry below says what the diagnostic did and how to put it back if you need it again while debugging.

The removed code is in the BetterButterBot v1.0 source package (`firmware-source.zip`). The factory test also lives in CircuitMess's public firmware: https://github.com/CircuitMess/ButterBot-Firmware-Public (`main/src/JigHWTest/`).

## Build settings (`sdkconfig`), about 221 KB

Measured together: the app went from 0x878bf0 to 0x8419d0 bytes.

| Setting | Was | Now | What changes |
|---|---|---|---|
| `CONFIG_LOG_MAXIMUM_LEVEL` | 4, Debug | 3, Info (`CONFIG_LOG_MAXIMUM_EQUALS_DEFAULT`) | Debug-level log text from every component (Wi-Fi, Bluetooth, TCP/IP, camera, CMF) is no longer compiled in, so `esp_log_level_set(..., ESP_LOG_DEBUG)` has no effect. The firmware's own logs use Info and above. Probably most of the saving. |
| `CONFIG_COMPILER_OPTIMIZATION_ASSERTION_LEVEL` | 2, full (`ASSERTIONS_ENABLE`) | 1, silent (`ASSERTIONS_SILENT`) | A failed `assert()` still stops the program, but without the file, line and expression text. The panic backtrace still identifies the location: `xtensa-esp32s3-elf-addr2line -pfiaC -e build/ButterBot-Firmware.elf <addresses>`. |
| `CONFIG_ESP_ERR_TO_NAME_LOOKUP` | y | n | Error codes print as numbers (`ESP_ERR 0x101`) instead of names (`ESP_ERR_NO_MEM`). Look them up in `components/esp_common/include/esp_err.h` or the component's header. |

**To restore:** `idf.py menuconfig`:
- Component config → Log → Maximum log verbosity → Debug.
- Compiler options → Assertion level → Enabled.
- Component config → Common ESP-related → "Enable lookup of error code strings".

Each of these forces a full rebuild.

## Code, about 22 KB

| Removed | Where it was | What it did | To restore |
|---|---|---|---|
| Factory test-rig self-test (`JigHWTest`), 20 KB | `main/src/JigHWTest/`; the `JigHWTest::checkJig()` block at the start of `Butterbot::init` in `main/main.cpp` | If the robot booted in CircuitMess's factory test rig, it ran a hardware self-test (I/O expander, motor board, accelerometer, RTC, camera, battery, voltage reference, speaker and mic, SPIFFS checksum) and reported over serial. Never runs on a normal robot. | Copy `main/src/JigHWTest/` back from the v1.0 source or CircuitMess's firmware, re-add `#include "JigHWTest/JigHWTest.h"` and the `checkJig()` block, then `idf.py reconfigure` (new source files). |
| Status-LED register dump | `logStatusLed()` in `main/main.cpp`, called at init, at idle and on network ready/down | Logged the I/O expander's output and direction registers for the status LED pin (P0), to debug an LED that seemed stuck. The LED was confirmed working. | v1.0 source |
| `[DBG]` critical-battery prints | `onBatteryLevelEvent()` in `main/main.cpp` | Printed the live and cached charge state before a low-battery shutdown, to catch a stale charge-state cache. The shutdown logic is unchanged. | v1.0 source |
| Wake-word statistics | `WakeWord::loop()` / `process()` in `main/src/Services/WakeWord.cpp` | Logged once a minute: model runs, average run time (about 3.8 ms) and peak detection score. Used while tuning the wake word. | v1.0 source |
| Per-listen and on-connect heap lines | `ListenState.cpp` (`listen-stream`, `listen-result`), `VoiceLink.cpp` (`ready`) | Logged internal-RAM figures at every listen and at every gateway connect. | v1.0 source |

**Kept:** the once-a-minute `heap[minute]` line from `VoiceLink`. It shows free internal RAM, the largest free block, the all-time minimum and Wi-Fi signal. Internal RAM is the robot's tightest resource (about 30 KB free), so it's the first thing to check if anything misbehaves.

## Also removed earlier the same day

- **Movement diagnostics**, about 3 KB. Every bottom/front proximity change and motion event was sent to the gateway log (`robot: move-diag: ...`). They showed that movements stopped because the bottom (cliff) sensor briefly lost a dark or glossy floor while the robot rocked. That's handled by requiring sustained readings (`FloorLostConfirmMs` / `FrontCloseConfirmMs` in `main/src/Services/BaseBoard.h`). Find me still reports its steps to the gateway log (`robot: find me: ...`); remove `SummonEventRoutine::note()` calls to save about 2 KB. The gateway still logs any `{"type":"log"}` message a future firmware sends.
- **Find me diagnostics:** which sensor interrupted it, and in which phase.

## Where the rest of the space goes

For future features, from `idf.py size-components`:

| Part | Flash |
|---|---|
| Robot voice (flite) | 4.6 MB, almost all voice data |
| Main robot code | 0.8 MB |
| Face and object AI (esp-dl, TFLite Micro) | about 1.0 MB |
| Bluetooth | about 0.47 MB |
| Wi-Fi and networking | about 0.3 MB |

The biggest remaining levers aren't diagnostics:
- **A smaller flite voice.** It would sound different.
- **Building for size instead of speed** (`CONFIG_COMPILER_OPTIMIZATION_SIZE`). It saves hundreds of KB but may slow wake-word detection and audio. Measure before adopting.
