"""Background agent: things ButterBot does without being asked right now.

Timers, reminders and alarms live in SQLite (they survive gateway restarts). A one-second loop
fires due items by pushing them to the robot: alarms as {"type":"alarm"} (it sings the butter
song), timers as {"type":"alarm","song":"timer"} (the toast song; the gateway turns this into plain
speech for firmware without that song), reminders as {"type":"say"}. If the robot is offline the
push waits until it reconnects. The LLM can create and manage
them through tool calls (see tools() / run_tool), read the owner's calendar when one is configured,
and anything on the LAN holding the admin token can push speech or a scenario directly.
"""
import asyncio
import datetime as dt
import json
import logging
import re
import sqlite3
import time
from zoneinfo import ZoneInfo

from pathlib import Path

from calendar_feed import make_calendar, spoken_clock, spoken_day

log = logging.getLogger("agent")

ALARM_STALE_S = 600   # an alarm the robot couldn't take within 10 minutes is dropped, not sung late;
                      # a timer that late is just spoken

TOOLS = [
    {"type": "function", "function": {
        "name": "set_timer",
        "description": "Start a countdown timer. When it ends ButterBot sings its toast song and "
                       "announces the timer until the user presses its button.",
        "parameters": {"type": "object", "properties": {
            "seconds": {"type": "integer", "description": "Duration in seconds"},
            "label": {"type": "string", "description": "Short name, e.g. 'pasta'"}},
            "required": ["seconds"]}}},
    {"type": "function", "function": {
        "name": "set_alarm",
        "description": "Set an alarm for a clock time. When it goes off ButterBot sings its butter "
                       "song and announces the alarm until the user presses its button. Use this "
                       "for 'set an alarm' / 'wake me up at'.",
        "parameters": {"type": "object", "properties": {
            "time": {"type": "string", "description": "Local time: 'HH:MM' (24-hour; the next "
                                                      "time it occurs) or a full ISO 8601 date-time"},
            "label": {"type": "string", "description": "Optional reason, e.g. 'dentist'"}},
            "required": ["time"]}}},
    {"type": "function", "function": {
        "name": "set_reminder",
        "description": "Remind the user of something at a local date/time (ISO 8601, e.g. 2026-10-02T17:30).",
        "parameters": {"type": "object", "properties": {
            "when": {"type": "string", "description": "Local time, ISO 8601"},
            "text": {"type": "string", "description": "What to say when it is due"}},
            "required": ["when", "text"]}}},
    {"type": "function", "function": {
        "name": "list_reminders",
        "description": "List pending timers, alarms and reminders.",
        "parameters": {"type": "object", "properties": {}}}},
    {"type": "function", "function": {
        "name": "cancel_reminder",
        "description": "Cancel a pending timer, alarm or reminder by its id (from list_reminders).",
        "parameters": {"type": "object", "properties": {"id": {"type": "integer"}},
                       "required": ["id"]}}},
]

# Things the robot can physically do that the LLM may start. Built-in commands run by their
# activation phrase (looked up in Scenarios.h); "routine" ones have no voice command of their own.
ROBOT_ACTIONS = {
    "find_me": ("routine", "summon"),                 # the controller's Summon: camera face search, drives to you
    "what_do_you_see": ("phrase", "what do you see"),   # camera object recognition
    "who_is_this": ("phrase", "who is this"),           # camera face recognition (owner or stranger)
    "remember_my_face": ("phrase", "remember this face"),
    "forget_my_face": ("phrase", "forget my face"),
    "dance": ("phrase", "do a dance"),
    "camera_stream_on": ("camera", True),    # live view for the owner, relayed by the gateway
    "camera_stream_off": ("camera", False),
}
ROBOT_TOOL = {"type": "function", "function": {
    "name": "robot_action",
    "description": "Make ButterBot physically do something with its camera or wheels. It starts "
                   "right after your spoken reply. find_me: look around with the camera for a "
                   "face and drive to the person ('come here', 'find me', 'where am I'). "
                   "what_do_you_see: look with the camera and say what object is in front of it. "
                   "who_is_this: recognise the face in front of the camera. remember_my_face / "
                   "forget_my_face: save or clear the owner's face. dance: do a dance. "
                   "camera_stream_on / camera_stream_off: start or stop the live camera view the "
                   "owner can watch in a browser on the home network (it stops by itself after a while).",
    "parameters": {"type": "object", "properties": {
        "action": {"type": "string", "enum": list(ROBOT_ACTIONS)}},
        "required": ["action"]}}}

CALENDAR_TOOL = {"type": "function", "function": {
    "name": "get_calendar_events",
    "description": "Read the user's calendar. Use for questions about their day, schedule, meetings or "
                   "appointments. If the user names a calendar, pass it so only that one is read.",
    "parameters": {"type": "object", "properties": {
        "date": {"type": "string", "description": "First day, YYYY-MM-DD (default today)"},
        "days": {"type": "integer", "description": "Number of days to include (default 1, max 14)"},
        "calendar": {"type": "string", "description": "Only this calendar, as the user said it (spelling "
                                                      "may be off, e.g. 'why mca' for YMCA; 'my calendar' = "
                                                      "their own). Omit for all calendars."}},
        "required": []}}}

# Calendar changes are two-step: these tools only draft the change and return a sentence to read
# back; nothing is written until the user says yes (confirm_calendar_change, or a plain "yes" that
# the gateway handles directly). Moving/removing only works on events ButterBot created (their
# "ref" from get_calendar_events).
CALENDAR_WRITE_TOOLS = [
    {"type": "function", "function": {
        "name": "add_calendar_event",
        "description": "Draft a new event on the user's calendar. Read the returned summary back and "
                       "ask the user to confirm; it is only added after they say yes.",
        "parameters": {"type": "object", "properties": {
            "title": {"type": "string"},
            "start": {"type": "string", "description": "Local start, ISO 8601 (2026-10-08T14:00), or "
                                                       "YYYY-MM-DD for an all-day event"},
            "duration_minutes": {"type": "integer", "description": "Default 60"},
            "location": {"type": "string"},
            "calendar": {"type": "string", "description": "Calendar to add it to, as the user said it. "
                                                          "Omit for their main calendar."},
            "also_on": {"type": "array", "items": {"type": "string"},
                        "description": "More dates (YYYY-MM-DD) for the same event at the same time, e.g. "
                                       "'tonight and tomorrow' = start today + also_on [tomorrow]. One draft, "
                                       "one confirmation for all of them."}},
            "required": ["title", "start"]}}},
    {"type": "function", "function": {
        "name": "move_calendar_event",
        "description": "Draft moving or renaming an event ButterBot added (by its ref from "
                       "get_calendar_events). Read it back and ask the user to confirm.",
        "parameters": {"type": "object", "properties": {
            "ref": {"type": "integer"},
            "start": {"type": "string", "description": "New local start, ISO 8601 or YYYY-MM-DD"},
            "duration_minutes": {"type": "integer"},
            "title": {"type": "string"}},
            "required": ["ref"]}}},
    {"type": "function", "function": {
        "name": "remove_calendar_event",
        "description": "Draft deleting an event ButterBot added (by its ref from get_calendar_events). "
                       "Read it back and ask the user to confirm.",
        "parameters": {"type": "object", "properties": {"ref": {"type": "integer"}},
                       "required": ["ref"]}}},
    {"type": "function", "function": {
        "name": "confirm_calendar_change",
        "description": "Apply (confirm=true) or discard (confirm=false) the drafted calendar change "
                       "after the user answers.",
        "parameters": {"type": "object", "properties": {"confirm": {"type": "boolean"}},
                       "required": ["confirm"]}}},
]
PENDING_TTL_S = 120   # a drafted calendar change waits this long for a yes

MEETING_TOOL = {"type": "function", "function": {
    "name": "meeting_reminders",
    "description": "Turn ButterBot's spoken heads-up before calendar meetings on or off, or change how "
                   "many minutes before the start it speaks. Call with no arguments to get the setting.",
    "parameters": {"type": "object", "properties": {
        "enabled": {"type": "boolean"},
        "minutes_before": {"type": "array", "items": {"type": "integer"},
                           "description": "Every reminder time in minutes before the start, 1 to 120, e.g. "
                                          "[15, 5] for two reminders. Replaces the current list."}},
        "required": []}}}
MEETING_CHECK_S = 60   # how often the calendar is polled for meetings about to start


class Agent:
    def __init__(self, db: sqlite3.Connection, tz: str, push, calendar_cfg: dict | None = None,
                 data_dir: Path = Path("data"), phrases: dict[str, int] | None = None):
        """push(device, message_dict) -> awaitable bool (True if delivered).
        phrases: built-in command text -> activation index, for robot_action."""
        self.db = db
        self.phrases = phrases or {}
        self.tz = ZoneInfo(tz)
        self.push = push
        self.calendar = make_calendar(calendar_cfg, self.tz, data_dir)
        self.pending: dict[str, dict] = {}   # device -> drafted calendar change awaiting a yes
        cal_cfg = calendar_cfg or {}
        self.db.execute("create table if not exists settings(key text primary key, value text)")
        self.db.execute("create table if not exists announced(key text primary key, at real)")
        self.db.commit()
        # config.yaml gives the defaults; a voice change ("remind me 15 minutes before meetings")
        # is stored in the database and wins from then on.
        leads = cal_cfg.get("remind_minutes_before", [15, 5])
        self.remind_default = (bool(cal_cfg.get("meeting_reminders", True)),
                               [int(x) for x in (leads if isinstance(leads, list) else [leads])])
        self.last_meeting_check = 0.0
        self.db.execute(
            "create table if not exists tasks(id integer primary key, device text, kind text,"
            " due real, text text, created real, fired real)"
        )
        self.db.commit()

    def tools(self) -> list:
        tools = TOOLS + ([ROBOT_TOOL] if self.phrases else [])
        if not self.calendar.enabled:
            return tools
        return tools + [CALENDAR_TOOL, MEETING_TOOL] + (CALENDAR_WRITE_TOOLS if self.calendar.can_write else [])

    # ------------------------------------------------------------------ meeting heads-up

    def _setting(self, key: str, default: str) -> str:
        row = self.db.execute("select value from settings where key=?", (key,)).fetchone()
        return row[0] if row else default

    def _set(self, key: str, value: str):
        self.db.execute("insert or replace into settings values(?,?)", (key, value))
        self.db.commit()

    def meeting_settings(self) -> tuple[bool, list[int]]:
        """(on, lead times in minutes, largest first), e.g. (True, [15, 5])."""
        on = self._setting("meeting_reminders", "1" if self.remind_default[0] else "0") == "1"
        raw = self._setting("remind_minutes_before", ",".join(str(m) for m in self.remind_default[1]))
        leads = sorted({min(120, max(1, int(x))) for x in raw.split(",") if x.strip()}, reverse=True)
        return on, leads or [10]

    async def check_meetings(self):
        """Speak once per meeting per lead time (e.g. 15 and 5 minutes before). A push that can't reach
        the robot is retried every check until the meeting starts; announced keys survive restarts.
        A meeting found late (added 4 minutes before it starts) gets one reminder, not every one it missed."""
        on, leads = self.meeting_settings()
        if not on or not self.calendar.enabled:
            return
        now = self.now()
        try:
            soon = await self.calendar.upcoming(now, now + dt.timedelta(minutes=leads[0], seconds=MEETING_CHECK_S))
        except Exception as e:
            log.warning("meeting check failed: %s", e)
            return
        due = []   # (meeting, the lead it is due for)
        for m in soon:
            left = (m["start"] - now).total_seconds()
            passed = [lead for lead in leads if left <= lead * 60]
            if not passed:
                continue
            stage = min(passed)   # the closest reminder whose time has come
            if not self.db.execute("select 1 from announced where key=?", (f"{m['key']}#{stage}",)).fetchone():
                due.append((m, stage, passed))
        if not due:
            return
        # Everything due goes out in one push (the robot holds a single pending message), one sentence
        # per start time, so nothing waits for the next check.
        sentences = []
        for start in sorted({m["start"] for m, _, _ in due}):
            together = [m for m, _, _ in due if m["start"] == start]
            final = len(leads) > 1 and any(stage == leads[-1] for m, stage, _ in due if m["start"] == start)
            minutes = max(1, round((start - now).total_seconds() / 60))
            names = " and ".join(m["title"] for m in together)
            place = together[0]["location"] if len(together) == 1 else ""
            # Video-call links aren't worth reading aloud; say it's online instead.
            if re.search(r"https?://|zoom\.us|meet\.google|teams\.microsoft", place, re.I):
                place = "online"
            where = "" if not place else (" It's online." if place == "online" else f" Location: {place}.")
            verb = "start" if len(together) > 1 else "starts"
            # The last reminder is the one meant to pull you out of an overrunning meeting.
            sentences.append(f"{'Time to wrap up' if final else 'Heads up'}: {names} {verb} in {minutes} "
                             f"minute{'s' if minutes != 1 else ''}, at {spoken_clock(start)}.{where}")
        text = " ".join(sentences)
        if await self.push("", {"type": "say", "text": text}):
            # Mark this reminder and any earlier ones it stands in for.
            self.db.executemany("insert or replace into announced values(?,?)",
                                [(f"{m['key']}#{lead}", time.time()) for m, _, passed in due for lead in passed])
            self.db.execute("delete from announced where at < ?", (time.time() - 3 * 86400,))
            self.db.commit()
            log.info("meeting heads-up: %s", text)

    def has_pending(self, device: str) -> bool:
        p = self.pending.get(device)
        if p and time.time() - p["at"] > PENDING_TTL_S:
            del self.pending[device]
            return False
        return p is not None

    @staticmethod
    def _add_starts(args: dict) -> list[str]:
        """The first start plus one per also_on date, same time of day; duplicates and bad dates dropped."""
        start = str(args["start"])
        starts = [start]
        for d in args.get("also_on") or []:
            day = str(d)[:10]
            try:
                dt.date.fromisoformat(day)
            except ValueError:
                continue
            s = day if len(start) == 10 else day + start[10:]
            if s not in starts:
                starts.append(s)
        return starts[:14]   # a spoken request never needs more than two weeks of copies

    def _draft(self, device: str, action: str, args: dict, summary: str) -> str:
        self.pending[device] = {"action": action, "args": args, "summary": summary, "at": time.time()}
        return json.dumps({"status": "needs_confirmation", "read_back": summary,
                           "instruction": "Read this back in one sentence and ask the user to confirm. "
                                          "Do not say it is done yet."})

    async def resolve_pending(self, device: str, confirm: bool) -> str:
        """Apply or discard the drafted change. Returns a sentence for the robot to say."""
        if not self.has_pending(device):
            return "There's no calendar change waiting to be confirmed."
        p = self.pending.pop(device)
        if not confirm:
            return "Okay, I left your calendar alone."
        a = p["args"]
        if p["action"] == "add":
            added = []
            for start in self._add_starts(a):
                added.append(await self.calendar.create(a["title"], start, a.get("duration_minutes"),
                                                        len(start) == 10, a.get("location") or "",
                                                        calendar_id=a.get("calendar_id", "")))
            where = p["summary"].split(" to your ", 1)[1] if " to your " in p["summary"] else "calendar"
            if len(added) == 1:
                return f"Done. I added {added[0]} to your {where}."
            return f"Done. I added {a['title']} on {len(added)} days to your {where}."
        if p["action"] == "move":
            done = await self.calendar.update(int(a["ref"]), a.get("start"), a.get("duration_minutes"),
                                              a.get("title"))
            return f"Done. It's now {done}."
        if p["action"] == "remove":
            done = await self.calendar.delete(int(a["ref"]))
            return f"Done. I removed {done} from your calendar."
        return "I couldn't apply that change."

    def now(self) -> dt.datetime:
        return dt.datetime.now(self.tz)

    def describe_now(self) -> str:
        return self.now().strftime("%A %B %d %Y, %I:%M %p")

    def _add(self, device: str, kind: str, due: float, text: str) -> int:
        cur = self.db.execute("insert into tasks(device, kind, due, text, created) values(?,?,?,?,?)",
                              (device, kind, due, text, time.time()))
        self.db.commit()
        return cur.lastrowid

    def _say_when(self, ts: float) -> str:
        t = dt.datetime.fromtimestamp(ts, self.tz)
        today = self.now().date()
        day = ("today" if t.date() == today else
               "tomorrow" if t.date() == today + dt.timedelta(days=1) else t.strftime("%A"))
        return f"{spoken_clock(t)} {day}"

    def _alarm_time(self, value: str) -> dt.datetime:
        value = value.strip()
        if len(value) <= 5 and ":" in value:  # "7:00" / "07:30": the next time that clock time comes
            hour, minute = (int(p) for p in value.split(":"))
            when = self.now().replace(hour=hour, minute=minute, second=0, microsecond=0)
            if when <= self.now():
                when += dt.timedelta(days=1)
            return when
        when = dt.datetime.fromisoformat(value)
        return when if when.tzinfo else when.replace(tzinfo=self.tz)

    async def run_tool(self, device: str, name: str, args: dict) -> str:
        try:
            if name == "robot_action":
                kind, target = ROBOT_ACTIONS[str(args["action"])]
                if kind == "routine":
                    msg = {"type": "routine", "name": target}
                elif kind == "camera":
                    msg = {"type": "camera", "on": target}
                else:
                    msg = {"type": "action", "index": self.phrases[target]}
                # Queued on the robot; it starts once the spoken reply (and follow-up window) is over.
                if await self.push(device, msg):
                    log.info("robot_action %s -> %s", args["action"], msg)
                    return json.dumps({"ok": True, "note": "starts right after you finish speaking"})
                return json.dumps({"ok": False, "error": "the robot can't do that right now (its "
                                   "firmware doesn't support it yet, or it isn't connected)"})
            if name == "set_timer":
                seconds = max(1, int(args["seconds"]))
                label = (args.get("label") or "").strip()
                text = f"Your {label} timer is done." if label else "Your timer is done."
                tid = self._add(device, "timer", time.time() + seconds, text)
                return json.dumps({"ok": True, "id": tid, "ends": self._say_when(time.time() + seconds)})
            if name == "set_alarm":
                when = self._alarm_time(str(args["time"]))
                if when.timestamp() < time.time():
                    return json.dumps({"ok": False, "error": "that time has already passed"})
                label = (args.get("label") or "").strip()
                clock = spoken_clock(when)
                greeting = "Good morning! " if when.hour < 12 else ""
                text = f"{greeting}It is {clock}. " + (f"Time for {label}." if label else "Time to get up.")
                tid = self._add(device, "alarm", when.timestamp(), text)
                return json.dumps({"ok": True, "id": tid, "rings": self._say_when(when.timestamp())})
            if name == "set_reminder":
                when = dt.datetime.fromisoformat(args["when"])
                if when.tzinfo is None:
                    when = when.replace(tzinfo=self.tz)
                if when.timestamp() < time.time():
                    return json.dumps({"ok": False, "error": "that time has already passed"})
                tid = self._add(device, "reminder", when.timestamp(), f"Reminder: {args['text']}")
                return json.dumps({"ok": True, "id": tid, "due": self._say_when(when.timestamp())})
            if name == "list_reminders":
                rows = self.db.execute("select id, kind, due, text from tasks where fired is null"
                                       " order by due").fetchall()
                return json.dumps([{"id": r[0], "kind": r[1], "due": self._say_when(r[2]),
                                    "text": r[3]} for r in rows])
            if name == "cancel_reminder":
                n = self.db.execute("delete from tasks where id=? and fired is null",
                                    (int(args["id"]),)).rowcount
                self.db.commit()
                return json.dumps({"ok": n > 0})
            if self.calendar.can_write:
                if name == "add_calendar_event":
                    start = str(args["start"])
                    summary = self.calendar.describe_new(args["title"], start, args.get("duration_minutes"),
                                                         len(start) == 10, args.get("location") or "")
                    extra = self._add_starts(args)[1:]
                    if extra:
                        days = [spoken_day(dt.date.fromisoformat(s[:10])) for s in extra]
                        summary += ", and the same on " + " and ".join(days)
                    target = ""
                    if args.get("calendar"):
                        cid, cname = await self.calendar.resolve(str(args["calendar"]), writable=True)
                        args = {**args, "calendar_id": cid}
                        if cid != getattr(self.calendar, "primary_id", ""):
                            target = f" to your {cname} calendar"
                    return self._draft(device, "add", args, f"Add {summary}{target}")
                if name == "move_calendar_event":
                    current = await self.calendar.describe_ref(int(args["ref"]))
                    change = []
                    if args.get("start"):
                        start = str(args["start"])
                        if len(start) == 10:
                            when = f"all day {spoken_day(dt.date.fromisoformat(start))}"
                        else:
                            t = dt.datetime.fromisoformat(start)
                            when = f"{spoken_day(t)} at {spoken_clock(t)}"
                        change.append(f"to {when}")
                    if args.get("title"):
                        change.append(f"renamed {args['title']}")
                    return self._draft(device, "move", args, f"Change {current} {' and '.join(change)}")
                if name == "remove_calendar_event":
                    current = await self.calendar.describe_ref(int(args["ref"]))
                    return self._draft(device, "remove", args, f"Delete {current}")
                if name == "confirm_calendar_change":
                    return json.dumps({"result": await self.resolve_pending(device, bool(args.get("confirm")))})
            if name == "meeting_reminders" and self.calendar.enabled:
                if "enabled" in args and args["enabled"] is not None:
                    self._set("meeting_reminders", "1" if args["enabled"] else "0")
                if args.get("minutes_before"):
                    mins = args["minutes_before"]
                    mins = mins if isinstance(mins, list) else [mins]
                    self._set("remind_minutes_before", ",".join(str(min(120, max(1, int(m)))) for m in mins))
                on, leads = self.meeting_settings()
                return json.dumps({"meeting_reminders": on, "minutes_before": leads})
            if name == "get_calendar_events" and self.calendar.enabled:
                day = dt.date.fromisoformat(args["date"]) if args.get("date") else self.now().date()
                days = min(14, max(1, int(args.get("days") or 1)))
                start = dt.datetime.combine(day, dt.time(), self.tz)
                events = await self.calendar.events(start, start + dt.timedelta(days=days),
                                                    calendar=str(args.get("calendar") or ""))
                out = {"from": spoken_day(start), "days": days, "events": events}
                if args.get("calendar"):
                    out["only_calendar"] = (await self.calendar.resolve(str(args["calendar"])))[1]
                return json.dumps(out)
        except Exception as e:
            return json.dumps({"ok": False, "error": str(e)})
        return json.dumps({"ok": False, "error": f"unknown tool {name}"})

    async def loop(self):
        while True:
            try:
                due = self.db.execute("select id, device, kind, due, text from tasks where fired is null"
                                      " and due <= ? order by due", (time.time(),)).fetchall()
                for tid, device, kind, due_ts, text in due:
                    if kind == "alarm" and time.time() - due_ts > ALARM_STALE_S:
                        self.db.execute("update tasks set fired=? where id=?", (time.time(), tid))
                        self.db.commit()
                        log.warning("dropped alarm #%d: robot unreachable since it was due", tid)
                        continue
                    if kind == "alarm":
                        msg = {"type": "alarm", "text": text}
                    elif kind == "timer" and time.time() - due_ts <= ALARM_STALE_S:
                        msg = {"type": "alarm", "song": "timer", "text": text}
                    else:
                        msg = {"type": "say", "text": text}
                    if await self.push(device, msg):
                        self.db.execute("update tasks set fired=? where id=?", (time.time(), tid))
                        self.db.commit()
                        log.info("fired %s #%d to %s: %s", kind, tid, device, text)
            except Exception:
                log.exception("agent loop")
            if time.time() - self.last_meeting_check >= MEETING_CHECK_S:
                self.last_meeting_check = time.time()
                await self.check_meetings()
            await asyncio.sleep(1)
