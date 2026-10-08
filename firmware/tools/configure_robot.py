"""Set the robot's Wi-Fi network and voice-gateway address before building the firmware.

    python tools/configure_robot.py --ssid "MyNetwork" --password "secret" --gateway 192.168.1.50
    python tools/configure_robot.py --show

Writes CONFIG_BB_VOICELINK_SSID / _PASSWORD / _URL into sdkconfig (created from sdkconfig.defaults
if missing), then run `idf.py build`. The values are compiled into the firmware, so a robot built
this way only works on that network - and the built images contain the Wi-Fi password: don't share
them. Standard library only; run from the firmware folder (or pass --sdkconfig).
"""
import argparse
import ipaddress
import re
import shutil
import sys
from pathlib import Path

KEYS = ("CONFIG_BB_VOICELINK_SSID", "CONFIG_BB_VOICELINK_PASSWORD", "CONFIG_BB_VOICELINK_URL")


def quote(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def read(path: Path) -> dict:
    vals = {}
    for line in path.read_text().splitlines():
        for k in KEYS:
            if line.startswith(k + "="):
                vals[k] = line.split("=", 1)[1].strip('"')
    return vals


def main():
    here = Path(__file__).resolve().parent.parent
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ssid", help="2.4 GHz Wi-Fi network name (max 32 characters)")
    ap.add_argument("--password", help="Wi-Fi password (8-63 characters; omit for an open network)", default=None)
    ap.add_argument("--gateway", help="IP or hostname of the machine running bb-voice-gateway")
    ap.add_argument("--port", type=int, default=8765, help="gateway port (default 8765)")
    ap.add_argument("--sdkconfig", type=Path, default=here / "sdkconfig")
    ap.add_argument("--show", action="store_true", help="print the current settings (password masked)")
    args = ap.parse_args()

    cfg = args.sdkconfig
    if not cfg.exists():
        base = next((p for p in (cfg.with_name("sdkconfig.defaults"), cfg.with_name("sdkconfig.public")) if p.exists()), None)
        if base is None:
            sys.exit(f"{cfg} not found (and no sdkconfig.defaults next to it to start from)")
        shutil.copy(base, cfg)
        print(f"Created {cfg} from {base.name}")

    if args.show:
        vals = read(cfg)
        print(f"Wi-Fi:    {vals.get(KEYS[0]) or '(not set - LAN voice off)'}")
        print(f"Password: {'*' * len(vals.get(KEYS[1], '')) or '(none)'}")
        print(f"Gateway:  {vals.get(KEYS[2], '(not set)')}")
        return

    if not (args.ssid and args.gateway):
        ap.error("--ssid and --gateway are required (or use --show)")
    if len(args.ssid.encode()) > 32:
        ap.error("Wi-Fi names are at most 32 bytes")
    password = args.password or ""
    if password and not 8 <= len(password) <= 63:
        ap.error("WPA2 passwords are 8-63 characters")
    host = args.gateway.strip()
    host = re.sub(r"^wss?://", "", host).split("/")[0].split(":")[0]
    try:
        ipaddress.ip_address(host)
    except ValueError:
        if not re.fullmatch(r"[A-Za-z0-9.-]+", host):
            ap.error(f"'{args.gateway}' is not an IP address or hostname")
        print(f"Note: '{host}' is a hostname; the robot must be able to resolve it via your router's DNS.")
    url = f"ws://{host}:{args.port}/bb"

    lines = cfg.read_text().splitlines()
    new = {KEYS[0]: args.ssid, KEYS[1]: password, KEYS[2]: url}
    seen = set()
    for i, line in enumerate(lines):
        for k, v in new.items():
            if line.startswith(k + "=") or line.startswith(f"# {k} is not set"):
                lines[i] = f"{k}={quote(v)}"
                seen.add(k)
    lines += [f"{k}={quote(v)}" for k, v in new.items() if k not in seen]
    cfg.write_text("\n".join(lines) + "\n")
    print(f"Updated {cfg}: Wi-Fi '{args.ssid}', gateway {url}")
    print("Next: idf.py build   (then flash - see docs/INSTALL.md)")


if __name__ == "__main__":
    main()
