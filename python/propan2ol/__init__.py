"""propan2ol: text to IPA, a standalone port of the phonemizing part of espeak-ng, with the processing of phonemizer 3.3's espeak
backend on top. Over the shared library's C ABI (ctypes: no compiled extension module).

    import propan2ol
    propan2ol.phonemize("hello, world", "en-us", preserve_punctuation=True, with_stress=True, strip=True)

`EspeakBackend` mirrors phonemizer's class of the same name, so code written for phonemizer switches with an import change:

    from propan2ol import EspeakBackend, Separator
    EspeakBackend("en-us", preserve_punctuation=True, with_stress=True).phonemize(["hello, world"], strip=True)
"""
import ctypes
import os
import sys
from typing import List, Optional, Sequence

__all__ = ["phonemize", "languages", "EspeakBackend", "Separator", "Phonemizer", "DEFAULT_PROFILE"]

DEFAULT_PROFILE = "espeak-1.52"

_LANGUAGE_SWITCH = {"keep-flags": 0, "remove-flags": 1, "remove-utterance": 2}


def _find_library() -> str:
    name = {"win32": "propan2ol.dll", "cygwin": "cygpropan2ol.dll", "darwin": "libpropan2ol.dylib"}.get(sys.platform, "libpropan2ol.so")
    here = os.path.dirname(os.path.abspath(__file__))
    candidates = [os.environ.get("PROPAN2OL_LIBRARY", ""), os.path.join(here, name)]
    root = os.path.dirname(os.path.dirname(here))   # a source checkout: python/propan2ol -> the repository
    candidates += [os.path.join(root, d, name) for d in ("build", "build/Release", "build/Debug")]
    for c in candidates:
        if c and os.path.isfile(c):
            return c
    raise ImportError(f"cannot find {name}: put it next to the package, or set PROPAN2OL_LIBRARY")


class _Options(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_size_t),
        ("preserve_punctuation", ctypes.c_int),
        ("with_stress", ctypes.c_int),
        ("strip", ctypes.c_int),
        ("language_switch", ctypes.c_int),
        ("punctuation_marks", ctypes.c_char_p),
        ("tie", ctypes.c_char_p),
        ("word_separator", ctypes.c_char_p),
        ("syllable_separator", ctypes.c_char_p),
        ("phone_separator", ctypes.c_char_p),
    ]


_lib = ctypes.CDLL(_find_library())
_lib.p2o_create.restype = ctypes.c_void_p
_lib.p2o_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.p2o_destroy.restype = None
_lib.p2o_destroy.argtypes = [ctypes.c_void_p]
_lib.p2o_phonemize.restype = ctypes.c_void_p   # not c_char_p: the buffer is ours to free
_lib.p2o_phonemize.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(_Options)]
_lib.p2o_phonemize_lines.restype = ctypes.POINTER(ctypes.c_void_p)
_lib.p2o_phonemize_lines.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_size_t, ctypes.POINTER(_Options), ctypes.POINTER(ctypes.c_size_t)]
_lib.p2o_free_lines.restype = None
_lib.p2o_free_lines.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_size_t]
_lib.p2o_free.restype = None
_lib.p2o_free.argtypes = [ctypes.c_void_p]
_lib.p2o_languages.restype = ctypes.c_void_p
_lib.p2o_languages.argtypes = [ctypes.c_char_p]
_lib.p2o_last_error.restype = ctypes.c_char_p
_lib.p2o_last_error.argtypes = []


def _take(ptr) -> Optional[str]:
    """The C string at ptr as str, freeing it."""
    if not ptr:
        return None
    try:
        return ctypes.string_at(ptr).decode("utf-8")
    finally:
        _lib.p2o_free(ptr)


def languages(profile: str = DEFAULT_PROFILE) -> List[str]:
    """The language codes available in a profile."""
    return (_take(_lib.p2o_languages(profile.encode())) or "").split("\n")


class Separator:
    """phonemizer's Separator: boundary tokens in the output (empty = none; the non-empty ones must differ)."""

    def __init__(self, word: str = " ", syllable: Optional[str] = None, phone: Optional[str] = None):
        tokens = [t for t in (phone, syllable, word) if t]
        if len(tokens) != len(set(tokens)):
            raise ValueError(f'illegal separator with word="{word}", syllable="{syllable}" and phone="{phone}", must be all differents if not empty')
        self.word = str(word) if word else ""
        self.syllable = str(syllable) if syllable else ""
        self.phone = str(phone) if phone else ""

    def __eq__(self, other):
        return isinstance(other, Separator) and (self.phone, self.syllable, self.word) == (other.phone, other.syllable, other.word)

    def __repr__(self):
        return f'(phone: "{self.phone}", syllable: "{self.syllable}", word: "{self.word}")'


class Phonemizer:
    """One language under one profile. Not safe to call from two threads at once; separate instances are independent."""

    def __init__(self, language: str, profile: str = DEFAULT_PROFILE):
        self._handle = _lib.p2o_create(profile.encode(), language.encode())
        if not self._handle:
            raise RuntimeError((_lib.p2o_last_error() or b"").decode() or "cannot create a phonemizer")
        self.language = language
        self.profile = profile

    def __del__(self):
        handle, self._handle = getattr(self, "_handle", None), None
        if handle:
            _lib.p2o_destroy(handle)

    def _options(self, preserve_punctuation, with_stress, strip, tie, language_switch, punctuation_marks, separator) -> _Options:
        if language_switch not in _LANGUAGE_SWITCH:
            raise RuntimeError(f'mode "{language_switch}" invalid, must be in {", ".join(_LANGUAGE_SWITCH)}')
        if tie is True:
            tie = "͡"   # phonemizer's default tie
        elif tie:
            tie = str(tie)
            if len(tie) != 1:
                raise RuntimeError(f"explicit tie must be a single charcacter but is {tie}")
        sep = separator or Separator()
        return _Options(
            struct_size=ctypes.sizeof(_Options), preserve_punctuation=int(preserve_punctuation), with_stress=int(with_stress),
            strip=int(strip), language_switch=_LANGUAGE_SWITCH[language_switch],
            punctuation_marks=punctuation_marks.encode() if punctuation_marks else None,
            tie=tie.encode() if tie else None, word_separator=sep.word.encode(), syllable_separator=sep.syllable.encode(),
            phone_separator=sep.phone.encode())

    def phonemize_lines(self, lines: Sequence[str], preserve_punctuation: bool = False, with_stress: bool = False, strip: bool = False,
                        tie=False, language_switch: str = "keep-flags", punctuation_marks: Optional[str] = None,
                        separator: Optional[Separator] = None) -> List[str]:
        """phonemizer's phonemize( lines ): the lines are handled together, so punctuation is restored across them as it is there."""
        opts = self._options(preserve_punctuation, with_stress, strip, tie, language_switch, punctuation_marks, separator)
        arr = (ctypes.c_char_p * len(lines))(*[l.encode("utf-8") for l in lines])
        n = ctypes.c_size_t(0)
        out = _lib.p2o_phonemize_lines(self._handle, arr, len(lines), ctypes.byref(opts), ctypes.byref(n))
        if not out:
            raise RuntimeError((_lib.p2o_last_error() or b"").decode() or "phonemization failed")
        try:
            return [ctypes.string_at(out[i]).decode("utf-8") for i in range(n.value)]
        finally:
            _lib.p2o_free_lines(out, n.value)

    def phonemize(self, text: str, preserve_punctuation: bool = False, with_stress: bool = False, strip: bool = False,
                  tie=False, language_switch: str = "keep-flags", punctuation_marks: Optional[str] = None,
                  separator: Optional[Separator] = None) -> str:
        opts = self._options(preserve_punctuation, with_stress, strip, tie, language_switch, punctuation_marks, separator)
        out = _take(_lib.p2o_phonemize(self._handle, text.encode("utf-8"), ctypes.byref(opts)))
        if out is None:
            raise RuntimeError((_lib.p2o_last_error() or b"").decode() or "phonemization failed")
        return out


def phonemize(text: str, language: str, preserve_punctuation: bool = False, with_stress: bool = False, strip: bool = False,
              tie=False, language_switch: str = "keep-flags", punctuation_marks: Optional[str] = None,
              separator: Optional[Separator] = None, profile: str = DEFAULT_PROFILE) -> str:
    """The phonemes of `text` in `language` (an espeak code: en-us, fr-fr, de, ja, ko, cmn-latn-pinyin)."""
    return Phonemizer(language, profile).phonemize(text, preserve_punctuation, with_stress, strip, tie, language_switch, punctuation_marks, separator)


class EspeakBackend:
    """phonemizer's EspeakBackend(language, ...).phonemize([text], separator, strip), over this library."""

    def __init__(self, language: str, punctuation_marks: Optional[str] = None, preserve_punctuation: bool = False, with_stress: bool = False,
                 tie=False, language_switch: str = "keep-flags", words_mismatch: str = "ignore", logger=None, profile: str = DEFAULT_PROFILE):
        if words_mismatch != "ignore":
            raise RuntimeError('only words_mismatch="ignore" is supported')
        self.language = language
        self._phonemizer = Phonemizer(language, profile)
        self._kw = dict(preserve_punctuation=preserve_punctuation, with_stress=with_stress, tie=tie, language_switch=language_switch,
                        punctuation_marks=punctuation_marks)

    @staticmethod
    def name() -> str:
        return "espeak"

    @classmethod
    def supported_languages(cls):
        return {code: code for code in languages()}

    def phonemize(self, text: Sequence[str], separator: Optional[Separator] = None, strip: bool = False, njobs: int = 1) -> List[str]:
        if isinstance(text, str):
            raise RuntimeError("input text to phonemize() is str but it must be list of str")
        return self._phonemizer.phonemize_lines(list(text), strip=strip, separator=separator, **self._kw)
