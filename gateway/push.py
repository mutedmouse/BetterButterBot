"""Make ButterBot say something (or run a command index) via the gateway's admin push.
Usage: BB_ADMIN_TOKEN=... python push.py "text to say" [ws://<gateway-ip>:8765/bb]
       BB_ADMIN_TOKEN=... python push.py --index 9            (runs activation #9, "tell me a joke")
"""
import asyncio
import json
import os
import sys

import websockets


async def main():
    args = sys.argv[1:]
    url = next((a for a in args if a.startswith("ws://")), "ws://127.0.0.1:8765/bb")
    msg = {"type": "push", "token": os.environ.get("BB_ADMIN_TOKEN", "")}
    if "--index" in args:
        msg["index"] = int(args[args.index("--index") + 1])
    else:
        msg["text"] = next(a for a in args if not a.startswith("ws://"))
    async with websockets.connect(url) as ws:
        await ws.send(json.dumps(msg))
        print(await ws.recv())


asyncio.run(main())
