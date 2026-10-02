# ArtistStudio Stem Lab v0.3

This milestone adds the producer-preview workflow on top of the existing local Demucs separation engine.

## New in v0.3

- Original-track waveform display
- In-app original preview
- Synchronized 4-stem preview mixer
- Per-stem Solo / Mute / Volume
- VST3 previews are routed through the DAW audio channel
- Standalone previews use the computer audio output
- Live Demucs progress when percentage output is available
- Cancel separation without blocking the UI
- Cleaner ASCII-safe Windows labels
- Visible v0.3 build badge
- VS2022 build for broad DAW compatibility
- pluginval host-safety validation in CI

## Still next

- Drag stem WAV files directly into a DAW
- Host-track capture from the VST3 input
- Four dedicated VST3 output buses
- GPU/runtime diagnostics and memory-aware model choice
- Native packaged inference backend that removes the Python/Demucs bootstrap step
