# Troubleshooting

**Where to look:**
- **The gateway log** shows every connection, transcript and answer:
  - Docker: `docker compose logs -f`
  - Linux: `journalctl -u bb-voice-gateway -f`
  - Windows and macOS native: the log file the installer names
- **The robot's log** shows its side. Connect USB and open a serial monitor at 115200 baud, for example `idf.py -p PORT monitor` (exit with Ctrl+]).

---

## Power and flashing

**It turns off as soon as I let go of the button.**
Hold the camera button 4–5 seconds, until it speaks. The firmware takes over the power latch only once it has started.

**The flash stopped partway (`Lost connection`, `device not found`, `Access is denied`).**
The robot lost power because the button was released. With half-written firmware, the button is the only thing keeping it powered.
1. Hold the button.
2. Run the same flash command again and keep holding until `Hard resetting via RTS pin...`.

The bootloader is never overwritten by these commands, so the robot can't be bricked this way.

**No serial port shows up.**
- Use a data-capable USB cable.
- Install the CH340 driver (Windows, macOS).
- Linux: add yourself to the `dialout` group and log in again.
- The port disappears when the robot is off, which is normal.

**`idf.py build` fails.**
- Check the version with `idf.py --version`: it must be v5.5.3.
- Run the ESP-IDF export script first in each new terminal (`. ./export.sh`, or the ESP-IDF PowerShell shortcut on Windows).
- Use a folder path without spaces.
- The first build needs internet access to download components.

## Network

<a id="it-never-says-network-voice-ready"></a>
**It never says "Network voice ready."**
Work through these in order:
1. **Wi-Fi details:** `python tools/configure_robot.py --show` shows what's compiled in. A wrong name or password means re-run configure, rebuild and re-flash.
2. **2.4 GHz:** the robot can't join 5 GHz-only networks. On a combined network, make sure 2.4 GHz is enabled.
3. **Gateway address:** the gateway must be at exactly the IP you built with. Give it a DHCP reservation.
4. **Firewall:** TCP 8765 must be open on the gateway computer. Test from another device: `curl http://<gateway-ip>:8765` should reply (with an error page), not time out.
5. **AP / client isolation** on the Wi-Fi network must be off, or the robot can't reach the gateway.
6. **Gateway log:** `hello from <ip> ... (match)` means it worked. `(MISMATCH, server has …)` means the robot and gateway come from different versions: update both from the same release.

The robot's log narrows it down:
- `Wi-Fi started, joining …` with no `Got IP`: wrong password, or out of 2.4 GHz range.
- `Got IP` then repeated `Error connecting to host`: wrong gateway address, or the firewall is blocking it.

**It says "Network voice is back online" now and then.**
The connection dropped and recovered. Occasional drops are normal. Frequent ones point to Wi-Fi trouble (signal, interference, a busy channel). The robot's log prints `rssi` every minute; around -40 to -65 is fine, below -75 is weak.

**Answers are slow (more than about 5 s after you stop talking).**
- **Speech recognition:** the gateway log's `stt=…ms` is the transcription time. On CPUs without AVX2, choose a smaller model (`base.en`) in `config.yaml` or use a GPU.
- **LLM:** `llm=…ms` is the answer time. Hosted "flash" or "mini" models answer in under a second; large or "thinking" models take several seconds.
- **Long listens:** if listens often run 10 s or more and the robot log shows `Audio send failed`, the Wi-Fi link is struggling. See the drops note above.

## Understanding

**"I didn't understand that" for questions.**
- **No LLM configured:** the gateway's startup log shows `LLM: disabled`. Rerun `python setup.py`.
- **Bad key or provider error:** the gateway log shows `LLM failed: …` with the provider's message. `HTTP 401/403` means a wrong key; `HTTP 503` means the provider is overloaded, which is usually temporary.

**It answers the wrong thing / mishears words.**
The gateway log shows the transcript. Speak a little closer, or pick a larger speech model if your gateway is fast enough.

**The wake word doesn't trigger, or triggers by itself.**
The "butterbot" model was trained on synthetic voices, so how well it works varies from person to person. Sensitivity is *Wake word cutoff* in `idf.py menuconfig` → ButterBot VoiceLink (default 179 of 255): lower is more sensitive, higher means fewer false wakes. Rebuild and flash the app after changing it.

**It keeps listening after "never mind".**
Say "stop listening" or "cancel". It also stops by itself after a few seconds of silence.

## Calendar

**"Calendar: off" in the gateway log.**
- *iCal:* `BB_CALENDAR_ICS` isn't in `secret.env`. On Docker, run `docker compose … up -d` after editing it; `restart` keeps the old environment.
- *Google:* `google_token.json` isn't in the gateway's data folder, or `calendar.provider: google` is missing from `config.yaml`.

**"Your calendar login has expired."**
The Google sign-in expires every 7 days while the Cloud app is in "Testing". Run `calendar_reauth.ps1` / `calendar_reauth.sh`; no gateway restart is needed.

**Google sign-in shows "Access blocked … has not completed the Google verification process".**
Add your Google account under *Audience → Test users* in Google Cloud Console, then sign in again.

**Sign-in works but then "Google Calendar API has not been used in project … or it is disabled".**
Enable the Google Calendar API for that Cloud project, wait a minute, and run the sign-in again.

**It can't add events ("I only have read access").**
You're using the iCal link, or you signed in without `--write` (or unticked the edit box). Sign in again with `calendar_auth.py --write`, leaving both permission boxes ticked.

**It won't delete or move an event.**
By design, it only changes events it added itself. Edit other events in Google Calendar.

**A meeting reminder came late.**
Reminders go out at 15 and 5 minutes before the start. An event added inside that window only gets the reminders that are still ahead of it. New calendars are noticed within 10 minutes.

## Alarms

**The alarm didn't go off.**
The robot has to be on and connected to the gateway at that time. An alarm that can't be delivered within 10 minutes is dropped rather than going off late.

**The alarm or timer only speaks, no song.**
The songs live in the robot's storage partition. A full flash ([INSTALL](INSTALL.md) step 4c) includes it; an app-only flash doesn't.

## Moving

**It won't move: "I can't do that while plugged in".**
A USB cable or charger counts as charging, and the robot doesn't drive while charging. Unplug it.

**Movements stop early with nothing in the way.**
The bottom (cliff) sensor lost the floor for a whole second. Very dark, glossy or deep-pile surfaces can do that; try a lighter, matte surface. Brief flickers are already ignored.

**It says it was picked up or shaken when it wasn't, or warns long after the fact.**
Those warnings come from the bottom sensor and the motion sensor. They now clear the moment the floor is seen again; if they still appear on the floor, the surface is too dark or glossy for the bottom sensor (see above).

**"Come find me" says "On my way" but doesn't move.**
Same two causes as above, or the robot isn't upright. It searches by turning in steps for up to a full circle and says so if it can't see anyone. Its face detector only finds faces within roughly 1–1.5 m, at roughly its own height: put it on a desk and sit in front of it. The gateway log shows each step (`robot: find me: look 3 of 13: 4 frames, 0 detections`, `aim 1: turning 23 degrees left toward the face`). If it keeps stopping short of you or overshooting when it aims, the camera field-of-view estimate (`CameraFovDeg` in `SummonEventRoutine.h`) needs adjusting.

## Camera viewer

| Symptom | Fix |
|---|---|
| The page says there are no accounts | Create one: [GATEWAY-CONFIG.md](GATEWAY-CONFIG.md#camera-viewer-camera). |
| The page doesn't load | Allow TCP 8766 through the gateway computer's firewall (Docker: the compose file publishes it). |
| "Too many failed sign-ins" | Wait 5 minutes. |
| Signed in, but the picture is black or frozen | Say "start the camera stream", then reload. The stream also pauses while the robot is listening or using its camera for something else. |
| The robot says "My firmware needs an update before I can stream my camera" | Flash the current firmware. |

## Phone app / RC controller can't find the robot

That's expected while voice is connected: Bluetooth advertising pauses so Wi-Fi gets the radio. Stop the gateway, or start the robot while the gateway is offline, then pair. See the user guide.
