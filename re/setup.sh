#!/bin/sh
# Imports and analyzes hover.exe into a local Ghidra project, applies symbols.csv,
# and exports the full decompilation to re/out/. Usage: re/setup.sh path/to/hover.exe
set -e
: "${GHIDRA_HOME:?set GHIDRA_HOME to your Ghidra install}"
RE=$(cd "$(dirname "$0")" && pwd)
EXE=$(realpath "${1:-$RE/../hover/hover.exe}")
mkdir -p "$RE/ghidra_proj" "$RE/out"
"$GHIDRA_HOME/support/analyzeHeadless" "$RE/ghidra_proj" Hover -import "$EXE" -overwrite \
    -scriptPath "$RE/scripts" \
    -postScript ApplySymbols.java "$RE/symbols.csv" \
    -postScript ExportDecomp.java "$RE/out" > "$RE/out/analyze.log" 2>&1
echo "Decompilation written to $RE/out/hover_decomp.c"
