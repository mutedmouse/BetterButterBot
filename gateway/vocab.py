"""Loads ButterBot's command vocabulary straight from the firmware's Scenarios.h.

Indices match the order of the C++ `activations` array, which is what the robot's
ListenState::onPhrase() expects. The FNV-1a hash must match VoiceLink::vocabHash() on the device.
"""
import re
from dataclasses import dataclass
from pathlib import Path

ENTRY_RE = re.compile(
    r'^\s*\{\s*BB::Action::Scenario::(\w+)\s*,\s*(\{\}|\w+\{[^}]*\})\s*,\s*"([^"]*)"',
    re.MULTILINE,
)


@dataclass(frozen=True)
class Activation:
    index: int
    scenario: str
    data: str
    text: str


def load(scenarios_h: Path) -> list[Activation]:
    src = scenarios_h.read_text(encoding="utf-8")
    start = src.index("activations")
    body = src[start:]
    # Drop comments so commented-out entries don't shift indices and inline /* */ notes don't
    # break the entry pattern.
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.DOTALL)
    body = re.sub(r"//[^\n]*", "", body)
    return [
        Activation(i, m.group(1), m.group(2), m.group(3))
        for i, m in enumerate(ENTRY_RE.finditer(body))
    ]


def vocab_hash(acts: list[Activation]) -> str:
    h = 0x811C9DC5
    for a in acts:
        for b in (a.text + "\n").encode("utf-8"):
            h ^= b
            h = (h * 0x01000193) & 0xFFFFFFFF
    return f"{h:08x}"


if __name__ == "__main__":
    import sys

    acts = load(Path(sys.argv[1]))
    for a in acts:
        print(a.index, a.scenario, a.data, repr(a.text))
    print("count", len(acts), "hash", vocab_hash(acts))
