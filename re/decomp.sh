#!/bin/sh
# Decompile functions by address: re/decomp.sh 4128f0 419de0 > out.c
# Needs GHIDRA_HOME and a project created by re/setup.sh.
set -e
RE=$(cd "$(dirname "$0")" && pwd)
OUT=$(mktemp)
"$GHIDRA_HOME/support/analyzeHeadless" "$RE/ghidra_proj" Hover -process hover.exe -noanalysis \
    -scriptPath "$RE/scripts" -postScript Decompile.java "$OUT" "$@" >/dev/null 2>&1
cat "$OUT"; rm -f "$OUT"
