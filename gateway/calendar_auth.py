"""One-time Google Calendar sign-in for the gateway.

Run this on any computer with a web browser - not necessarily the gateway:

    python calendar_auth.py [--write] path/to/client_secret.json [data/google_token.json]

Without --write it asks for read-only access; with --write it asks to see and edit events (not
calendars or sharing), so ButterBot can add events and change the ones it added. It opens
Google's consent page, you approve, and it writes a token
file (client id/secret + refresh token, private). Copy that file to the gateway's data
directory (Docker: ./data/, VM: /opt/bb-voice-gateway/data/) and set calendar.provider: google.

The OAuth client should be a "Desktop app" client (any loopback port works). A "Web application"
client works too if http://localhost:8085/ is listed in its Authorized redirect URIs.
Standard library only.
"""
import base64
import hashlib
import http.server
import json
import os
import secrets
import socket
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import webbrowser
from pathlib import Path

READ_SCOPE = "https://www.googleapis.com/auth/calendar.readonly"
WRITE_SCOPE = "https://www.googleapis.com/auth/calendar.events"
AUTH_URI = "https://accounts.google.com/o/oauth2/v2/auth"
TOKEN_URI = "https://oauth2.googleapis.com/token"
WEB_PORT = 8085


def free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def main():
    argv = [a for a in sys.argv[1:] if a != "--write"]
    write = "--write" in sys.argv
    if not argv:
        sys.exit(__doc__)
    # calendar.events alone can't list calendars (calendarList needs a calendar-level scope), so
    # write access asks for both: read every calendar + edit events.
    scope = f"{READ_SCOPE} {WRITE_SCOPE}" if write else READ_SCOPE
    client_path = Path(argv[0])
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).parent / "data" / "google_token.json"

    raw = json.loads(client_path.read_text())
    kind = "installed" if "installed" in raw else "web" if "web" in raw else None
    if kind is None:
        sys.exit("That file isn't an OAuth client JSON (no 'installed' or 'web' section).")
    client = raw[kind]
    if kind == "web":
        port = WEB_PORT
        redirect = f"http://localhost:{port}/"
        if redirect not in client.get("redirect_uris", []):
            print(f"Note: this is a Web client. If Google says redirect_uri_mismatch, add\n  {redirect}\n"
                  "to the client's Authorized redirect URIs in Cloud Console, wait a minute, and rerun.\n")
    else:
        port = free_port()
        redirect = f"http://127.0.0.1:{port}/"

    verifier = secrets.token_urlsafe(64)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b"=").decode()
    state = secrets.token_urlsafe(16)
    url = AUTH_URI + "?" + urllib.parse.urlencode({
        "client_id": client["client_id"], "redirect_uri": redirect, "response_type": "code",
        "scope": scope, "access_type": "offline", "prompt": "consent", "state": state,
        "code_challenge": challenge, "code_challenge_method": "S256"})

    result = {}

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            if "code" not in q and "error" not in q:
                self.send_response(404)
                self.end_headers()
                return
            result.update({k: v[0] for k, v in q.items()})
            ok = "code" in q and q.get("state", [""])[0] == state
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(b"<h2>ButterBot: calendar access " + (b"granted" if ok else b"NOT granted") +
                             b".</h2>You can close this tab.")

        def log_message(self, *args):
            pass

    server = http.server.HTTPServer(("127.0.0.1" if kind == "installed" else "localhost", port), Handler)
    print(f"Opening your browser for Google sign-in ({'see and edit events' if write else 'read-only'})...")
    print(f"If it doesn't open, visit:\n{url}\n")
    webbrowser.open(url)
    while not result:
        server.handle_request()

    if result.get("state") != state or "code" not in result:
        sys.exit(f"Sign-in failed: {result.get('error', 'state mismatch')}")

    body = urllib.parse.urlencode({
        "code": result["code"], "client_id": client["client_id"], "client_secret": client.get("client_secret", ""),
        "redirect_uri": redirect, "grant_type": "authorization_code", "code_verifier": verifier}).encode()
    with urllib.request.urlopen(urllib.request.Request(TOKEN_URI, data=body)) as r:
        tok = json.load(r)
    if "refresh_token" not in tok:
        sys.exit("Google returned no refresh token. Remove ButterBot's access at "
                 "https://myaccount.google.com/permissions and run this again.")

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({"client_id": client["client_id"], "client_secret": client.get("client_secret", ""),
                               "refresh_token": tok["refresh_token"], "token_uri": TOKEN_URI,
                               # what Google actually granted (the user can untick boxes)
                               "scope": tok.get("scope", scope),
                               # when this sign-in happened: the gateway warns before a 7-day "Testing" token runs out
                               "issued_at": int(time.time())}, indent=2))
    if write and WRITE_SCOPE not in tok.get("scope", "").split():
        print("\nNote: edit access was not granted (box unticked?), so this token is read-only.")
    if os.name == "posix":
        os.chmod(out, 0o600)

    print(f"\nSaved {out}")
    req = urllib.request.Request("https://www.googleapis.com/calendar/v3/users/me/calendarList?minAccessRole=reader",
                                 headers={"Authorization": f"Bearer {tok['access_token']}"})
    try:
        with urllib.request.urlopen(req) as r:
            items = json.load(r).get("items", [])
    except urllib.error.HTTPError as e:
        msg = json.load(e).get("error", {}).get("message", str(e))
        sys.exit(f"Signed in, but listing calendars failed: {msg}\n"
                 "(Calendar API not enabled in the Cloud project, or the read permission was unticked.)")
    print(f"Calendars this account can read ({len(items)}):")
    for c in items:
        mark = "*" if c.get("selected") else " "
        print(f"  {mark} {c.get('summaryOverride') or c.get('summary')}   [{c['id']}]")
    print("\n* = shown in your Google Calendar; the gateway reads those by default.")


if __name__ == "__main__":
    main()
