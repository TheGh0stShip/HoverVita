# Contributing to HoverVita

Thanks for helping! There's room for reverse engineers, C programmers, Vita
testers and artists.

## Ground rules

1. **No Microsoft material in the repo.** That means no `hover.exe` or DLLs,
   no game data (`.maz`, `.tex`, `.wav`, `.muz`, extracted PNGs or MIDIs), and
   no Ghidra output (`re/out/`, decompiled C, Ghidra projects). `.gitignore`
   covers the usual paths, but check your diff.
2. **Write code, don't paste decompiler output.** Work out what a function
   does, then implement it cleanly. Name things, use structs, add comments.
   Reference the original address in a comment
   (`/* CMerlinTexture::Serialize, hover.exe 0x412980 */`) so others can check it.
3. **Document what you find.** Formats go in `docs/formats.md`, names in
   `re/symbols.csv`, and open questions in `TODO(re):` comments.

## Getting started

- Build and run the PC version (see the [README](README.md#building)).
- Read [docs/architecture.md](docs/architecture.md) and
  [docs/reverse-engineering.md](docs/reverse-engineering.md).
- Pick a box from [docs/roadmap.md](docs/roadmap.md) and open an issue or draft
  PR to say you're on it.

## Code style

- C99, 4-space indent, braces on the function line for control flow and on a
  new line for functions. Run `clang-format` (`.clang-format` is in the repo).
- `src/engine/` must not include SDL, `<windows.h>`, or Vita headers.
- Builds must stay warning-free (`-Wall -Wextra`) on both PC and Vita.
- Python tools: standard library only, so they run anywhere.

## Pull requests

- Keep PRs focused. RE notes plus the code using them in one PR is fine.
- CI builds the Vita VPK and the PC build. Both must pass.
- If you can, test on a real Vita and say so in the PR (model and firmware).
- Tests that need game data must exit 77 when it is missing, so CI skips them.
