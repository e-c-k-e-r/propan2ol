#!/bin/sh
# the whole parity suite against libespeak-ng: tests/parity/run_all.sh <build dir> [golden g2p.jsonl] [espeak-ng clone]
B="${1:-build}"; [ -x "$B/p2o_probe" ] || { echo "no $B/p2o_probe: build first" >&2; exit 2; }; G="${2:-/mnt/tmp/models/golden/g2p/g2p.jsonl}"; D="${3:-$HOME/Programs/espeak-ng}/dictsource"
H="$(dirname "$0")"; T=$(mktemp -d); rc=0
run() { printf '%s: ' "$1"; "$H/compare.py" "$B/p2o_probe" "$B/ref_probe" "$T/$1.tsv" 5 || rc=1; }
[ -f "$G" ] && { "$H/make_cases.py" golden "$G" > "$T/golden.tsv"; run golden; }
"$H/make_cases.py" generated > "$T/generated.tsv"; run generated
"$H/make_cases.py" edge > "$T/edge.tsv"; run edge
for p in en:en-us fr:fr-fr de:de ko:ko cmn:cmn-latn-pinyin; do
	"$H/make_cases.py" words "${p%%:*}" "${p##*:}" "$D/${p%%:*}_list" > "$T/words_${p%%:*}.tsv"; run "words_${p%%:*}"
done
rm -rf "$T"; exit $rc
