#!/bin/bash
# ButterBot voice gateway - native install (no Docker) on Linux or macOS.
# Re-running it upgrades the code and keeps config.yaml, secret.env and data/.
#
#   Linux:  sudo ./install.sh        -> /opt/bb-voice-gateway, systemd service, runs as $SUDO_USER
#   macOS:  ./install.sh             -> ~/bb-voice-gateway, launchd agent (starts at login)
#   Override: APP=/path RUN_AS=user  (Linux)    APP=/path (macOS)
#   Unattended (no prompts, default config if none): BB_UNATTENDED=1, or run without a terminal
#   Remove: sudo ./uninstall.sh [--purge]      Windows: use install.ps1
set -euo pipefail

SRC=$(cd "$(dirname "$0")" && pwd)
OS=$(uname -s)

if [ "$OS" = Darwin ]; then
  APP=${APP:-$HOME/bb-voice-gateway}
  RUN_AS=$(id -un)
  AS_USER=()                      # run as yourself; no sudo on macOS
else
  [ "$(id -u)" -eq 0 ] || { echo "Run with sudo."; exit 1; }
  APP=${APP:-/opt/bb-voice-gateway}
  RUN_AS=${RUN_AS:-${SUDO_USER:-$(id -un)}}
  AS_USER=(sudo -u "$RUN_AS")
fi

PY=${PYTHON:-python3}
if ! "$PY" -c 'import sys; assert sys.version_info >= (3, 10)' 2>/dev/null; then
  echo "Python 3.10+ is required (macOS: brew install python@3.12, or python.org installer)."; exit 1
fi
if [ "$OS" = Linux ] && ! "$PY" -c 'import ensurepip' 2>/dev/null; then
  command -v apt-get >/dev/null && { apt-get update -qq; apt-get install -y -qq python3-venv; }
fi

echo "== installing bb-voice-gateway $(cat "$SRC/VERSION" 2>/dev/null || echo dev) to $APP (runs as $RUN_AS)"
mkdir -p "$APP/data"
cp "$SRC"/*.py "$SRC"/Scenarios.h "$SRC"/config.example.yaml "$SRC"/requirements.txt "$SRC"/VERSION "$SRC"/uninstall.sh "$APP"/
for f in config.yaml secret.env; do                   # bring over a config made with setup.py
  if [ -f "$SRC/$f" ] && [ ! -f "$APP/$f" ]; then cp "$SRC/$f" "$APP/$f"; fi
done
[ "$OS" = Linux ] && chown -R "$RUN_AS": "$APP"

${AS_USER[@]+"${AS_USER[@]}"} "$PY" -m venv "$APP/.venv"
${AS_USER[@]+"${AS_USER[@]}"} "$APP/.venv/bin/pip" install -q --upgrade pip
${AS_USER[@]+"${AS_USER[@]}"} "$APP/.venv/bin/pip" install -q -r "$APP/requirements.txt"

LD_PATH=""
if [ "$OS" = Linux ] && command -v nvidia-smi >/dev/null && nvidia-smi -L >/dev/null 2>&1; then
  echo "== NVIDIA GPU: $(nvidia-smi --query-gpu=name --format=csv,noheader | head -1); adding CUDA 12 / cuDNN 9 libraries"
  ${AS_USER[@]+"${AS_USER[@]}"} "$APP/.venv/bin/pip" install -q nvidia-cublas-cu12 "nvidia-cudnn-cu12==9.*"
  # __path__, not __file__: newer nvidia-* wheels make these namespace packages (no __file__)
  LD_PATH=$("$APP/.venv/bin/python" -c 'import nvidia.cublas.lib as a, nvidia.cudnn.lib as b; print(":".join(list(m.__path__)[0] for m in (a, b)))')
fi

if [ ! -f "$APP/config.yaml" ]; then
  SETUP_ARGS=()
  if [ -n "${BB_UNATTENDED:-}" ] || [ ! -t 0 ]; then
    SETUP_ARGS=(--yes)              # no keyboard (automation): defaults - local Whisper, no LLM
    echo "== no config.yaml yet - writing a default one (edit it, or run setup.py, then restart)"
  else
    echo "== no config.yaml yet - starting the setup wizard"
  fi
  ${AS_USER[@]+"${AS_USER[@]}"} env BB_INSTALLER=1 "$APP/.venv/bin/python" "$APP/setup.py" --dir "$APP" ${SETUP_ARGS[@]+"${SETUP_ARGS[@]}"}
fi
touch "$APP/secret.env"; chmod 600 "$APP/secret.env"
[ "$OS" = Linux ] && chown "$RUN_AS": "$APP/secret.env"

# The service reads KEY=value lines from secret.env (systemd EnvironmentFile; on macOS a tiny
# launcher exports them, since launchd has no env-file option).
cat > "$APP/run.sh" <<EOF
#!/bin/bash
cd "$APP"
set -a; [ -f secret.env ] && . ./secret.env; [ -f admin.env ] && . ./admin.env; set +a
export PYTHONUNBUFFERED=1 HF_HOME="$APP/data/hf" BB_DATA_DIR="$APP/data" LD_LIBRARY_PATH="$LD_PATH"
# macOS logs to a file (Linux uses the journal): start a fresh one once it passes 5 MB
LOG="$APP/data/gateway.log"
if [ -f "\$LOG" ] && [ "\$(wc -c < "\$LOG")" -gt 5000000 ]; then mv -f "\$LOG" "\$LOG.1"; fi
exec "$APP/.venv/bin/python" gateway.py --config "$APP/config.yaml"
EOF
chmod +x "$APP/run.sh"
[ "$OS" = Linux ] && chown "$RUN_AS": "$APP/run.sh"

if [ "$OS" = Darwin ]; then
  PLIST="$HOME/Library/LaunchAgents/com.butterbot.voicegateway.plist"
  mkdir -p "$(dirname "$PLIST")"
  cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>Label</key><string>com.butterbot.voicegateway</string>
  <key>ProgramArguments</key><array><string>$APP/run.sh</string></array>
  <key>RunAtLoad</key><true/>
  <key>KeepAlive</key><true/>
  <key>StandardOutPath</key><string>$APP/data/gateway.log</string>
  <key>StandardErrorPath</key><string>$APP/data/gateway.log</string>
</dict></plist>
EOF
  launchctl bootout "gui/$(id -u)" "$PLIST" 2>/dev/null || true
  launchctl bootstrap "gui/$(id -u)" "$PLIST"
  IP=$(ipconfig getifaddr en0 2>/dev/null || echo "<this Mac's IP>")
  echo
  echo "Gateway: ws://$IP:8765/bb   (allow incoming connections for Python if macOS asks)"
  echo "Logs:    tail -f $APP/data/gateway.log   (first start downloads the Whisper model)"
  exit 0
fi

cat > /etc/systemd/system/bb-voice-gateway.service <<EOF
[Unit]
Description=ButterBot voice gateway
After=network-online.target
Wants=network-online.target

[Service]
User=$RUN_AS
ExecStart=$APP/run.sh
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now bb-voice-gateway
systemctl restart bb-voice-gateway
sleep 3
systemctl --no-pager --lines=0 status bb-voice-gateway || true
echo
echo "Gateway: ws://$(hostname -I | awk '{print $1}'):8765/bb"
echo "Logs:    journalctl -u bb-voice-gateway -f   (first start downloads the Whisper model)"
