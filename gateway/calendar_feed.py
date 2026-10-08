"""Read-only calendar access for the agent, from iCal (.ics) feeds.

Google Calendar: Settings -> (your calendar) -> Integrate calendar -> "Secret address in iCal
format". Put that URL in secret.env as BB_CALENDAR_ICS (several feeds: separate with spaces or
commas). Outlook/iCloud/Fastmail publish the same kind of .ics link. No OAuth, nothing is written
to the calendar, and the URL never leaves the gateway.
"""
import asyncio
import datetime as dt
import logging
import os
import re
import time
from zoneinfo import ZoneInfo

import httpx

log = logging.getLogger("calendar")

CACHE_S = 300         # re-download a feed at most every 5 minutes
MAX_EVENTS = 25


def spoken_day(d: dt.date) -> str:
    """'Thursday October 8' - no zero padding, which TTS reads as 'oh eight'."""
    return f"{d:%A %B} {d.day}"


def spoken_clock(t: dt.datetime) -> str:
    """'2 PM' / '2:30 PM'."""
    hour = t.hour % 12 or 12
    return f"{hour} {t:%p}" if t.minute == 0 else f"{hour}:{t.minute:02d} {t:%p}"


PRIMARY_WORDS = {"my", "mine", "main", "personal", "primary", "default", "own"}


def match_calendar(spoken: str, calendars: list[tuple[str, str]], primary_id: str = "") -> tuple[str, str] | None:
    """Pick the calendar the user named, tolerating speech-recognition spelling ('why mca' -> 'YMCA').
    calendars: (id, name). 'my calendar' / 'personal' means the primary one."""
    from rapidfuzz import fuzz

    words = re.sub(r"\bcalendars?\b", " ", spoken.lower())
    words = re.sub(r"[^a-z0-9@. ]", " ", words).split()
    if primary_id and (not words or set(words) <= PRIMARY_WORDS):
        return next((c for c in calendars if c[0] == primary_id), None)
    target = " ".join(w for w in words if w not in PRIMARY_WORDS) or " ".join(words)
    best, score = None, 0.0
    for cid, name in calendars:
        for candidate in {name.lower(), cid.lower().split("@")[0]}:
            # Whole-name similarity only: substring matching let short misheard names hit long ones ("Holidays in United States").
            s = max(fuzz.ratio(target, candidate),
                    fuzz.ratio(target.replace(" ", ""), candidate.replace(" ", "")),   # "book club" = BookClub
                    fuzz.token_set_ratio(target, candidate) if len(target) >= 5 else 0)
            if s > score:
                best, score = (cid, name), s
    return best if score >= 70 else None


def make_calendar(cfg: dict | None, tz: ZoneInfo, data_dir):
    """calendar.provider: google (OAuth, calendar_google.py) or ics (default)."""
    cfg = cfg or {}
    if cfg.get("provider") == "google":
        from calendar_google import GoogleCalendar
        return GoogleCalendar(cfg, tz, data_dir)
    return CalendarFeed(cfg, tz)


class CalendarFeed:
    def __init__(self, cfg: dict, tz: ZoneInfo):
        cfg = cfg or {}
        env = cfg.get("ics_url_env", "BB_CALENDAR_ICS")
        self.urls = [u for u in re.split(r"[\s,]+", os.environ.get(env, "")) if u]
        self.tz = tz
        self.client = httpx.AsyncClient(timeout=httpx.Timeout(8.0, connect=4.0), follow_redirects=True)
        self.cache: dict[str, tuple[float, bytes]] = {}

    can_write = False   # iCal links are read-only

    @property
    def enabled(self) -> bool:
        return bool(self.urls)

    def describe(self) -> str:
        return f"{len(self.urls)} iCal feed(s)" if self.urls else "off (set BB_CALENDAR_ICS in secret.env)"

    async def _fetch(self, url: str) -> bytes:
        hit = self.cache.get(url)
        if hit and time.time() - hit[0] < CACHE_S:
            return hit[1]
        r = await self.client.get(url)
        r.raise_for_status()
        self.cache[url] = (time.time(), r.content)
        return r.content

    async def upcoming(self, start: dt.datetime, end: dt.datetime) -> list[dict]:
        """Timed, non-cancelled events starting in [start, end), with a stable 'key'."""
        import icalendar
        import recurring_ical_events

        feeds = await asyncio.gather(*(self._fetch(u) for u in self.urls), return_exceptions=True)
        out = []
        for data in feeds:
            if isinstance(data, Exception):
                continue
            cal = icalendar.Calendar.from_ical(data)
            for ev in recurring_ical_events.of(cal).between(start, end):
                s = ev.decoded("DTSTART")
                if not isinstance(s, dt.datetime) or str(ev.get("STATUS", "")).upper() == "CANCELLED":
                    continue
                s_local = (s if s.tzinfo else s.replace(tzinfo=self.tz)).astimezone(self.tz)
                if start <= s_local < end:
                    out.append({"key": f"{ev.get('UID', '')}/{s_local.isoformat()}",
                                "title": str(ev.get("SUMMARY", "") or "a meeting"), "start": s_local,
                                "location": str(ev.get("LOCATION", "") or "")})
        return sorted(out, key=lambda x: x["start"])

    async def resolve(self, spoken: str, writable: bool = False) -> tuple[str, str]:
        """The feed the user named -> (url, calendar name), matched on the feed's X-WR-CALNAME."""
        import icalendar

        feeds = await asyncio.gather(*(self._fetch(u) for u in self.urls), return_exceptions=True)
        cals = [(u, str(icalendar.Calendar.from_ical(d).get("X-WR-CALNAME", "") or u))
                for u, d in zip(self.urls, feeds) if not isinstance(d, Exception)]
        hit = match_calendar(spoken, cals)
        if hit is None:
            raise RuntimeError(f"no calendar matches '{spoken}'. Available: {', '.join(n for _, n in cals)}. "
                               "Ask the user which one they meant.")
        return hit

    async def events(self, start: dt.datetime, end: dt.datetime, calendar: str = "") -> list[dict]:
        import icalendar
        import recurring_ical_events

        urls = [(await self.resolve(calendar))[0]] if calendar else self.urls
        feeds = await asyncio.gather(*(self._fetch(u) for u in urls), return_exceptions=True)
        out = []
        for url, data in zip(urls, feeds):
            if isinstance(data, Exception):
                log.warning("calendar feed failed: %s", data)
                continue
            cal = icalendar.Calendar.from_ical(data)
            name = str(cal.get("X-WR-CALNAME", "") or "")
            for ev in recurring_ical_events.of(cal).between(start, end):
                if str(ev.get("STATUS", "")).upper() == "CANCELLED":
                    continue
                s = ev.decoded("DTSTART")
                e = ev.decoded("DTEND") if ev.get("DTEND") else None
                all_day = not isinstance(s, dt.datetime)
                if all_day:
                    s_local = dt.datetime.combine(s, dt.time(), self.tz)
                else:
                    s_local = (s if s.tzinfo else s.replace(tzinfo=self.tz)).astimezone(self.tz)
                    if e is not None and isinstance(e, dt.datetime):
                        e = (e if e.tzinfo else e.replace(tzinfo=self.tz)).astimezone(self.tz)
                out.append({
                    "sort": s_local,
                    "title": str(ev.get("SUMMARY", "") or "(no title)"),
                    "all_day": all_day,
                    "start": None if all_day else spoken_clock(s_local),
                    "end": None if all_day or not isinstance(e, dt.datetime) else spoken_clock(e),
                    "day": spoken_day(s_local),
                    "location": str(ev.get("LOCATION", "") or "") or None,
                    "calendar": name or None,
                })
        out.sort(key=lambda x: (x["sort"], not x["all_day"]))
        for x in out:
            del x["sort"]
        return [{k: v for k, v in x.items() if v not in (None, False)} for x in out[:MAX_EVENTS]]
