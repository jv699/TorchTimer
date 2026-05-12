# TorchTimer

A native Pebble SDK 3 watch app for managing multiple countdown timers with light-themed presets.

## Features

- **Live clock display** at the top of the main screen
- **Multiple concurrent timers** (up to 10) with real-time countdown
- **Preset timers** themed around light sources:
  - Torch (1h)
  - Lantern (1h)
  - Oil Lamp (2h)
  - Candle (10m)
- **Custom timer** with adjustable duration (1-99 minutes)
- **Button controls** for custom timer setup (Up/Down to adjust, Select to start, Back to cancel)

## Requirements

- Pebble SDK 3 installed
- `pebble` CLI on PATH

## Build & Deploy

```bash
pebble build                          # Compile the app
pebble install                        # Deploy to connected device/emulator
pebble install --emulator basalt      # Run on Basalt (Pebble Time) emulator
pebble install --emulator chalk       # Run on Chalk (Pebble Time Round) emulator
pebble install --emulator diorite     # Run on Diorite (Pebble 2) emulator
pebble logs                           # View app logs
```

## Supported Platforms

- Basalt (Pebble Time)
- Chalk (Pebble Time Round)
- Diorite (Pebble 2)

## Project Structure

```
TorchTimer/
├── appinfo.json      # App manifest (UUID, capabilities, platforms)
├── package.json      # Package metadata
├── src/
│   └── main.c        # All app logic (windows, menus, timers)
└── build/            # Generated build artifacts (gitignored)
```

## Usage

1. Launch TorchTimer from your Pebble watch
2. The main screen shows the current time and any active timers
3. Select **+ Add Light** to choose a preset or create a custom timer
4. Active timers display a live countdown; completed timers show "Done"

## Version

1.0
