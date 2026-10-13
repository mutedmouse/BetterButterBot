"""ButterBot voice gateway: robot streams 16 kHz s16le mono PCM over a WebSocket,
gets back the activation index to run.

Protocol (path /bb):
  robot -> {"type":"hello","vocab":"<fnv32 hex>","n":<activation count>}
  robot -> {"type":"start","codec":"mulaw"|"pcm16"}  then binary audio frames
           then (optionally) {"type":"end","reason":"maxlen"}
  gw    -> {"type":"endpoint","reason":"end|nospeech|maxlen"}  as soon as the speaker stops
  gw    -> {"type":"result","recognized":bool,"index":int,"text":str,"score":float,"ms":int,
            "reply":str (optional)}
           index -1 with recognized=true means "cancel"; "reply" is a spoken answer for anything
           that isn't a command.
"""
import argparse
import asyncio
import json
import logging
import os
import re
import sqlite3
import time
import wave
from pathlib import Path

import numpy as np
import websockets

import config
import vocab
from camera import CameraRelay
from endpoint import Endpointer, decode, load_silero, normalize
from intent import IntentMatcher, normalize as normalize_text
from agent import Agent
from portal import PORTAL_SAY_MAX, PortalMixin  # noqa: F401  (PORTAL_SAY_MAX: tests read it here)

HERE = Path(__file__).resolve().parent
VERSION = (HERE / "VERSION").read_text().strip() if (HERE / "VERSION").exists() else "dev"
# A Scenarios.h copied next to the gateway (server deployments) wins over the repo checkout.
DEFAULT_SCENARIOS = Path(os.environ.get("BB_SCENARIOS", "")) if os.environ.get("BB_SCENARIOS") else (
    HERE / "Scenarios.h" if (HERE / "Scenarios.h").exists() else
    HERE.parent.parent / "ButterBot-Firmware-Public" / "ButterBot-Firmware-Public-master"
    / "components" / "ButterBot-Common" / "src" / "Scenarios.h"
)
# Short answers to "should I add it?" - handled directly while a calendar change is pending, since
# one-word replies never reach the LLM and "no"/"never mind" would otherwise be command matches.
YES_RE = re.compile(r"^(?:yes|yeah|yep|yup|sure|ok(?:ay)?|correct|right|do it|go ahead|please do|confirm(?:ed)?"
                    r"|that'?s (?:right|correct)|sounds good)(?: please)?(?: do it| add it| go ahead)?$")
NO_RE = re.compile(r"^(?:no|nope|nah|don'?t|cancel(?: that| it)?|never ?mind|stop|wrong|no thanks?)(?: please)?$")

# Handled here, not by the LLM: "find me" must actually start the routine (a small model may just
# say "On my way" without calling the tool), and "start over" must work even when the LLM is stalling.
# Matched on the normalized text (wake word, "please" and a leading "can you" already removed).
FIND_ME_RE = re.compile(r"^(?:come (?:and |to )?(?:find|get) me|find me|come (?:over )?here|come to me|"
                        r"come over|where am i|look for me)(?: (?:right )?now| over here)?$")
CAMERA_ON_RE = re.compile(r"^(?:start|turn on|switch on|open)(?: up)? (?:the |your |a )?camera(?: stream| feed)?(?: for (\w+) minutes?)?$"
                          r"|^camera (?:stream )?on$|^start (?:the |a )?(?:camera )?stream(?:ing)?$|^show me (?:the |your )?camera$")
CAMERA_OFF_RE = re.compile(r"^(?:stop|turn off|switch off|close|end)(?: the| your)? camera(?: stream| feed)?$"
                           r"|^camera (?:stream )?off$|^stop (?:the )?stream(?:ing)?$")
RESET_RE = re.compile(r"^(?:start over|start again|forget (?:our|this|the) conversation|forget everything|"
                      r"clear your memory|reset(?: your memory| the conversation)?|new conversation|fresh start)$")

CONFIRM_WINDOW_MS = 6000   # silence allowed before answering "should I add it?"
BUSY_REPLY = "Sorry, my brain is a bit busy right now. Ask me again in a moment."

SAMPLE_RATE = 16000
MAX_SECONDS = 10

log = logging.getLogger("gateway")


class Gateway(PortalMixin):
    def __init__(self, args):
        self.args = args
        self.acts = vocab.load(Path(args.scenarios))
        self.hash = vocab.vocab_hash(self.acts)
        self.matcher = IntentMatcher(self.acts, threshold=args.threshold)
        self.hotwords = self.matcher.hotwords()
        log.info("bb-voice-gateway %s", VERSION)
        log.info("vocab: %d activations, hash %s", len(self.acts), self.hash)

        self.cfg = config.load(args.config, args)
        self.stt = config.build_stt(self.cfg, self.hotwords)
        log.info("STT: %s", self.stt.describe())

        self.data_dir = Path(os.environ.get("BB_DATA_DIR", HERE / "data"))
        (self.data_dir / "wav").mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(self.data_dir / "log.sqlite3", check_same_thread=False)
        self.db.execute(
            "create table if not exists utterances(ts real, dev text, wav text, seconds real,"
            " transcript text, recognized int, idx int, phrase text, score real, stt_ms int)"
        )
        self.lock = asyncio.Lock()
        self.chat = config.build_chat(self.cfg)
        log.info("LLM: %s", self.chat.describe())
        self.robots: dict[str, object] = {}  # device ip -> websocket, once the vocab matched
        self.robot_songs: dict[str, set] = {}  # device ip -> alarm songs its firmware can sing
        self.robot_routines: dict[str, set] = {}  # device ip -> routines it can start on request
        self.robot_camera: dict[str, bool] = {}   # device ip -> firmware can stream its camera
        cam_cfg = self.cfg.get("camera", {}) or {}
        self.camera = CameraRelay(self.data_dir, port=int(cam_cfg.get("port", 8766)),
                                  tls_cert=cam_cfg.get("tls_cert", ""), tls_key=cam_cfg.get("tls_key", ""),
                                  trusted_proxies=cam_cfg.get("trusted_proxies", ["127.0.0.1", "::1"]))
        self.camera.portal = self
        self.portal_init()
        self.agent = Agent(self.db, self.cfg.get("timezone") or os.environ.get("BB_TZ", "UTC"), self.push,
                           calendar_cfg=self.cfg.get("calendar"), data_dir=self.data_dir,
                           phrases={a.text: a.index for a in self.acts})
        log.info("calendar: %s", self.agent.calendar.describe())
        self.admin_token = os.environ.get("BB_ADMIN_TOKEN", "")
        conv = self.cfg.get("conversation", {}) or {}
        # Silence allowed after an answer before the follow-up window closes. 0 = no follow-ups:
        # the window closes on the first frame (the robot still chirps briefly).
        self.followup_ms = max(0, int(float(conv.get("followup_seconds", 3.0)) * 1000))
        # End-of-speech detector: silero (neural speech/non-speech, default) or energy (loudness).
        detector = (self.cfg.get("endpoint", {}) or {}).get("detector", "silero")
        self.silero = load_silero() if detector == "silero" else None
        log.info("endpoint detector: %s", "silero" if self.silero is not None else "energy")

    async def push(self, device: str, message: dict) -> bool:
        """Send a background-agent message to the robot (falls back to any connected robot)."""
        dev = device if device in self.robots else next(iter(self.robots), None)
        if dev is None:
            return False
        ws = self.robots[dev]
        song = message.get("song")
        if song and song not in self.robot_songs.get(dev, ()):
            # Firmware from before that song (no "songs" in its hello): just say the text.
            message = {"type": "say", "text": message.get("text", "")}
        if message.get("type") == "routine" and message.get("name") not in self.robot_routines.get(dev, ()):
            return False   # older firmware would ignore it
        if message.get("type") == "camera":
            if not (self.camera.enabled and self.robot_camera.get(dev)):
                return False
        try:
            await ws.send(json.dumps(message))
        except websockets.ConnectionClosed:
            return False
        if message.get("type") == "camera":   # voice, the LLM's robot_action or the portal switch
            await self.set_camera_wanted(bool(message.get("on")))
        return True

    async def camera_command(self, dev: str, on: bool) -> str:
        """Voice "start/stop the camera stream" (also used by the robot_action tool)."""
        if not self.camera.enabled:
            return "The camera viewer has no accounts yet, so I won't start the stream."
        if not self.robot_camera.get(dev):
            return "My firmware needs an update before I can stream my camera."
        if not await self.push(dev, {"type": "camera", "on": on}):
            return "I couldn't reach my camera just now."
        if on:
            return "Camera stream on. Say stop the camera when you're done."
        return "Camera stream off."

    # The stream stays on until asked to stop. It is never restored: the robot and the gateway always
    # start with it off, and a dropped robot connection ends it.
    async def set_camera_wanted(self, on: bool):
        if self.camera.wanted != on:
            self.camera.wanted = on
            await self.camera.broadcast_status()

    async def warmup(self):
        t = time.perf_counter()
        await self.stt.warmup()
        log.info("warm-up %.1fs", time.perf_counter() - t)

    def save_wav(self, pcm: bytes) -> str:
        name = time.strftime("%Y%m%d-%H%M%S") + f"-{int(time.time() * 1000) % 1000:03d}.wav"
        with wave.open(str(self.data_dir / "wav" / name), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SAMPLE_RATE)
            w.writeframes(pcm)
        return name

    async def handle_utterance(self, ws, dev: str, pcm: bytes, reason: str, levels: str = "",
                               followup: bool = False, chat_listen: bool = False):
        seconds = len(pcm) / 2 / SAMPLE_RATE
        audio = np.frombuffer(pcm, dtype=np.int16).astype(np.float32) / 32768.0
        t = time.perf_counter()
        if reason == "nospeech":
            text = ""  # nothing above the noise floor; don't let Whisper hallucinate on silence
        else:
            try:
                async with self.lock:  # one transcription at a time (local models share the CPU)
                    text = await self.stt.transcribe(normalize(audio))
            except Exception as e:  # remote STT outage: report "didn't understand" rather than hang
                log.warning("STT failed: %s", e)
                text = ""
        stt_ms = int((time.perf_counter() - t) * 1000)
        if chat_listen:
            # The portal's Listen button: text for the chat only, never a command or a question.
            await ws.send(json.dumps({"type": "result", "recognized": False, "index": -1, "text": text,
                                      "score": 0, "ms": stt_ms, "chat": True}))
            log.info("%s %.1fs (%s) stt=%dms chat listen [%s]", dev, seconds, reason, stt_ms, levels)
            await self.chat_heard(dev, text)
            return
        m = self.matcher.match(text)
        result = {"type": "result", "recognized": m.recognized, "index": m.index,
                  "text": text, "score": round(m.score, 1), "ms": stt_ms}

        # Just the wake word (e.g. said during a follow-up window): ask the robot to start a fresh
        # listen, the same as if the on-device wake word had fired.
        if not m.recognized and text and not normalize_text(text) and "butter" in text.lower():
            result["rewake"] = True

        # A drafted calendar change is waiting for a yes/no: answer it here, before command matching.
        answer = re.sub(r"[^a-z' ]", "", text.lower()).strip()
        if self.agent.has_pending(dev) and (YES_RE.match(answer) or NO_RE.match(answer)):
            try:
                reply = await self.agent.resolve_pending(dev, bool(YES_RE.match(answer)))
            except Exception as e:
                log.warning("calendar change failed: %s", e)
                reply = "Sorry, I couldn't change your calendar just now."
            result.update({"recognized": False, "index": -1, "reply": reply})
            result.pop("rewake", None)

        said = normalize_text(text)
        if "reply" not in result and FIND_ME_RE.match(said):
            if await self.push(dev, {"type": "routine", "name": "summon"}):
                reply = "On my way."   # Summon starts once the robot is idle again
            else:
                reply = "I can't come and find you yet. My firmware needs an update for that."
            result.update({"recognized": False, "index": -1, "reply": reply})
            log.info("%s find me -> %s", dev, reply)
        elif "reply" not in result and (CAMERA_ON_RE.match(said) or CAMERA_OFF_RE.match(said)):
            on = bool(CAMERA_ON_RE.match(said))
            reply = await self.camera_command(dev, on)
            result.update({"recognized": False, "index": -1, "reply": reply})
            log.info("%s camera %s -> %s", dev, "on" if on else "off", reply)
        elif "reply" not in result and RESET_RE.match(said):
            self.chat.forget(dev)
            self.agent.pending.pop(dev, None)
            result.update({"recognized": False, "index": -1, "reply": "Okay, fresh start."})
            log.info("%s conversation memory cleared", dev)

        # Not a command but real words: answer it like a question. Outside a conversation, single
        # words are usually noise; in a follow-up they're answers ("yes", "Thursday"), so they count.
        llm_ms = 0
        if "reply" not in result and not m.recognized and "rewake" not in result \
                and len(text.split()) >= (1 if followup else 2) and self.chat.enabled:
            t = time.perf_counter()
            try:
                result["reply"] = await self.chat.ask(
                    dev, text, tools=self.agent.tools(),
                    run_tool=lambda name, a: self.agent.run_tool(dev, name, a),
                    now_text=self.agent.describe_now())
            except Exception as e:  # provider down, over its rate limit, or too slow
                log.warning("LLM failed: %s", str(e) or type(e).__name__)   # timeouts have an empty str()
                # Say so, instead of the robot's "I didn't understand that" (it understood fine).
                result["reply"] = BUSY_REPLY
            llm_ms = int((time.perf_counter() - t) * 1000)

        await ws.send(json.dumps(result))
        log.info("%s %.1fs (%s) stt=%dms '%s' -> %s idx=%d '%s' (%.0f) [%s]", dev, seconds, reason,
                 stt_ms, text, "OK" if result["recognized"] else "NO", result["index"], m.phrase, m.score, levels)
        if "reply" in result:
            log.info("%s llm=%dms reply: %s", dev, llm_ms, result["reply"])
        wav = self.save_wav(pcm) if self.args.keep_audio else ""
        self.db.execute("insert into utterances values(?,?,?,?,?,?,?,?,?,?)",
                        (time.time(), dev, wav, seconds, text, int(m.recognized), m.index,
                         m.phrase, m.score, stt_ms))
        self.db.commit()

    async def serve_client(self, ws):
        dev = f"{ws.remote_address[0]}"
        log.info("connected: %s", dev)
        pcm = bytearray()
        streaming = False
        followup = False
        chat_listen = False
        ep = Endpointer(silero_session=self.silero)
        codec = "pcm16"
        frame_len = 0
        try:
            async for msg in ws:
                if isinstance(msg, bytes):
                    if frame_len:                  # the JPEG announced by the last {"type":"frame"}
                        frame_len = 0
                        await self.camera.publish(msg)
                        continue
                    if streaming and len(pcm) < MAX_SECONDS * SAMPLE_RATE * 2:
                        frame = decode(msg, codec)
                        pcm.extend(frame)
                        ended = ep.feed(frame)
                        if ended:
                            # The gateway owns end-of-utterance: tell the robot to stop listening
                            # now, then transcribe (and maybe ask the LLM) while it waits.
                            streaming = False
                            await ws.send(json.dumps({"type": "endpoint", "reason": ended}))
                            await self.handle_utterance(ws, dev, bytes(pcm), ended, ep.summary(), followup, chat_listen)
                    continue
                ev = json.loads(msg)
                kind = ev.get("type")
                if kind == "hello":
                    ok = ev.get("vocab") == self.hash
                    log.info("hello from %s vocab=%s (%s)", dev, ev.get("vocab"),
                             "match" if ok else f"MISMATCH, server has {self.hash}")
                    await ws.send(json.dumps({"type": "hello", "vocab": self.hash, "ok": ok}))
                    if ok:
                        self.robots[dev] = ws
                        self.robot_songs[dev] = set(ev.get("songs") or ["alarm"])
                        self.robot_routines[dev] = set(ev.get("routines") or [])
                        self.robot_camera[dev] = bool(ev.get("camera"))
                        self.robot_portal[dev] = bool(ev.get("portal"))
                        self.robot_state[dev] = {"mode": "vc"}   # the robot always connects in VC
                        log.info("%s firmware: songs=%s routines=%s camera=%s portal=%s", dev,
                                 sorted(self.robot_songs[dev]), sorted(self.robot_routines[dev]) or "none",
                                 self.robot_camera[dev], self.robot_portal[dev])
                        if self.robot_camera[dev] and self.camera.enabled:
                            # Off at every (re)connect: a robot still streaming from before a gateway
                            # restart or a dropped connection stops; the portal and voice turn it on.
                            await self.push(dev, {"type": "camera", "on": False})
                        await self.camera.broadcast_status()
                elif kind == "status":
                    await self.robot_status(dev, ev)
                elif kind == "battery":
                    await self.robot_battery_report(dev, ev)
                elif kind == "log":
                    # Robot-side diagnostics (e.g. which sensor stopped a movement).
                    log.info("%s robot: %s", dev, str(ev.get("text", ""))[:200])
                elif kind == "frame":
                    frame_len = int(ev.get("len") or 0)
                elif kind == "camera":
                    # The robot's camera stream switched on or off (and why).
                    # Pauses ("paused" for face detection, "offline") resume by themselves on the robot.
                    on, reason, mode = bool(ev.get("on")), ev.get("reason", ""), ev.get("mode", "")
                    await self.camera.set_on(on, mode)
                    log.info("%s camera stream %s%s%s", dev, "on" if on else "off",
                             f" at {mode}" if mode else "", f" ({reason})" if reason else "")
                    if not on and reason in ("timeout", "camera error", "busy"):
                        await self.set_camera_wanted(False)   # ended for good (timeout: older firmware)
                    notice = {"timeout": "The camera stream has stopped.",
                              "busy": "My camera is busy right now, so I didn't start the stream.",
                              "camera error": "My camera wouldn't start streaming."}.get(reason)
                    if notice and not on:
                        await self.push(dev, {"type": "say", "text": notice})
                elif kind == "push":
                    # Admin/agent push from the LAN, e.g. push.py or a future Hermes tool.
                    if not self.admin_token or ev.get("token") != self.admin_token:
                        await ws.send(json.dumps({"type": "push", "ok": False, "error": "bad token"}))
                        continue
                    msg_out = {"type": "say", "text": ev["text"]} if ev.get("text") else \
                        {"type": "action", "index": int(ev.get("index", -1))}
                    delivered = await self.push(ev.get("device", ""), msg_out)
                    await ws.send(json.dumps({"type": "push", "ok": delivered}))
                    log.info("admin push from %s delivered=%s: %s", dev, delivered, msg_out)
                elif kind == "start":
                    pcm.clear()
                    # Follow-up windows give up sooner when nobody speaks (conversation.followup_seconds).
                    # A question waiting for a yes/no (calendar change) gets more time to answer.
                    wait_ms = max(self.followup_ms, CONFIRM_WINDOW_MS) if self.agent.has_pending(dev) \
                        else self.followup_ms
                    ep = Endpointer(silero_session=self.silero, no_speech_ms=wait_ms) if ev.get("followup") \
                        else Endpointer(silero_session=self.silero)
                    codec = ev.get("codec", "pcm16")
                    followup = bool(ev.get("followup"))
                    chat_listen = bool(ev.get("chat"))   # the portal's Listen button: text only
                    streaming = True
                elif kind == "end" and streaming:
                    streaming = False
                    await self.handle_utterance(ws, dev, bytes(pcm), ev.get("reason", ""),
                                                ep.summary(), followup, chat_listen)
                elif kind == "ping":
                    await ws.send(json.dumps({"type": "pong"}))
        except websockets.ConnectionClosed:
            pass
        finally:
            if self.robots.get(dev) is ws:
                del self.robots[dev]
                self.robot_songs.pop(dev, None)
                self.robot_routines.pop(dev, None)
                if self.robot_camera.pop(dev, None) and self.camera.on:
                    await self.camera.set_on(False)
                await self.portal_robot_gone(dev)
                if not self.robots:
                    await self.set_camera_wanted(False)
                await self.camera.broadcast_status()
            log.info("disconnected: %s", dev)


async def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="0.0.0.0")
    p.add_argument("--port", type=int, default=8765)
    p.add_argument("--model", default="small.en")
    p.add_argument("--device", default="cpu")
    p.add_argument("--compute-type", default="int8")
    p.add_argument("--threads", type=int, default=8)
    p.add_argument("--beam", type=int, default=1)
    p.add_argument("--threshold", type=float, default=80.0)
    p.add_argument("--scenarios", default=str(DEFAULT_SCENARIOS))
    p.add_argument("--keep-audio", action=argparse.BooleanOptionalAction, default=True)
    p.add_argument("--config", default=None,
                   help="config.yaml path (default: next to gateway.py). Without one, the STT flags "
                        "above and BB_LLM_* environment variables are used.")
    args = p.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    # websockets.server logs every TCP connection, including Docker's health probes every 30 s;
    # the gateway logs real robot connections itself ("connected:" / "disconnected:").
    for noisy in ("httpx", "huggingface_hub", "faster_whisper", "websockets.server"):
        logging.getLogger(noisy).setLevel(logging.WARNING)
    gw = Gateway(args)
    await gw.warmup()
    asyncio.create_task(gw.agent.loop())
    try:
        await gw.camera.start()
        log.info("camera stream: %s", gw.camera.describe())
    except OSError as e:   # e.g. the port is taken: voice must keep working without the viewer
        log.warning("camera viewer off: can't listen on port %d (%s)", gw.camera.port, e.strerror or e)
    async with websockets.serve(gw.serve_client, args.host, args.port, max_size=2**20,
                                ping_interval=20, ping_timeout=60):  # robot Wi-Fi power-save can stall replies for seconds
        log.info("listening on ws://%s:%d", args.host, args.port)
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
