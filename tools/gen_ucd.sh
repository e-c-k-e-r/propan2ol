#!/bin/sh
# builds tools/gen_ucd.cpp against the espeak-ng clone's ucd-tools and prints src/ucd_tables.cpp
set -e
SRC="${ESPEAK_SRC:-$HOME/Programs/espeak-ng}/src/ucd-tools"
T=$(mktemp -d)
for f in "$SRC"/src/*.c; do gcc -O1 -I"$SRC/src/include" -c "$f" -o "$T/$(basename "$f").o"; done
g++ -std=c++17 -O1 -I"$SRC/src/include" "$(dirname "$0")/gen_ucd.cpp" "$T"/*.o -o "$T/gen"
"$T/gen"
rm -rf "$T"
