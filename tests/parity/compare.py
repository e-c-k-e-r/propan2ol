#!/usr/bin/env python3
"""Runs the port's probe and the libespeak-ng reference probe over a file of "voice<TAB>text" lines and diffs the output.
usage: compare.py build/p2o_probe build/ref_probe cases.tsv [max_shown]"""
import subprocess, sys
port, ref, cases = sys.argv[1:4]
shown = int(sys.argv[4]) if len(sys.argv) > 4 else 20
data = open(cases, 'rb').read()
def run(exe):
    return subprocess.run([exe], input=data, capture_output=True, check=True).stdout.split(b'\n')
a, b, lines = run(port), run(ref), data.split(b'\n')
bad = [i for i in range(len(lines)) if lines[i] and a[i] != b[i]]
for i in bad[:shown]:
    print(f"{lines[i].decode()!r}\n  port: {a[i].decode()}\n  ref:  {b[i].decode()}")
n = sum(1 for l in lines if l)
print(f"{n - len(bad)}/{n} match")
sys.exit(1 if bad else 0)
