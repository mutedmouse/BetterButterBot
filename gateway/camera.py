"""Robot portal: ButterBot sends JPEG frames over its gateway socket while its camera stream is on;
this serves them to signed-in browsers on the LAN, with controls (camera on/off, chat; see portal_page.py).

    http://<gateway>:8766/              sign-in page, then the portal
    http://<gateway>:8766/stream        MJPEG (needs a signed-in session)
    http://<gateway>:8766/snapshot.jpg  latest frame (needs a signed-in session)
    ws://<gateway>:8766/ws              portal controls (signed-in session, same-origin pages only)

Accounts live in <data>/camera_users.json as salted scrypt hashes. Create one on the gateway machine
(the password is asked for, never passed on the command line):

    python camera.py adduser <name>          (Docker: docker compose exec gateway-cpu python camera.py adduser <name>)
    python camera.py deluser <name>
    python camera.py list

The robot only streams after being asked (the portal's CS switch, or by voice: "start the camera
stream") and keeps streaming until asked to stop. It is always off after the robot or gateway restarts,
or when the robot's connection drops. Plain HTTP sends the password unencrypted on your network;
set camera.tls_cert / camera.tls_key in config.yaml to serve HTTPS instead.
"""
import argparse
import asyncio
import getpass
import hashlib
import html
import json
import logging
import os
import secrets
import ssl
import sys
import time
from collections import defaultdict, deque
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

from websockets.exceptions import InvalidState, ProtocolError
from websockets.frames import Opcode
from websockets.server import ServerProtocol

import portal_page

log = logging.getLogger("camera")

USERS_FILE = "camera_users.json"
SESSION_S = 12 * 3600          # how long a sign-in lasts
MAX_FAILS = 5                  # failed sign-ins per address ...
FAIL_WINDOW_S = 300            # ... within this window lock the address out ...
LOCKOUT_S = 300                # ... for this long
OFF_GRACE_S = 300              # a viewer stays connected this long after the stream turns off or pauses
                               # (find me / face recognition pause it while they use the camera)
MIN_PASSWORD = 8
CHAT_KEEP = 200                # chat lines kept per signed-in session (cleared at sign-out / expiry)
WS_IDLE_S = 75                 # the page pings every 25 s; a silent socket this long is dead
WS_MAX_MESSAGE = 8192
SCRYPT = {"n": 2 ** 14, "r": 8, "p": 1}
COOKIE = "bbcam"

STYLE = """body{margin:0;background:#111;color:#ddd;font:15px system-ui,sans-serif;text-align:center}
form{display:inline-block;margin-top:14vh;padding:24px 28px;background:#1c1c1c;border-radius:10px;text-align:left}
input{display:block;width:240px;margin:6px 0 14px;padding:8px;border:1px solid #444;border-radius:6px;background:#111;color:#eee}
button{padding:8px 16px;border:0;border-radius:6px;background:#d9a400;color:#111;font-weight:600;cursor:pointer}
.err{color:#f77;margin:0 0 10px}.bar{display:flex;justify-content:space-between;align-items:center;padding:8px 12px}
.bar form{margin:0;padding:0;background:none}
img{display:block;margin:0 auto;width:min(100vw,640px);aspect-ratio:4/3;object-fit:contain;image-rendering:auto;background:#000}"""

LOGIN_PAGE = """<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ButterBot camera</title><style>{style}</style></head><body>
<form method="post" action="/login"><h2 style="margin-top:0">ButterBot camera</h2>{error}
<label>User name<input name="user" autocomplete="username" autofocus required></label>
<label>Password<input name="password" type="password" autocomplete="current-password" required></label>
<button type="submit">Sign in</button></form></body></html>"""

NO_USERS_PAGE = """<!doctype html><html><head><meta charset="utf-8"><title>ButterBot camera</title><style>{style}</style></head>
<body><p style="margin-top:20vh">No camera accounts yet. On the gateway machine run<br><br>
<code>python camera.py adduser &lt;name&gt;</code></p></body></html>"""

SECURITY_HEADERS = ("X-Frame-Options: DENY\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"
                    "Content-Security-Policy: default-src 'none'; img-src 'self'; style-src 'self' 'unsafe-inline'; "
                    "script-src 'self'; connect-src 'self'; form-action 'self'; frame-ancestors 'none'\r\n")


# ---- accounts -------------------------------------------------------------------------------

def _hash(password: str, salt: bytes) -> bytes:
    return hashlib.scrypt(password.encode(), salt=salt, n=SCRYPT["n"], r=SCRYPT["r"], p=SCRYPT["p"], dklen=32)


class Users:
    def __init__(self, path: Path):
        self.path = path

    def load(self) -> dict:
        try:
            return json.loads(self.path.read_text(encoding="utf-8")).get("users", {})
        except (OSError, ValueError):
            return {}

    def save(self, users: dict):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        tmp = self.path.with_suffix(".tmp")
        tmp.write_text(json.dumps({"users": users}, indent=1), encoding="utf-8")
        try:
            os.chmod(tmp, 0o600)
        except OSError:
            pass
        tmp.replace(self.path)

    def set(self, name: str, password: str):
        users = self.load()
        salt = secrets.token_bytes(16)
        users[name] = {"salt": salt.hex(), "hash": _hash(password, salt).hex()}
        self.save(users)

    def delete(self, name: str) -> bool:
        users = self.load()
        if users.pop(name, None) is None:
            return False
        self.save(users)
        return True

    def check(self, name: str, password: str) -> bool:
        entry = self.load().get(name)
        # Hash even for unknown names, so timing doesn't reveal which names exist.
        salt = bytes.fromhex(entry["salt"]) if entry else b"\0" * 16
        digest = _hash(password, salt)
        return entry is not None and secrets.compare_digest(digest.hex(), entry["hash"])


# ---- viewer server --------------------------------------------------------------------------

class PortalConn:
    """One portal page's WebSocket (framing by the websockets library's sans-I/O protocol)."""

    def __init__(self, proto: ServerProtocol, writer: asyncio.StreamWriter, sid: str, user: str):
        self.proto = proto
        self.writer = writer
        self.sid = sid
        self.user = user
        self.lock = asyncio.Lock()
        self.last_say = 0.0

    async def flush(self):
        for chunk in self.proto.data_to_send():
            if chunk:
                self.writer.write(chunk)
            else:                       # the protocol wants the TCP connection half-closed
                self.writer.close()
        await self.writer.drain()

    async def send(self, message: dict):
        async with self.lock:
            try:
                self.proto.send_text(json.dumps(message).encode())
                await self.flush()
            except (InvalidState, ConnectionError, RuntimeError):
                pass                    # closing; the reader loop cleans up


class CameraRelay:
    def __init__(self, data_dir: Path, port: int = 8766, host: str = "0.0.0.0",
                 tls_cert: str = "", tls_key: str = "", trusted_proxies=("127.0.0.1", "::1")):
        self.users = Users(Path(data_dir) / USERS_FILE)
        self.port = port
        self.host = host
        # Reverse proxies on this machine whose X-Forwarded-For / X-Forwarded-Proto are believed. Everyone
        # else is judged by their own address, so those headers can't be spoofed.
        self.trusted_proxies = set(trusted_proxies or ())
        self.tls = None
        if tls_cert and tls_key:
            self.tls = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            self.tls.load_cert_chain(tls_cert, tls_key)
        self.sessions: dict[str, tuple[str, float]] = {}   # session id -> (user, expires)
        self.fails: dict[str, deque] = defaultdict(deque)  # address -> failed sign-in times
        self.locked: dict[str, float] = {}                 # address -> locked until
        self.frame: bytes | None = None
        self.frame_no = 0
        self.on = False
        self.mode = ""
        self.off_since = time.time()
        self.cond = asyncio.Condition()
        self.viewers = 0
        self.server = None
        self.wanted = False            # the stream was asked for (paused counts as wanted)
        self.conns: set[PortalConn] = set()
        self.chats: dict[str, deque] = {}   # session id -> chat lines
        # The gateway: portal_status(conn) -> dict, async portal_message(conn, msg), async portal_closed(conn),
        # async portal_signed_out(sid).
        self.portal = None

    @property
    def enabled(self) -> bool:
        """At least one account exists (checked live, so adding one needs no restart)."""
        return bool(self.users.load())

    def describe(self) -> str:
        scheme = "https" if self.tls else "http"
        n = len(self.users.load())
        return (f"{scheme} port {self.port}, {n} account(s)" if n
                else f"{scheme} port {self.port}, no accounts yet (python camera.py adduser <name>)")

    async def start(self):
        self.server = await asyncio.start_server(self.handle, self.host, self.port, ssl=self.tls)

    async def publish(self, jpeg: bytes):
        async with self.cond:
            self.frame = jpeg
            self.frame_no += 1
            self.cond.notify_all()

    async def set_on(self, on: bool, mode: str = ""):
        async with self.cond:
            self.on = on
            self.mode = mode if on else ""
            if not on:
                self.off_since = time.time()
            self.cond.notify_all()
        await self.broadcast_status()

    # -- portal pages --

    async def broadcast(self, message: dict):
        for conn in list(self.conns):
            await conn.send(message)

    async def broadcast_status(self):
        for conn in list(self.conns):   # per page: "driver_is_you" differs
            await conn.send(self.status(conn))

    def status(self, conn: "PortalConn | None" = None) -> dict:
        s = {"type": "status", "camera": {"wanted": self.wanted, "on": self.on, "mode": self.mode}}
        if self.portal is not None:
            s.update(self.portal.portal_status(conn))
        return s

    async def chat(self, sid: str, who: str, text: str):
        """Add a line to one session's chat ("you", "heard" or "note") and show it on its pages."""
        line = {"who": who, "text": text, "ts": int(time.time())}
        self.chats.setdefault(sid, deque(maxlen=CHAT_KEEP)).append(line)
        for conn in list(self.conns):
            if conn.sid == sid:
                await conn.send({"type": "chat", **line})

    def session_valid(self, sid: str) -> bool:
        user, expires = self.sessions.get(sid, (None, 0))
        return bool(user) and time.time() < expires and user in self.users.load()

    async def end_session(self, sid: str):
        if self.portal is not None and sid in self.sessions:
            await self.portal.portal_signed_out(sid)   # e.g. the driver signing out hands back control
        self.sessions.pop(sid, None)
        self.chats.pop(sid, None)
        for conn in list(self.conns):
            if conn.sid == sid:
                try:
                    conn.writer.close()
                except Exception:
                    pass

    # -- sessions and sign-in --

    def session_id(self, headers: dict) -> str | None:
        for part in headers.get("cookie", "").split(";"):
            name, _, value = part.strip().partition("=")
            if name == COOKIE and value:
                if self.session_valid(value):
                    return value
                self.sessions.pop(value, None)
                self.chats.pop(value, None)
        return None

    def session_user(self, headers: dict) -> str | None:
        sid = self.session_id(headers)
        return self.sessions[sid][0] if sid else None

    def locked_out(self, addr: str) -> bool:
        return self.locked.get(addr, 0) > time.time()

    def record_fail(self, addr: str):
        now = time.time()
        q = self.fails[addr]
        q.append(now)
        while q and now - q[0] > FAIL_WINDOW_S:
            q.popleft()
        if len(q) >= MAX_FAILS:
            self.locked[addr] = now + LOCKOUT_S
            q.clear()
            log.warning("camera: %s locked out for %d s after %d failed sign-ins", addr, LOCKOUT_S, MAX_FAILS)

    def cookie(self, value: str, max_age: int, https: bool = False) -> str:
        secure = "; Secure" if self.tls or https else ""
        return f"Set-Cookie: {COOKIE}={value}; Path=/; Max-Age={max_age}; HttpOnly; SameSite=Strict{secure}\r\n"

    def client(self, peer: str, headers: dict) -> tuple[str, bool]:
        """(the real client address, reached over https) - through a trusted proxy, from its headers.
        Behind a proxy every request comes from the proxy's address: sign-in lockouts must count the
        real visitor, or one stranger's wrong passwords would lock everyone out."""
        if peer in self.trusted_proxies:
            real = headers.get("x-forwarded-for", "").split(",")[0].strip()
            https = headers.get("x-forwarded-proto", "").lower() == "https"
            return (real or peer), (https or bool(self.tls))
        return peer, bool(self.tls)

    # -- HTTP --

    async def handle(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        addr = (writer.get_extra_info("peername") or ("?",))[0]
        try:
            request = await asyncio.wait_for(reader.readline(), 10)
            headers = {}
            while True:
                line = await asyncio.wait_for(reader.readline(), 10)
                if line in (b"\r\n", b"\n", b""):
                    break
                name, _, value = line.decode("latin-1").partition(":")
                headers[name.strip().lower()] = value.strip()
                if len(headers) > 64:
                    return await self.reply(writer, 400, "text/plain", b"Bad request\n")
            parts = request.decode("latin-1").split()
            if len(parts) < 2:
                return await self.reply(writer, 400, "text/plain", b"Bad request\n")
            method, path = parts[0], urlsplit(parts[1]).path
            addr, https = self.client(addr, headers)
            sid = self.session_id(headers)
            user = self.sessions[sid][0] if sid else None

            if method == "POST" and path == "/login":
                return await self.login(reader, writer, headers, addr, https)
            if method == "POST" and path == "/logout":
                for part in headers.get("cookie", "").split(";"):
                    n, _, v = part.strip().partition("=")
                    if n == COOKIE:
                        await self.end_session(v)
                return await self.redirect(writer, "/", self.cookie("", 0, https))
            if method != "GET":
                return await self.reply(writer, 405, "text/plain", b"Method not allowed\n")
            if path == "/":
                if not self.enabled:
                    return await self.reply(writer, 200, "text/html; charset=utf-8",
                                            NO_USERS_PAGE.format(style=STYLE).encode())
                if user is None:
                    return await self.login_page(writer)
                return await self.reply(writer, 200, "text/html; charset=utf-8",
                                        portal_page.PAGE.format(user=html.escape(user)).encode())
            if path in ("/app.js", "/app.css"):
                if user is None:
                    return await self.reply(writer, 401, "text/plain", b"Sign in first\n")
                if path == "/app.js":
                    return await self.reply(writer, 200, "text/javascript; charset=utf-8", portal_page.SCRIPT.encode())
                return await self.reply(writer, 200, "text/css; charset=utf-8", portal_page.STYLE.encode())
            if path == "/ws":
                if user is None:
                    return await self.reply(writer, 401, "text/plain", b"Sign in first\n")
                if not self.same_origin(headers):
                    log.warning("camera: refused portal socket from %s (origin %r)", addr, headers.get("origin"))
                    return await self.reply(writer, 403, "text/plain", b"Forbidden\n")
                return await self.websocket(reader, writer, request, headers, sid, user, addr)
            if path in ("/stream", "/snapshot.jpg"):
                if user is None:
                    return await self.reply(writer, 401, "text/plain", b"Sign in first\n")
                if path == "/stream":
                    return await self.stream(reader, writer, addr, user)
                if self.frame is None:
                    return await self.reply(writer, 503, "text/plain", b"No frame yet: the camera stream is off\n")
                return await self.reply(writer, 200, "image/jpeg", self.frame)
            return await self.reply(writer, 404, "text/plain", b"Not found\n")
        except (asyncio.TimeoutError, ConnectionError, asyncio.IncompleteReadError, ssl.SSLError):
            pass
        finally:
            try:
                writer.close()
            except Exception:
                pass

    async def login_page(self, writer, error: str = "", status: int = 200):
        err = f'<p class="err">{html.escape(error)}</p>' if error else ""
        await self.reply(writer, status, "text/html; charset=utf-8",
                         LOGIN_PAGE.format(style=STYLE, error=err).encode())

    async def login(self, reader, writer, headers: dict, addr: str, https: bool = False):
        if self.locked_out(addr):
            return await self.login_page(writer, "Too many failed sign-ins. Try again in a few minutes.", 429)
        length = int(headers.get("content-length") or 0)
        if length <= 0 or length > 4096:
            return await self.login_page(writer, "Sign-in failed.", 400)
        form = parse_qs((await asyncio.wait_for(reader.readexactly(length), 10)).decode("utf-8", "replace"))
        name = (form.get("user") or [""])[0].strip()
        password = (form.get("password") or [""])[0]
        # scrypt is deliberately slow (~50 ms); keep it off the event loop.
        ok = await asyncio.to_thread(self.users.check, name, password)
        if not ok:
            self.record_fail(addr)
            log.warning("camera: failed sign-in for %r from %s", name, addr)
            return await self.login_page(writer, "Wrong user name or password.", 401)
        self.fails.pop(addr, None)
        now = time.time()
        self.sessions = {k: v for k, v in self.sessions.items() if v[1] > now}   # drop expired
        self.chats = {k: v for k, v in self.chats.items() if k in self.sessions}
        sid = secrets.token_urlsafe(32)
        self.sessions[sid] = (name, now + SESSION_S)
        log.info("camera: %s signed in from %s", name, addr)
        await self.redirect(writer, "/", self.cookie(sid, SESSION_S, https))

    async def redirect(self, writer, location: str, extra: str = ""):
        writer.write(f"HTTP/1.1 303 See Other\r\nLocation: {location}\r\n{extra}{SECURITY_HEADERS}"
                     "Content-Length: 0\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n".encode())
        await writer.drain()

    async def reply(self, writer, status: int, ctype: str, body: bytes):
        reason = {200: "OK", 400: "Bad Request", 401: "Unauthorized", 403: "Forbidden", 404: "Not Found",
                  405: "Method Not Allowed", 429: "Too Many Requests", 503: "Service Unavailable"}[status]
        writer.write(f"HTTP/1.1 {status} {reason}\r\nContent-Type: {ctype}\r\nContent-Length: {len(body)}\r\n"
                     f"{SECURITY_HEADERS}Cache-Control: no-store\r\nConnection: close\r\n\r\n".encode() + body)
        await writer.drain()

    async def stream(self, reader, writer, addr: str, user: str):
        writer.write(b"HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                     + SECURITY_HEADERS.encode() + b"Cache-Control: no-store\r\nConnection: close\r\n\r\n")
        await writer.drain()
        self.viewers += 1
        log.info("camera: %s watching from %s (%d viewer(s))", user, addr, self.viewers)
        seen = -1
        # Writes to a closed connection can succeed for a while; the viewer leaving shows up as EOF.
        gone = asyncio.create_task(reader.read(1))
        try:
            while not gone.done():
                async with self.cond:
                    try:
                        await asyncio.wait_for(self.cond.wait_for(lambda: self.frame_no != seen or not self.on), 2)
                    except asyncio.TimeoutError:
                        pass
                    frame, seen, on = self.frame, self.frame_no, self.on
                if not on and time.time() - self.off_since > OFF_GRACE_S:
                    break
                if frame is not None:
                    writer.write(b"--frame\r\nContent-Type: image/jpeg\r\nContent-Length: "
                                 + str(len(frame)).encode() + b"\r\n\r\n" + frame + b"\r\n")
                    await writer.drain()
                if not on:
                    await asyncio.sleep(1)   # off: keep showing the last frame until the grace ends
        finally:
            gone.cancel()
            self.viewers -= 1
            log.info("camera: %s stopped watching (%d viewer(s))", user, self.viewers)

    def same_origin(self, headers: dict) -> bool:
        """Only this server's own page may open the portal socket (cookies alone don't stop other sites)."""
        # Either scheme: behind an https reverse proxy the page is https while this server speaks plain http.
        # What matters against other sites is the host name.
        host = headers.get("host")
        return bool(host) and headers.get("origin") in (f"http://{host}", f"https://{host}")

    async def websocket(self, reader, writer, request_line: bytes, headers: dict, sid: str, user: str, addr: str):
        proto = ServerProtocol(max_size=WS_MAX_MESSAGE)
        # The request was already parsed here; hand the library an equivalent copy for the handshake.
        raw = request_line + b"".join(f"{k}: {v}\r\n".encode("latin-1") for k, v in headers.items()) + b"\r\n"
        proto.receive_data(raw)
        events = proto.events_received()
        if not events:
            return await self.reply(writer, 400, "text/plain", b"Bad request\n")
        response = proto.accept(events[0])
        proto.send_response(response)
        conn = PortalConn(proto, writer, sid, user)
        await conn.flush()
        if response.status_code != 101:
            return
        self.conns.add(conn)
        log.info("camera: %s opened the portal from %s (%d open)", user, addr, len(self.conns))
        try:
            await conn.send({"type": "history", "lines": list(self.chats.get(sid, ()))})
            await conn.send(self.status(conn))
            while True:
                data = await asyncio.wait_for(reader.read(4096), WS_IDLE_S)
                if data:
                    proto.receive_data(data)
                else:
                    proto.receive_eof()
                for frame in proto.events_received():
                    if frame.opcode != Opcode.TEXT:
                        continue        # pings are answered by the protocol; close ends the loop below
                    if not self.session_valid(sid):
                        proto.send_close(1008, "signed out")
                        break
                    try:
                        msg = json.loads(frame.data)
                    except ValueError:
                        continue
                    if isinstance(msg, dict) and msg.get("type") != "ping" and self.portal is not None:
                        await self.portal.portal_message(conn, msg)
                async with conn.lock:
                    await conn.flush()
                if proto.close_expected() or not data:
                    break
        except (asyncio.TimeoutError, ConnectionError, ProtocolError, InvalidState):
            pass
        finally:
            self.conns.discard(conn)
            log.info("camera: %s closed the portal (%d open)", user, len(self.conns))
            if self.portal is not None:
                await self.portal.portal_closed(conn)


# ---- account management CLI ----------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description="Manage ButterBot camera viewer accounts.")
    p.add_argument("--data", default=os.environ.get("BB_DATA_DIR") or str(Path(__file__).parent / "data"),
                   help="gateway data folder (default: $BB_DATA_DIR or ./data)")
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("adduser", help="create an account or change its password").add_argument("name")
    sub.add_parser("deluser", help="remove an account").add_argument("name")
    sub.add_parser("list", help="list accounts")
    a = p.parse_args()
    users = Users(Path(a.data) / USERS_FILE)

    if a.cmd == "list":
        names = sorted(users.load())
        print("\n".join(names) if names else "no accounts")
    elif a.cmd == "deluser":
        print(f"removed {a.name}" if users.delete(a.name) else f"no account named {a.name}")
    else:
        if not a.name.strip() or any(c in a.name for c in "\r\n;=,"):
            sys.exit("invalid user name")
        pw = getpass.getpass(f"Password for {a.name} (at least {MIN_PASSWORD} characters): ")
        if len(pw) < MIN_PASSWORD:
            sys.exit("too short")
        if getpass.getpass("Again: ") != pw:
            sys.exit("passwords don't match")
        users.set(a.name, pw)
        print(f"saved {a.name} in {users.path}. Signed-in sessions start working immediately; no restart needed.")


if __name__ == "__main__":
    main()
