# re/: reverse-engineering workspace

| Path | |
|---|---|
| `symbols.csv` | **Shared names for hover.exe**: functions, globals, classes. Edit this. |
| `setup.sh` | Import + analyse hover.exe into a local Ghidra project and apply symbols |
| `decomp.sh` | Decompile chosen addresses from the command line |
| `scripts/ApplySymbols.java` | Ghidra script: apply `symbols.csv` |
| `scripts/Decompile.java` | Ghidra script used by `decomp.sh` |
| `scripts/ExportDecomp.java` | Ghidra script: dump every function to `out/hover_decomp.c` |
| `scripts/find_runtime_classes.py` | Find MFC `CRuntimeClass` records |
| `ghidra_proj/`, `out/` | Local only, gitignored. They contain Microsoft code. |

See [../docs/reverse-engineering.md](../docs/reverse-engineering.md).
