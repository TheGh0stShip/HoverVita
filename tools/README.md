# Tools

Python 3 tools, standard library only. Each one runs on files from your own
Hover! copy. Don't commit their output.

| Tool | Purpose |
|---|---|
| `hoverfmt.py` | Reference readers: CArchive, palette, CMerlinTexture, mazes (`read_maz_file`). Import it in your own scripts. |
| `tex2png.py mazes/text1.tex out/` | Dump every texture in a set to PNG (alpha = transparent spans). |
| `maz2svg.py mazes/maze1.maz maze1.svg` | Top-down map: walls (raised ones in gold), spawns, flags, pods. Hover over items for details. |
| `muz2mid.py music1.muz music1.mid` | Convert music to a Standard MIDI File. |
| `make_livearea.py` | Regenerate the placeholder art in `sce_sys/`. |
