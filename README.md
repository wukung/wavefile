# wavefile

A Qt6 desktop application that visualises audio files as a CoolEdit-style waveform.

## Features

- **CoolEdit-style rendering**: per-pixel min/max envelope (outer) + RMS energy band (inner)
- Open WAV files (8-bit and 16-bit PCM, A-law, μ-law) via file dialog (**Ctrl+O**)
- Open raw headerless PCM files (`.raw`, `.pcm`, `.bin`) — prompts for format on open
- Raw PCM format options: sample rate, bit depth (8/16), encoding (Linear / A-law / μ-law), channels (mono/stereo), byte order (little/big-endian)
- Mono and stereo support (stereo renders first channel)
- Correctly skips non-`data` WAV chunks (LIST, JUNK, fact, etc.)

## Requirements

- CMake ≥ 3.16
- Qt 6 (Core, Gui, Widgets)
- A C++17-capable compiler

## Build

```bash
cmake -B build
cmake --build build
```

## Usage

Run the executable and press **Ctrl+O** to open an audio file.

| File type | Behaviour |
|-----------|-----------|
| `.wav` | Parsed automatically; format detected from header |
| `.raw` / `.pcm` / `.bin` | Format dialog appears to collect sample rate, bit depth, encoding, channels, byte order |

### Supported WAV format types

| formattype | Encoding |
|-----------|----------|
| 1 | Linear PCM |
| 6 | A-law (G.711) |
| 7 | μ-law (G.711) |

## Waveform Rendering

Each horizontal pixel represents a time window of samples. Three layers are drawn per column:

1. **Outer envelope** (dark teal `#277A5C`) — vertical line from minimum to maximum sample value
2. **RMS band** (bright teal `#4BF3A7`) — ±RMS energy centred at the zero line
3. **Centre line** — drawn on top of the waveform

## Project Structure

```
wavefile/
├── include/
│   ├── WaveFile.h          # WAV/raw PCM parser, G.711 decoders, data storage
│   ├── widget.h            # Qt widget declaration
│   └── pcmformatdialog.h   # Raw PCM format dialog declaration
├── src/
│   ├── main.cpp            # Application entry point
│   ├── widget.cpp          # Waveform rendering and user interaction
│   ├── widget.ui           # Qt Designer UI file
│   └── pcmformatdialog.cpp # Raw PCM format dialog implementation
└── CMakeLists.txt
```
