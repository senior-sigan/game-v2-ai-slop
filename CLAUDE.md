# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Raylib docs

[Raylib API Docs Index]|root: docs/raylib|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning|Raylib version: 5.5|api:{INDEX.md,01_defines.txt,02_structs.txt,03_enums.txt,04_core_window.txt,05_input.txt,06_shapes.txt,07_textures.txt,08_text.txt,09_models.txt,10_audio.txt}

## Build Commands

```bash
make build          # cmake configure + build
make test           # build + run with test.lua
make run            # run without test script
make tidy           # clang-tidy (all warnings are errors)
make clean          # rm -rf build
```

Binary: `./build/Game [script.lua]` — with Lua script runs test mode, without runs normal game.
