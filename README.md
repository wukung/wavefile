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

## External Libraries

- **Qt 6** (`Core`, `Gui`, `Widgets`): desktop UI, rendering, and event system.
- **FFTW3** *(optional but recommended)*: accelerates spectrogram FFT computation.
  - If FFTW3 is not available, spectrogram rendering falls back to a slower built-in DFT path.

## Build

```bash
cmake -B build
cmake --build build
```

### Tests (optional)

Unit tests use Qt Test and are enabled when **`BUILD_TESTING`** is **ON** (CMake default). To force tests on in CI or scripted builds:

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build
```

To skip building tests:

```bash
cmake -B build -DBUILD_TESTING=OFF
```

## Usage

Run the executable and press **Ctrl+O** to open an audio file.
Press **W** to show waveform view and **S** to show spectrogram view.

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

## Future Development (TODO)

This project is evolving from a waveform viewer into a professional **Audio Debugging & Analysis Tool**. The following features are planned to enhance signal diagnostics and data integrity analysis:

### 1. Advanced Signal Visualization
- [x] **Spectrogram View**: Implemented an FFT-based spectrogram detail mode with time-frequency-amplitude visualization.
- [x] **Microscopic Zooming**: Zooming supports single-sample inspection (`1 px = 1 sample`) with sample-level rendering at maximum zoom.
- [x] **Multi-scale View**: Dual-view system (Overview + Detail) is implemented to correlate long-term signal trends with instantaneous sample changes.

### 2. Quantitative Data Analysis
- [ ] **Statistical Dashboard**: Add a real-time data panel displaying critical metrics:
    - **Peak & RMS Levels**: To monitor amplitude headroom.
    - **Crest Factor**: To analyze the dynamic range and signal impulsiveness.
    - **Zero-Crossing Rate**: To help identify high-frequency noise or signal instability.
- [ ] **Clipping Detection**: Implement visual indicators (e.g., red highlighting) when samples exceed the maximum bit-depth threshold, identifying digital distortion.

### 3. Debugging & Integrity Tools
- [ ] **Format Validation Engine**: Add intelligence to detect misconfigured raw files (e.g., detecting if the chosen sample rate or endianness results in non-natural signal patterns).
- [ ] **Endianness Toggle**: Provide a quick-switch mechanism for Little-endian vs. Big-endian viewing to facilitate rapid debugging of raw data parsing.
- [ ] **Noise Floor Analysis**: Implement tools to measure and visualize the background noise level in silent segments.

## Waveform Rendering

Each horizontal pixel represents a time window of samples. Three layers are drawn per column:

1. **Outer envelope** (dark teal `#277A5C`) — vertical line from minimum to maximum sample value
2. **RMS band** (bright teal `#4BF3A7`) — ±RMS energy centred at the zero line
3. **Centre line** — drawn on top of the waveform

## Project Structure

```
wavefile/
├── tests/
│   └── test_wavefile.cpp   # Qt Test (WaveFile WAV/raw); run via `ctest`
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
