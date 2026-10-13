# bb-voice-gateway

The voice gateway for BetterButterBot. It runs on a computer on your home network and handles everything the robot sends it:

- speech recognition with Whisper;
- matching ButterBot's 238 built-in commands;
- answers from the LLM of your choice;
- timers, alarms, reminders, the calendar and meeting reminders.

Pick **one** way to run it.

## Option A: Docker (recommended)

**Prebuilt image, no source needed.** Copy `docker/docker-compose.yml` and `docker/.env.example` (renamed `.env`) into a folder. For an NVIDIA GPU, set `COMPOSE_PROFILES=gpu` in `.env`. Then:

```
docker compose run --rm setup          # wizard: speech engine, LLM + key, timezone
docker compose up -d
```

See [docker/README.md](docker/README.md) for where the image comes from, updates and everyday commands.

**Build the image yourself from this folder:**

```
python setup.py                                  # writes config.yaml + secret.env here
docker compose --profile cpu up -d --build       # or --profile gpu
```

## Option B: native install (no Docker)

| | Install | Remove |
|---|---|---|
| Windows | `powershell -ExecutionPolicy Bypass -File .\install.ps1` | `.\uninstall.ps1` |
| Linux | `sudo ./install.sh` | `sudo ./uninstall.sh` |
| macOS | `./install.sh` | `./uninstall.sh` |

- **What it sets up:** a private Python environment, then the setup wizard (if there's no `config.yaml` yet), then a background service: a systemd service on Linux, a login item on macOS, a startup task on Windows.
- **Upgrading:** re-run the installer. `config.yaml`, `secret.env` and `data/` are kept.
- **Uninstalling** also keeps those files, unless you add `--purge` (`-Purge` on Windows).
- **Without prompts:** `BB_UNATTENDED=1` (Linux/macOS) or `-Unattended` (Windows) writes a default config instead of asking.

Requires Python 3.10+.

## Files

| | |
|---|---|
| `config.example.yaml` | every setting, with its default |
| `setup.py` | the setup wizard |
| `calendar_auth.py`, `calendar_reauth.ps1` / `.sh` | Google Calendar sign-in and weekly renewal |
| `push.py` | make the robot say something from another program (needs `BB_ADMIN_TOKEN`) |
| `camera.py` | the explorer portal's web server (sign-in page on port 8766) and its accounts: `python camera.py adduser <name>` |
| `portal.py` | explorer mode: VC/RC, one driver at a time, the joystick, typed speech and chat listening, battery and Wi-Fi |
| `portal_page.py` | the portal page itself (HTML, CSS and script) |
| `*.py` (the rest), `Scenarios.h` | the gateway itself |

**Never share or commit** `secret.env` (your keys), `data/` (calendar login, camera accounts, transcripts, recordings) or `secrets/`.

## Guides

Full guides are in the repository's [`docs/`](../docs/) folder:
- [INSTALL](../docs/INSTALL.md): the whole setup, robot included
- [GATEWAY-CONFIG](../docs/GATEWAY-CONFIG.md): every setting
- [LLM-PROVIDERS](../docs/LLM-PROVIDERS.md): keys and settings per provider
- [CALENDAR](../docs/CALENDAR.md)
- [TROUBLESHOOTING](../docs/TROUBLESHOOTING.md)
