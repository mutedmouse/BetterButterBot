#!/usr/bin/env bash
# Renew ButterBot's Google Calendar login (needed every 7 days while the OAuth app is in "Testing").
# Opens the browser sign-in, puts the new token where the gateway reads it, removes the local copy.
#   ./calendar_reauth.sh user@192.168.1.50      # gateway on another machine (native Linux install)
#   ./calendar_reauth.sh local                  # gateway on this machine (Docker or native): ./data
#   READ_ONLY=1 ./calendar_reauth.sh local      # read-only access
# Host may also come from BB_GATEWAY_SSH; REMOTE_DIR overrides /opt/bb-voice-gateway/data.
set -euo pipefail
cd "$(dirname "$0")"
HOST="${1:-${BB_GATEWAY_SSH:-}}"
REMOTE_DIR="${REMOTE_DIR:-/opt/bb-voice-gateway/data}"
CLIENT="${CLIENT:-secrets/google_client.json}"
[ -n "$HOST" ] || { echo "usage: $0 user@<gateway-ip> | local" >&2; exit 2; }
[ -f "$CLIENT" ] || { echo "OAuth client JSON not found: $CLIENT (see docs/INSTALL.md)" >&2; exit 2; }
PY=python3; [ -x .venv/bin/python ] && PY=.venv/bin/python
WRITE=--write; [ -n "${READ_ONLY:-}" ] && WRITE=
$PY -u calendar_auth.py $WRITE "$CLIENT" secrets/google_token.json
if [ "$HOST" = local ]; then
  mkdir -p data && mv -f secrets/google_token.json data/google_token.json && chmod 600 data/google_token.json
  echo "Calendar login renewed in ./data (no restart needed)."
else
  scp -q secrets/google_token.json "$HOST:/tmp/google_token.json"
  ssh "$HOST" "install -m 600 /tmp/google_token.json $REMOTE_DIR/google_token.json && rm -f /tmp/google_token.json"
  rm -f secrets/google_token.json
  echo "Calendar login renewed on $HOST (no restart needed)."
fi
