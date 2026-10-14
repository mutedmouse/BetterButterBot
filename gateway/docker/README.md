# bb-voice-gateway: Docker deploy bundle

Runs the BetterButterBot voice gateway from an image **you built** from your own copy of the code, on a machine that has neither the source nor Python, only Docker. This project publishes no images; build yours as described in the project's `docs/INSTALL.md` (Option A), then push it to your own registry or move it as a file.

## 1. Set up the folder

Put `docker-compose.yml` and `.env.example` in an empty folder, and rename `.env.example` to `.env`. In `.env`:

| Setting | Use |
|---|---|
| `COMPOSE_PROFILES=cpu` | any 64-bit machine: Intel/AMD, or ARM (Raspberry Pi 5, Apple Silicon, ARM servers) |
| `COMPOSE_PROFILES=gpu` | an NVIDIA GPU: Linux with the NVIDIA Container Toolkit, or Windows with Docker Desktop (WSL2) |
| `BB_IMAGE=ghcr.io/<your-account>/bb-voice-gateway` | pull your image from **your own** registry |
| `BB_IMAGE=bb-voice-gateway` | use your image file, loaded with `docker load -i bb-voice-gateway-cpu.tar.gz` |
| `BB_CPU_TAG` / `BB_GPU_TAG` | the tags you gave your image (default `cpu` / `gpu`) |

## 2. Configure

```
docker compose run --rm setup
```

This is the same wizard as the native install. It asks:
- which speech engine to use;
- which LLM and its key (typing is hidden);
- your timezone: **type it**, because inside the container it can't be detected;
- optionally, a calendar link.

It writes `config.yaml`, `secret.env` and `data/` into this folder. For an automated deploy, run `docker compose run --rm setup --yes` instead: it accepts the defaults (local Whisper, no LLM), and you edit `config.yaml` afterwards. Every option is described in the project's `docs/GATEWAY-CONFIG.md`.

On Linux the files are created by the container as root. To edit them, use `sudo`, or `sudo chown -R $USER: .` once.

## 3. Start

```
docker compose up -d
docker compose logs -f        # the first start downloads the speech model (up to 1.6 GB)
```

`docker compose ps` shows **healthy** once the gateway is listening on port 8765. Allow incoming TCP 8765 through the host firewall. The robot's firmware needs `ws://<this machine's IP>:8765/bb`.

**Explorer portal (optional, camera + remote control + chat):** create an account with `docker compose exec gateway-cpu python camera.py adduser <name>` (`gateway-gpu` on the GPU profile; it asks for the password), allow TCP 8766, and open `http://<this machine's IP>:8766/`.

## Everyday commands

| | |
|---|---|
| Changed `config.yaml` | `docker compose restart` |
| Changed `secret.env` or `.env` | `docker compose up -d` (`restart` would keep the old values) |
| Update | build and push (or save and load) your new image, then `docker compose pull && docker compose up -d` |
| Stop / remove | `docker compose down`. Your files in this folder stay |
| Calendar sign-in token | put `google_token.json` in `data/`; see the project's `docs/CALENDAR.md` |

**Keep private:** `secret.env` and `data/` (keys, calendar login, transcripts, recordings).
