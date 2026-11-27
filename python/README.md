# propan2ol

Text to IPA: a standalone port of the phonemizing part of espeak-ng 1.52.0, with the processing of phonemizer 3.3's espeak backend on top.
Python over the shared library's C ABI (ctypes).

```python
import propan2ol
propan2ol.phonemize("hello, world", "en-us", preserve_punctuation=True, with_stress=True, strip=True)

# phonemizer's class, over this library
from propan2ol import EspeakBackend
EspeakBackend("en-us", preserve_punctuation=True, with_stress=True).phonemize(["hello, world"], strip=True)
```

Languages: en-us, fr-fr, de, ja, ko, cmn-latn-pinyin. License: GPL-3.0-or-later.
