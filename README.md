# wavefile

A Qt6 desktop application that visualises WAV audio files as a waveform.

## Features

- Open 8-bit and 16-bit PCM WAV files via file dialog (Ctrl+O)
- Mono and stereo support (stereo files are rendered as the first channel)
- Anti-aliased waveform rendering using QPainterPath
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

Run the executable and press **Ctrl+O** to open a WAV file.
Only 8-bit and 16-bit PCM WAV files are supported.

## Project Structure

| File | Description |
|------|-------------|
| `WaveFile.h` | WAV parser: chunk scanning, decoding, data storage |
| `widget.h/cpp` | Qt widget: waveform rendering and user interaction |
| `widget.ui` | Qt Designer UI file |
| `main.cpp` | Application entry point |
