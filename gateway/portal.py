"""Robot portal logic (the page itself is camera.py + portal_page.py): VC/RC mode, one driver at a
time, the joystick, typed speech and chat listening.

Robot protocol (only for firmware that says "portal": true in its hello):
  gw    -> {"type":"mode","rc":bool}           enter / leave Remote Control
  gw    -> {"type":"drive","l":int,"r":int}    track speeds -100..100, about 10 a second while the
                                               stick is held; the robot stops by itself if they stop
  gw    -> {"type":"listen","chat":true}       one listen for the chat; the result is text only
  robot -> {"type":"status","mode":"vc"|"rc","speaking":bool,"listening":bool,"blocked":str}
  robot -> {"type":"start",...,"chat":true}    the chat listen's audio (transcribed, never dispatched)
  robot -> {"type":"battery","percent":int,"charging":"unplugged"|"charging"|"full","rssi":dBm}
                                               after each hello, every minute (5 s in RC), on plug in/out
"""
import asyncio
import logging
import math
import re
import time

log = logging.getLogger("gateway")

PORTAL_SAY_MAX = 300       # characters of typed chat the robot will say
PORTAL_SAY_GAP_S = 1.0     # per page: one typed message a second
DRIVER_GRACE_S = 30        # a driver whose pages are all closed this long hands back control (VC)
DRIVE_DEADZONE = 0.4      # radius around the centre (fraction of full travel) where the robot stops,
                           # so the stick can pass through the middle to change direction without letting go
AXIS_SNAP = 0.06           # a push within ~3.5° of an axis goes exactly along it

# What Whisper "hears" in silence or noise. Real speech is never exactly one of these.
PHANTOMS = {"thanks for watching", "thank you for watching", "thanks for watching and see you next time",
            "please subscribe", "subtitles by the amaraorg community", "you", "bye", "so"}


def mix(x: float, y: float) -> tuple[int, int]:
    """Stick position (x right, y forward, each -1..1) to track speeds -100..100.
    Inside the dead zone the robot stops. Beyond it, speed grows from zero at the dead zone's edge to
    full at the rim. Forward plus turn; scaled down together so a full diagonal keeps its direction."""
    x, y = max(-1.0, min(1.0, x)), max(-1.0, min(1.0, y))
    reach = math.hypot(x, y)
    if reach <= DRIVE_DEADZONE:
        return 0, 0
    gain = min(1.0, (reach - DRIVE_DEADZONE) / (1 - DRIVE_DEADZONE)) / reach
    x, y = x * gain, y * gain
    if abs(x) < AXIS_SNAP * abs(y):     # within ~3.5° of straight ahead/back: drive straight
        x = 0.0
    elif abs(y) < AXIS_SNAP * abs(x):   # within ~3.5° of sideways: spin in place
        y = 0.0
    left, right = y + x, y - x
    scale = max(1.0, abs(left), abs(right))
    return round(100 * left / scale), round(100 * right / scale)


def phantom(text: str) -> bool:
    words = re.sub(r"[^a-z ]", "", text.lower()).strip()
    return not words or words in PHANTOMS


class PortalMixin:
    """Mixed into Gateway: uses self.robots, self.push, self.camera, self.camera_command."""

    def portal_init(self):
        self.robot_portal: dict[str, bool] = {}       # device -> firmware has RC / chat listen
        self.robot_state: dict[str, dict] = {}        # device -> its last {"type":"status"}
        self.robot_battery: dict[str, dict] = {}      # device -> {percent, charging, at}
        self.driver: tuple[str, str] | None = None    # (session id, user) holding RC
        self.driver_timer: asyncio.Task | None = None
        self.chat_listener: str | None = None         # session that pressed Listen

    def portal_robot(self) -> str | None:
        return next(iter(self.robots), None)

    def portal_status(self, conn=None) -> dict:
        dev = self.portal_robot()
        state = self.robot_state.get(dev, {}) if dev else {}
        return {"robot": dev is not None,
                "camera_supported": bool(dev and self.robot_camera.get(dev)),
                "rc_supported": bool(dev and self.robot_portal.get(dev)),
                "listen_supported": bool(dev and self.robot_portal.get(dev)),
                "mode": state.get("mode", "vc"),
                "driver": self.driver[1] if self.driver else None,
                "driver_is_you": bool(conn and self.driver and self.driver[0] == conn.sid),
                "speaking": bool(state.get("speaking")), "listening": bool(state.get("listening")),
                "blocked": str(state.get("blocked") or ""),
                "deadzone": DRIVE_DEADZONE,
                "battery": self.robot_battery.get(dev) if dev else None}

    async def portal_message(self, conn, msg: dict):
        kind = msg.get("type")
        dev = self.portal_robot()
        if kind == "drive":
            return await self.portal_drive(conn, dev, msg)
        if dev is None:
            if kind in ("say", "camera", "mode", "listen"):
                await self.camera.chat(conn.sid, "note", "ButterBot isn't connected.")
            return
        if kind == "camera":
            on = bool(msg.get("on"))
            reply = await self.camera_command(dev, on)
            log.info("%s portal camera %s by %s -> %s", dev, "on" if on else "off", conn.user, reply)
            if not reply.startswith("Camera stream"):   # refused: say why
                await self.camera.chat(conn.sid, "note", reply)
        elif kind == "say":
            await self.portal_say(conn, dev, msg)
        elif kind == "mode":
            await self.portal_mode(conn, dev, msg.get("mode") == "rc")
        elif kind == "listen":
            await self.portal_listen(conn, dev)

    async def portal_say(self, conn, dev: str, msg: dict):
        text = " ".join(str(msg.get("text", "")).split())[:PORTAL_SAY_MAX]
        if not text:
            return
        now = time.monotonic()
        if now - conn.last_say < PORTAL_SAY_GAP_S:
            return await self.camera.chat(conn.sid, "note", "Wait a moment between messages.")
        conn.last_say = now
        await self.camera.chat(conn.sid, "you", text)
        if await self.push(dev, {"type": "say", "text": text}):
            log.info("%s portal say by %s: %s", dev, conn.user, text)
        else:
            await self.camera.chat(conn.sid, "note", "Couldn't reach ButterBot, so that wasn't said.")

    async def portal_mode(self, conn, dev: str, rc: bool):
        if not self.robot_portal.get(dev):
            return await self.camera.chat(conn.sid, "note", "Remote control needs a robot firmware update.")
        if rc:
            if self.driver and self.driver[0] != conn.sid:
                return await self.camera.chat(conn.sid, "note", f"{self.driver[1]} is driving right now.")
            self.driver = (conn.sid, conn.user)
        elif self.driver and self.driver[0] != conn.sid:
            return await self.camera.chat(conn.sid, "note", f"Only {self.driver[1]} can hand back control.")
        else:
            self.driver = None
        await self.push(dev, {"type": "mode", "rc": rc})
        log.info("%s portal %s by %s", dev, "RC" if rc else "VC", conn.user)
        await self.camera.broadcast_status()

    async def portal_drive(self, conn, dev: str | None, msg: dict):
        if dev is None or not self.driver or self.driver[0] != conn.sid:
            return
        if self.robot_state.get(dev, {}).get("mode") != "rc":
            return
        try:
            left, right = mix(float(msg.get("x", 0)), float(msg.get("y", 0)))
        except (TypeError, ValueError):
            return
        await self.push(dev, {"type": "drive", "l": left, "r": right})
        await conn.send({"type": "drive", "l": left, "r": right})   # the page shows the track speeds

    async def portal_listen(self, conn, dev: str):
        if not self.robot_portal.get(dev):
            return await self.camera.chat(conn.sid, "note", "Listening needs a robot firmware update.")
        state = self.robot_state.get(dev, {})
        if state.get("speaking") or state.get("listening"):
            return await self.camera.chat(conn.sid, "note", "ButterBot is busy; try again when it's quiet.")
        self.chat_listener = conn.sid
        if not await self.push(dev, {"type": "listen", "chat": True}):
            self.chat_listener = None
            await self.camera.chat(conn.sid, "note", "Couldn't reach ButterBot.")

    async def chat_heard(self, dev: str, text: str):
        """A chat listen's transcript: shown to the page that asked, never run as a command."""
        sid, self.chat_listener = self.chat_listener, None
        if sid is None:
            return
        if phantom(text):
            await self.camera.chat(sid, "note", "Didn't hear anything.")
        else:
            await self.camera.chat(sid, "heard", text)
        log.info("%s chat listen heard: %s", dev, text or "(nothing)")

    async def robot_status(self, dev: str, ev: dict):
        old = self.robot_state.get(dev, {})
        before = old.get("mode", "vc")
        self.robot_state[dev] = {k: ev.get(k) for k in ("mode", "speaking", "listening", "blocked")}
        if ev.get("mode") != before or (ev.get("blocked") or "") != (old.get("blocked") or ""):
            log.info("%s robot status: %s%s", dev, str(ev.get("mode", "?")).upper(),
                     f", can't move: {ev['blocked']}" if ev.get("blocked") else "")
        if ev.get("mode") != "rc" and self.driver:
            if before == "rc":
                log.info("%s back to VC; %s no longer driving", dev, self.driver[1])
            self.driver = None   # left RC on the robot (button, reboot) or never entered it
        await self.camera.broadcast_status()

    async def portal_signed_out(self, sid: str):
        """Signing out ends that session's driving at once: the robot goes back to voice control."""
        if self.chat_listener == sid:
            self.chat_listener = None
        if not self.driver or self.driver[0] != sid:
            return
        dev = self.portal_robot()
        log.info("%s driver %s signed out: back to VC", dev, self.driver[1])
        self.driver = None
        if dev:
            await self.push(dev, {"type": "mode", "rc": False})
        await self.camera.broadcast_status()

    async def robot_battery_report(self, dev: str, ev: dict):
        try:
            percent = max(0, min(100, int(ev.get("percent"))))
        except (TypeError, ValueError):
            return
        charging = ev.get("charging") if ev.get("charging") in ("unplugged", "charging", "full") else "unplugged"
        before = self.robot_battery.get(dev)
        try:
            rssi = int(ev.get("rssi") or 0)   # Wi-Fi signal at the robot, dBm (0 = unknown)
        except (TypeError, ValueError):
            rssi = 0
        self.robot_battery[dev] = {"percent": percent, "charging": charging, "rssi": rssi if -100 <= rssi < 0 else None,
                                   "at": int(time.time())}
        if not before or before["charging"] != charging or abs(before["percent"] - percent) >= 5:
            log.info("%s battery %d%% (%s)", dev, percent, charging)   # changes only, not every minute
        await self.camera.broadcast_status()

    async def portal_closed(self, conn):
        if not self.driver or self.driver[0] != conn.sid:
            return
        if any(c.sid == conn.sid for c in self.camera.conns):
            return                                   # the driver still has another page open
        if self.driver_timer:
            self.driver_timer.cancel()
        self.driver_timer = asyncio.create_task(self.driver_gone(conn.sid))

    async def driver_gone(self, sid: str):
        await asyncio.sleep(DRIVER_GRACE_S)
        if self.driver and self.driver[0] == sid and not any(c.sid == sid for c in self.camera.conns):
            dev = self.portal_robot()
            log.info("%s driver %s gone for %d s: back to VC", dev, self.driver[1], DRIVER_GRACE_S)
            self.driver = None
            if dev:
                await self.push(dev, {"type": "mode", "rc": False})
            await self.camera.broadcast_status()

    async def portal_robot_gone(self, dev: str):
        self.robot_portal.pop(dev, None)
        self.robot_state.pop(dev, None)
        self.robot_battery.pop(dev, None)
        self.driver = None          # the robot always comes back in VC
        self.chat_listener = None
