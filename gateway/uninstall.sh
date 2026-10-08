#!/bin/bash
# Remove a native bb-voice-gateway install (Linux systemd service / macOS login agent).
# Keeps config.yaml, secret.env, admin.env and data/ (keys, calendar login, transcripts, models)
# unless you pass --purge.
#
#   Linux:  sudo ./uninstall.sh [--purge]        macOS:  ./uninstall.sh [--purge]
#   Same APP=/path override as install.sh.
set -euo pipefail

PURGE=0
[ "${1:-}" = "--purge" ] && PURGE=1

if [ "$(uname -s)" = Darwin ]; then
  APP=${APP:-$HOME/bb-voice-gateway}
  PLIST="$HOME/Library/LaunchAgents/com.butterbot.voicegateway.plist"
  launchctl bootout "gui/$(id -u)" "$PLIST" 2>/dev/null || true
  rm -f "$PLIST"
else
  [ "$(id -u)" -eq 0 ] || { echo "Run with sudo."; exit 1; }
  APP=${APP:-/opt/bb-voice-gateway}
  systemctl disable --now bb-voice-gateway 2>/dev/null || true
  rm -f /etc/systemd/system/bb-voice-gateway.service
  systemctl daemon-reload
fi
echo "== service removed"

if [ ! -d "$APP" ]; then
  echo "Nothing installed at $APP."
  exit 0
fi
# Refuse to delete anything that isn't a gateway install (guards against a wrong APP=...): either the
# full install, or what an earlier uninstall left behind (config, keys, data only).
full=0; [ -f "$APP/gateway.py" ] && [ -f "$APP/run.sh" ] && full=1
others=$(find "$APP" -mindepth 1 -maxdepth 1 ! -name config.yaml ! -name secret.env ! -name admin.env ! -name data | head -1)
leftovers=0; [ -z "$others" ] && { [ -f "$APP/config.yaml" ] || [ -d "$APP/data" ]; } && leftovers=1
if [ "$full" = 0 ] && [ "$leftovers" = 0 ]; then
  echo "$APP doesn't look like a bb-voice-gateway install - not touching it."; exit 1
fi

if [ "$PURGE" = 1 ]; then
  rm -rf "$APP"
  echo "== removed $APP, including config, keys, calendar login and data"
else
  find "$APP" -mindepth 1 -maxdepth 1 \
       ! -name config.yaml ! -name secret.env ! -name admin.env ! -name data -exec rm -rf {} +
  echo "== removed the program files; kept $APP/{config.yaml,secret.env,data/}"
  echo "   (re-running install.sh picks them up again; ./uninstall.sh --purge deletes them)"
fi
