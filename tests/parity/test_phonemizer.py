#!/usr/bin/env python3
"""The phonemizer layer against phonemizer 3.3's EspeakBackend itself, on the same espeak 1.52.0: every option combination over
texts in every language, single lines and batches. Needs phonemizer importable (a venv with the wheel) and the built shared library.
usage: test_phonemizer.py [golden g2p.jsonl] [max texts per language]"""
import itertools, json, os, random, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
import propan2ol
from phonemizer.backend import EspeakBackend as Ref
from phonemizer.separator import Separator as RefSep

golden = sys.argv[1] if len(sys.argv) > 1 else "/mnt/tmp/models/golden/g2p/g2p.jsonl"
per_lang = int(sys.argv[2]) if len(sys.argv) > 2 else 40
VOICES = {"en": "en-us", "fr": "fr-fr", "de": "de", "ja": "ja", "ko": "ko", "zh": "cmn-latn-pinyin"}
random.seed(7)

EDGE = ["", " ", ",", " , ", "...", "Hello, world!", "  leading and trailing  ", "a,b;c:d", "(one) [two] {three}", '"quoted" text', "¡Hola! ¿Qué tal?",
        "end.", ".start", "a — b … c", "tab\there", "line\nbreak", "x  y", "J'aime le football, c'est super.", "Das ist ein Test, oder?",
        "word-word don't it's", "12, 34. 56!", "?!", "«guillemets» “curly”", "no punctuation at all", "(a)(b)(c)", "a , , b"]
texts = {v: list(EDGE) for v in VOICES.values()}
if os.path.exists(golden):
    bylang = {}
    for l in open(golden, encoding="utf-8"):
        d = json.loads(l)
        bylang.setdefault(d["language"], []).append(d["text"])
    for k, v in bylang.items():
        texts[VOICES[k]] += random.sample(v, min(per_lang, len(v)))

seps = [(None, None), (RefSep(word="|", phone="-"), propan2ol.Separator(word="|", phone="-")),
        (RefSep(word="  ", syllable="+", phone="."), propan2ol.Separator(word="  ", syllable="+", phone=".")),
        (RefSep(word="", phone=" "), propan2ol.Separator(word="", phone=" "))]
bad = total = 0
for lang, tx in texts.items():
    for pp, ws, tie, ls in itertools.product([False, True], [False, True], [False, True, "~"], ["keep-flags", "remove-flags", "remove-utterance"]):
        kw = dict(preserve_punctuation=pp, with_stress=ws, tie=tie, language_switch=ls)
        ref, ours = Ref(lang, **kw), propan2ol.EspeakBackend(lang, **kw)
        for strip in (False, True):
            for rs, os_ in seps if (ls == "keep-flags" and tie is False) else seps[:2]:
                batches = [[t] for t in tx] + [random.sample(tx, 4) for _ in range(8)]
                for batch in batches:
                    total += 1
                    try:
                        want = ref.phonemize(batch, separator=rs, strip=strip) if rs else ref.phonemize(batch, strip=strip)
                    except Exception as e:   # phonemizer raising (e.g. ties with a separator it rejects): ours must raise too
                        want = type(e).__name__
                    try:
                        got = ours.phonemize(batch, separator=os_, strip=strip) if os_ else ours.phonemize(batch, strip=strip)
                    except Exception as e:
                        got = type(e).__name__
                    if want != got:
                        bad += 1
                        if bad <= 10:
                            print(f"MISMATCH {lang} {kw} strip={strip} sep={rs}\n  in:   {batch!r}\n  want: {want!r}\n  got:  {got!r}")
print(f"{total - bad}/{total} match")
sys.exit(1 if bad else 0)
