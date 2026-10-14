"""Google Calendar access through the Calendar API (OAuth refresh token).

Create the token once with calendar_auth.py on any computer with a browser, then put the file in
the gateway's data directory. config.yaml:

    calendar:
      provider: google
      token_file: google_token.json     # relative to the data directory
      calendars: selected               # "selected" (as ticked in Google Calendar), "primary",
                                        # or a list of calendar ids
      write_calendar: primary           # where new events go (needs a --write sign-in)
      token_days: 7                     # how long a sign-in lasts: 7 while the Google Cloud app is in
                                        # "Testing" (the robot warns a day ahead); 0 once it's published

Reading needs the calendar.readonly scope. With the calendar.events scope (calendar_auth.py
--write) ButterBot can also add events, and move or delete the events it created itself: those
carry a private "butterbot" tag, and anything without it is refused here, not just in the prompt.
"""
import asyncio
import datetime as dt
import json
import logging
import time
from pathlib import Path
from urllib.parse import quote
from zoneinfo import ZoneInfo

import httpx

from calendar_feed import match_calendar, spoken_clock, spoken_day

log = logging.getLogger("calendar")

API = "https://www.googleapis.com/calendar/v3"
MAX_EVENTS = 25
WRITE_SCOPES = ("https://www.googleapis.com/auth/calendar.events", "https://www.googleapis.com/auth/calendar")
TAG = {"private": {"butterbot": "1"}}


class GoogleCalendar:
    def __init__(self, cfg: dict, tz: ZoneInfo, data_dir: Path):
        cfg = cfg or {}
        path = Path(cfg.get("token_file", "google_token.json"))
        self.token_path = path if path.is_absolute() else data_dir / path
        self.which = cfg.get("calendars", "selected")
        self.write_calendar = cfg.get("write_calendar", "primary")
        self.token_days = float(cfg.get("token_days", 7) or 0)
        self.tz = tz
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(8.0, connect=4.0))
        self.access_token = ""
        self.expires = 0.0
        self.calendars: list[tuple[str, str]] | None = None   # (id, name), cached after first use
        self.calendars_at = 0.0
        # Short numeric handles for the events ButterBot created, so the LLM can say "ref 2"
        # instead of copying long ids. Refilled on every listing.
        self.refs: dict[int, tuple[str, str]] = {}

    @property
    def enabled(self) -> bool:
        return self.token_path.exists()

    @property
    def can_write(self) -> bool:
        if not self.enabled:
            return False
        scope = json.loads(self.token_path.read_text()).get("scope", "")
        return any(s in scope.split() for s in WRITE_SCOPES)

    def login_expires_at(self) -> float | None:
        """When the sign-in stops working (Google expires "Testing" apps' tokens after 7 days), or None."""
        if not self.token_days or not self.enabled:
            return None
        try:
            issued = json.loads(self.token_path.read_text()).get("issued_at") or self.token_path.stat().st_mtime
        except (OSError, ValueError):
            return None
        return float(issued) + self.token_days * 86400

    def describe(self) -> str:
        if not self.enabled:
            return f"off (no {self.token_path}; run calendar_auth.py)"
        n = self.which if isinstance(self.which, str) else f"{len(self.which)}"
        return f"Google Calendar ({n} calendars, {'read/write' if self.can_write else 'read-only'})"

    async def _token(self) -> str:
        if self.access_token and time.time() < self.expires - 60:
            return self.access_token
        cred = json.loads(self.token_path.read_text())
        r = await self.client.post(cred.get("token_uri", "https://oauth2.googleapis.com/token"), data={
            "client_id": cred["client_id"], "client_secret": cred.get("client_secret", ""),
            "refresh_token": cred["refresh_token"], "grant_type": "refresh_token"})
        if r.status_code >= 400:
            # invalid_grant: revoked, or the consent screen is still in "Testing" (7-day tokens).
            # Worded for the LLM, which relays it to the user instead of reporting an empty day.
            log.warning("Google token refresh failed (%s): %s", r.status_code, r.text[:200])
            raise RuntimeError("calendar login expired: tell the user their Google Calendar access has "
                               "expired and the owner needs to rerun the calendar sign-in")
        tok = r.json()
        self.access_token = tok["access_token"]
        self.expires = time.time() + int(tok.get("expires_in", 3600))
        return self.access_token

    async def _call(self, method: str, path: str, params: dict | None = None, body: dict | None = None) -> dict:
        r = await self.client.request(method, f"{API}{path}", params=params, json=body,
                                      headers={"Authorization": f"Bearer {await self._token()}"})
        r.raise_for_status()
        return r.json() if r.content else {}

    async def _get(self, path: str, params: dict | None = None) -> dict:
        return await self._call("GET", path, params)

    async def _calendar_ids(self) -> list[tuple[str, str]]:
        if isinstance(self.which, list):
            return [(c, c) for c in self.which]
        if self.which == "primary":
            return [("primary", "")]
        await self._refresh_list()
        return self.calendars

    async def _refresh_list(self):
        # Every calendar (hidden ones too) is kept for naming; the default read covers the ticked ones.
        if self.calendars is not None and time.time() - self.calendars_at < 600:
            return
        items = (await self._get("/users/me/calendarList",
                                 {"minAccessRole": "reader", "showHidden": "true"})).get("items", [])
        self.all_calendars = [(c["id"], c.get("summaryOverride") or c.get("summary", "")) for c in items]
        self.calendars = [(c["id"], c.get("summaryOverride") or c.get("summary", ""))
                          for c in items if c.get("selected") or c.get("primary")]
        self.roles = {c["id"]: c.get("accessRole", "reader") for c in items}
        self.primary_id = next((c["id"] for c in items if c.get("primary")), "")
        self.calendars_at = time.time()

    async def resolve(self, spoken: str, writable: bool = False) -> tuple[str, str]:
        """The calendar the user named -> (id, name), hidden calendars included. Raises with the
        available names if nothing matches well."""
        if isinstance(self.which, list) or self.which == "primary":
            cals = await self._calendar_ids()
        else:
            await self._refresh_list()
            cals = self.all_calendars
        if writable:
            cals = [c for c in cals if getattr(self, "roles", {}).get(c[0], "owner") in ("owner", "writer")]
        hit = match_calendar(spoken, cals, getattr(self, "primary_id", ""))
        if hit is None:
            names = ", ".join(n or i for i, n in cals)
            raise RuntimeError(f"no {'writable ' if writable else ''}calendar matches '{spoken}'. "
                               f"Available: {names}. Ask the user which one they meant.")
        return hit

    def _clock(self, t: dt.datetime) -> str:
        return spoken_clock(t)

    def _describe_event(self, ev: dict, cal_name: str = "") -> tuple[dt.datetime, dict]:
        s, e = ev.get("start", {}), ev.get("end", {})
        all_day = "date" in s
        if all_day:
            s_local = dt.datetime.combine(dt.date.fromisoformat(s["date"]), dt.time(), self.tz)
            e_local = None
        else:
            s_local = dt.datetime.fromisoformat(s["dateTime"]).astimezone(self.tz)
            e_local = dt.datetime.fromisoformat(e["dateTime"]).astimezone(self.tz) if "dateTime" in e else None
        return s_local, {
            "title": ev.get("summary") or "(no title)",
            "all_day": all_day,
            "start": None if all_day else self._clock(s_local),
            "end": self._clock(e_local) if e_local else None,
            "day": spoken_day(s_local),
            "location": ev.get("location") or None,
            "calendar": cal_name or None,
        }

    async def _raw(self, start: dt.datetime, end: dt.datetime,
                   only: list[tuple[str, str]] | None = None) -> list[tuple[str, str, dict]]:
        """(calendar id, calendar name, event) for every live event between start and end."""
        cals = only or await self._calendar_ids()
        params = {"timeMin": start.isoformat(), "timeMax": end.isoformat(), "singleEvents": "true",
                  "orderBy": "startTime", "maxResults": "50", "timeZone": str(self.tz)}
        results = await asyncio.gather(*(self._get(f"/calendars/{quote(cid, safe='')}/events", params)
                                         for cid, _ in cals), return_exceptions=True)
        if results and all(isinstance(r, Exception) for r in results):
            raise results[0]  # e.g. expired login: report it rather than "no events"
        out = []
        for (cid, name), res in zip(cals, results):
            if isinstance(res, Exception):
                log.warning("calendar %s failed: %s", name or cid, res)
                continue
            out += [(cid, name, ev) for ev in res.get("items", []) if ev.get("status") != "cancelled"]
        return out

    async def upcoming(self, start: dt.datetime, end: dt.datetime) -> list[dict]:
        """Timed events starting in [start, end) worth a heads-up: not all-day, not declined, not
        out-of-office/working-location/focus-time blocks. Each has a stable 'key' for de-duplication."""
        out = []
        for cid, name, ev in await self._raw(start, end):
            if "dateTime" not in ev.get("start", {}):
                continue
            if ev.get("eventType", "default") not in ("default", "fromGmail"):
                continue
            if any(a.get("self") and a.get("responseStatus") == "declined" for a in ev.get("attendees", [])):
                continue
            s_local = dt.datetime.fromisoformat(ev["start"]["dateTime"]).astimezone(self.tz)
            if not (start <= s_local < end):
                continue  # already running (timeMin matches anything that hasn't ended)
            out.append({"key": f"{cid}/{ev['id']}/{s_local.isoformat()}", "title": ev.get("summary") or "a meeting",
                        "start": s_local, "location": ev.get("location") or "", "calendar": name})
        return sorted(out, key=lambda x: x["start"])

    async def events(self, start: dt.datetime, end: dt.datetime, calendar: str = "") -> list[dict]:
        only = [await self.resolve(calendar)] if calendar else None
        out = []
        for cid, name, ev in await self._raw(start, end, only):
            s_local, info = self._describe_event(ev, name)
            own = ev.get("extendedProperties", {}).get("private", {}).get("butterbot") == "1"
            out.append((s_local, info, (cid, ev["id"]) if own else None))
        out.sort(key=lambda x: (x[0], not x[1]["all_day"]))
        self.refs = {}
        listed = []
        for _, info, own in out[:MAX_EVENTS]:
            if own:
                ref = len(self.refs) + 1
                self.refs[ref] = own
                info["ref"] = ref   # only ButterBot's own events can be changed
            listed.append({k: v for k, v in info.items() if v not in (None, False)})
        return listed

    # ------------------------------------------------------------------ writing

    def _times(self, start: str, duration_min: int | None, all_day: bool) -> tuple[dict, dict, str]:
        if all_day:
            day = dt.date.fromisoformat(start[:10])
            return ({"date": day.isoformat()}, {"date": (day + dt.timedelta(days=1)).isoformat()},
                    f"all day {spoken_day(day)}")
        s = dt.datetime.fromisoformat(start)
        s = s if s.tzinfo else s.replace(tzinfo=self.tz)
        e = s + dt.timedelta(minutes=int(duration_min or 60))
        tz = str(self.tz)
        return ({"dateTime": s.isoformat(), "timeZone": tz}, {"dateTime": e.isoformat(), "timeZone": tz},
                f"{spoken_day(s)} from {self._clock(s)} to {self._clock(e)}")

    def describe_new(self, title: str, start: str, duration_min: int | None, all_day: bool,
                     location: str = "") -> str:
        _, _, when = self._times(start, duration_min, all_day)
        return f"{title}, {when}" + (f", at {location}" if location else "")

    async def create(self, title: str, start: str, duration_min: int | None, all_day: bool,
                     location: str = "", calendar_id: str = "") -> str:
        s, e, when = self._times(start, duration_min, all_day)
        body = {"summary": title, "start": s, "end": e, "extendedProperties": TAG,
                "description": "Added by ButterBot"}
        if location:
            body["location"] = location
        target = calendar_id or self.write_calendar
        await self._call("POST", f"/calendars/{quote(target, safe='')}/events", body=body)
        return f"{title}, {when}"

    async def _own(self, ref: int) -> tuple[str, str, dict]:
        if ref not in self.refs:
            raise RuntimeError("only events ButterBot added itself can be changed, and they must be "
                               "listed first (get_calendar_events) to get their ref")
        cid, eid = self.refs[ref]
        ev = await self._get(f"/calendars/{quote(cid, safe='')}/events/{quote(eid, safe='')}")
        if ev.get("extendedProperties", {}).get("private", {}).get("butterbot") != "1":
            raise RuntimeError("that event was not created by ButterBot, so it can't be changed")
        return cid, eid, ev

    async def describe_ref(self, ref: int) -> str:
        _, _, ev = await self._own(ref)
        s_local, info = self._describe_event(ev)
        when = f"all day {info['day']}" if info["all_day"] else f"{info['day']} at {info['start']}"
        return f"{info['title']}, {when}"

    async def update(self, ref: int, start: str | None, duration_min: int | None, title: str | None) -> str:
        cid, eid, ev = await self._own(ref)
        patch = {}
        if title:
            patch["summary"] = title
        if start:
            all_day = len(start) == 10
            if duration_min is None and not all_day and "dateTime" in ev.get("start", {}):
                old_s = dt.datetime.fromisoformat(ev["start"]["dateTime"])
                old_e = dt.datetime.fromisoformat(ev["end"]["dateTime"])
                duration_min = int((old_e - old_s).total_seconds() // 60)
            patch["start"], patch["end"], _ = self._times(start, duration_min, all_day)
        new = await self._call("PATCH", f"/calendars/{quote(cid, safe='')}/events/{quote(eid, safe='')}", body=patch)
        _, info = self._describe_event(new)
        return f"{info['title']}, " + (f"all day {info['day']}" if info["all_day"] else f"{info['day']} at {info['start']}")

    async def delete(self, ref: int) -> str:
        cid, eid, ev = await self._own(ref)
        await self._call("DELETE", f"/calendars/{quote(cid, safe='')}/events/{quote(eid, safe='')}")
        self.refs.pop(ref, None)
        return ev.get("summary") or "the event"
