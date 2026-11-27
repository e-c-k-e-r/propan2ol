#!/usr/bin/env python3
"""Builds "voice<TAB>text" case files for compare.py.
usage: make_cases.py golden <g2p.jsonl>   (the text of every golden line, vall-e's language -> espeak voice)
       make_cases.py generated   (numbers, abbreviations, punctuation, mixed scripts, per voice; seeded)
       make_cases.py words <lang> <voice> <dictsource/xx_list>   (every plain word of an espeak word list)"""
import json, re, sys
VOICES = {'en': 'en-us', 'fr': 'fr-fr', 'de': 'de', 'ja': 'ja', 'ko': 'ko', 'zh': 'cmn-latn-pinyin'}
if sys.argv[1] == 'golden':
    for l in open(sys.argv[2], encoding='utf-8'):
        d = json.loads(l)
        t = d['text'].replace('\t', ' ').replace('\n', ' ')
        print(f"{VOICES[d['language']]}\t{t}")
elif sys.argv[1] == 'words':
    voice = sys.argv[3]
    seen = set()
    for l in open(sys.argv[4], encoding='utf-8'):
        w = l.split('//')[0].split()
        if w and re.fullmatch(r"[^\W\d_]+(?:['-][^\W\d_]+)*", w[0]) and w[0] not in seen:
            seen.add(w[0]); print(f"{voice}\t{w[0]}")
elif sys.argv[1] == 'edge':
    # clause-reader edge cases (readclause.c): ellipses, runs of ?!, paragraphs are lines here (the probe reads one line), long clauses,
    # leading / repeated punctuation, brackets and quotes, abbreviations with dots. (Text in scripts that switch to a language whose data isn't embedded, e.g. Armenian or
    # Arabic digits, differs by design: those need a data directory at run time, see the plan.)
    cases = ['Wait... what?!', 'Really?!?! No.', '...', '...hello', 'a. b', 'e.g. this', 'U.S.A.\'s flag', '1.23 and 4.5', '!Speak :path', '(hello) [world] {x}',
             '"Quoted." said he.', 'He said: "go!" Then left; she stayed, quietly.', 'a' * 900, ('word ' * 400) + 'end.', 'x,y;z:w', '— dash — here', '… ellipsis',
             'hello\u00a0world', 'tab\there', 'ﬁne ﬂow', '!!!', '??', 'a-b-c', "it's 'quoted' text", ' ', '', '.', ',']
    for v in ['en-us', 'fr-fr', 'de', 'ja', 'ko', 'cmn-latn-pinyin']:
        for c in cases:
            print(f"{v}\t{c}")
elif sys.argv[1] == 'generated':
    import random
    random.seed(1)
    tpl = ['{n}', '{n}.', '-{n}', '{n}%', '${n}', '{n}.{m}', '{n}th', '{n}st', '{n}:{m}', '{n}/{m}', '#{n}', '{n}-{m}', '{n},{m}', '1{n}',
           'Chapter {n}', '{n} {m}', '{r}', '{r} {r}', 'Mr. Smith', 'Dr. {r}', 'http://example.com/a?b={n}', 'a@b.com', 'USA', 'NASA is great',
           'e-mail', "don't", "can't go", 'co-op', '3.5mm', '😀 {n}', 'Hello — world…', '"quoted" text', '(paren) [br] {n}', 'A.B.C.', 'ABC', 'abc']
    rom = ['IV', 'XII', 'MCMXCIX', 'III', 'XIV']
    mixed = ['Bonjour le monde. Hello world, this is a test.', 'The word Straße and café and naïve.', 'Ich habe einen Computer und a Computer.',
             '안녕하세요 hello 世界', 'こんにちは world', '你好 world 123 abc']
    for v in ['en-us', 'fr-fr', 'de', 'ja', 'ko', 'cmn-latn-pinyin']:
        for _ in range(400):
            n = random.choice([0, 1, 2, 7, 10, 11, 13, 19, 20, 21, 99, 100, 101, 110, 999, 1000, 1999, 2024, 12345, 100000, 1000000,
                               random.randint(0, 10 ** random.randint(1, 12))])
            print(f"{v}\t" + random.choice(tpl).format(n=n, m=random.randint(0, 99), r=random.choice(rom)))
        for t in mixed:
            print(f"{v}\t{t}")
