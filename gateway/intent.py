"""Maps a Whisper transcript onto a ButterBot activation index."""
import re
from dataclasses import dataclass

from rapidfuzz import fuzz

from vocab import Activation

ABORT_RE = re.compile(r"^(cancel|never ?mind|forget it|stop listening)( that)?$")
# Ways of ending a conversation said in a longer sentence ("I didn't ask you anything, stop
# listening"); only honoured in short utterances so real questions that contain them still go through.
END_RE = re.compile(r"\b(stop listening|never ?mind|that'?s all|that is all|thank you|thanks|"
                    r"go to sleep|be quiet|shut up|stop talking|i didn'?t ask( you)?( anything)?|"
                    r"not talking to you|no more questions)\b")
END_MAX_WORDS = 9
STOPWORDS = {
    "a", "an", "the", "me", "my", "you", "your", "i", "to", "it", "is", "are", "do", "and",
    "of", "on", "off", "in", "what", "how", "some", "something", "want", "give", "tell", "please",
}
ONES = "zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen " \
       "fifteen sixteen seventeen eighteen nineteen".split()
TENS = "_ _ twenty thirty forty fifty sixty seventy eighty ninety".split()


def num_words(n: int) -> str:
    if n < 20:
        return ONES[n]
    if n < 100:
        return TENS[n // 10] + ("" if n % 10 == 0 else " " + ONES[n % 10])
    if n < 1000:
        rest = n % 100
        return ONES[n // 100] + " hundred" + ("" if rest == 0 else " " + num_words(rest))
    return str(n)


def normalize(text: str) -> str:
    text = text.lower().replace("-", " ")
    text = re.sub(r"(?<=[a-z])(?=\d)|(?<=\d)(?=[a-z])", " ", text)  # d20 -> d 20
    text = re.sub(r"[^a-z0-9' ]+", " ", text)
    text = re.sub(r"\bad (?=\d)", "a d ", text)  # whisper hears "a d 20" as "ad 20"
    text = re.sub(r"\d+", lambda m: num_words(int(m.group())), text)
    text = text.replace("a hundred", "one hundred")
    # Wake words / politeness that never change the command.
    text = re.sub(r"\b(hey |ok |okay |hi )?butter ?(bot|but|butt|boat)s?\b", " ", text)
    text = re.sub(r"\bplease\b", " ", text)
    text = re.sub(r"^\s*(can|could|would|will) you\b", " ", text)
    return re.sub(r"\s+", " ", text).strip()


@dataclass
class Match:
    recognized: bool
    index: int  # -1 = abort (only meaningful when recognized)
    score: float
    phrase: str


class IntentMatcher:
    def __init__(self, acts: list[Activation], threshold: float = 80.0):
        self.acts = acts
        self.threshold = threshold
        self.choices = [normalize(a.text) for a in acts]

    def hotwords(self, limit: int = 60) -> str:
        """Distinctive command words used to bias Whisper's decoder."""
        seen: dict[str, None] = {"butterbot": None, "butter": None}
        for c in self.choices:
            for w in c.split():
                if w not in STOPWORDS and len(w) > 2:
                    seen.setdefault(w, None)
        return " ".join(list(seen)[:limit])

    def score(self, text: str, choice: str) -> float:
        s = max(fuzz.ratio(text, choice), fuzz.token_sort_ratio(text, choice))
        # A full command said inside a slightly longer sentence ("ok now tell me a joke").
        words = text.split()
        cwords = choice.split()
        if len(cwords) >= 2 and re.search(rf"\b{re.escape(choice)}\b", text) \
                and len(cwords) / len(words) >= 0.5:
            s = max(s, 95.0)
        # A fuzzy match must share the command's meaningful words, not just its filler: "what else
        # can you do" scored 80 against "what do you see" on "what/you/do" alone and started the camera.
        if s < 95.0:
            content = [w for w in cwords if w not in STOPWORDS]
            if content:
                hits = sum(1 for w in content if any(fuzz.ratio(w, t) >= 65 for t in words))
                if hits / len(content) < 0.5:
                    s -= 15.0
        return s

    def match(self, transcript: str) -> Match:
        text = normalize(transcript)
        if not text:
            return Match(False, -1, 0.0, "")
        if ABORT_RE.match(text) or (END_RE.search(text) and len(text.split()) <= END_MAX_WORDS):
            return Match(True, -1, 100.0, "cancel")

        best_i, best_s = -1, 0.0
        for i, c in enumerate(self.choices):
            s = self.score(text, c)
            if s > best_s:
                best_i, best_s = i, s
        phrase = self.acts[best_i].text if best_i >= 0 else ""
        if best_s < self.threshold:
            return Match(False, -1, best_s, phrase)
        return Match(True, best_i, best_s, phrase)
