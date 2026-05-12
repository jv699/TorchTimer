# TorchTimer — Pebble Watch App

## What this is
- Native Pebble SDK 3 watch app (not a watchface)
- Targets Basalt (Pebble Time), Chalk (Pebble Time Round), Diorite (Pebble 2)
- Single source file: `src/main.c`
- Entry point: `main()` → `init()` → `app_event_loop()` → `deinit()`

## Build & deploy
- Requires Pebble SDK installed and `pebble` CLI on PATH
- `pebble build` — compile
- `pebble install` — deploy to connected emulator or device
- `pebble install --emulator basalt` — run on Basalt emulator
- `pebble logs` — view app logs

## Project structure
- `appinfo.json` — app manifest (UUID, capabilities, platforms, resources)
- `package.json` — Pebble package metadata (minimal)
- `src/main.c` — all app logic (window, time display, menu layer)
- `build/` — generated build artifacts (gitignored)

## Conventions
- No test framework or CI configured
- No linting or formatting tooling configured
- All C code lives in `src/`; no headers currently exist
