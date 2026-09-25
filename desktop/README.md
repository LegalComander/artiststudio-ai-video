# ArtistStudio Stem Lab Desktop

Standalone Windows-first version of ArtistStudio Stem Lab.

## v0.1 scope

- Blue/cyan neon JUCE desktop interface
- Drag/drop or choose WAV, MP3, FLAC, AIFF
- Local BPM estimation
- Local musical-key estimation
- Track length + sample-rate analysis
- Local Demucs 4-stem separation
- Vocals / drums / bass / other WAV output
- Fast `htdemucs` and maximum-quality `htdemucs_ft` modes
- No Replicate/API requirement for desktop separation

## Architecture

```text
Desktop UI (JUCE)
  -> TrackAnalyzer (C++ / JUCE DSP)
  -> StemEngine
       -> local Python/Demucs for v0.1
       -> ONNX/native engine later
  -> WAV stems in Documents/ArtistStudio Stem Lab/Stems
```

The stem engine is deliberately isolated so the temporary Python/Demucs runner can later be replaced with ONNX Runtime or another native inference backend without redesigning the UI.

## Windows build

Requirements:

- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- Internet access during configure so CMake can fetch JUCE 9.0.2

From the repository root:

```powershell
cmake -S desktop -B desktop/build -G "Visual Studio 17 2022" -A x64
cmake --build desktop/build --config Release
```

The executable is created under the CMake artefacts directory in `desktop/build`.

## Enable local AI separation

For the first development release, Demucs is launched locally. Run:

```powershell
powershell -ExecutionPolicy Bypass -File desktop/scripts/install-demucs.ps1
```

or manually:

```powershell
py -3 -m pip install -U demucs
```

Then start ArtistStudio Stem Lab, load a track and press **Separate 4 Stems**.

## Next milestones

1. CI-build downloadable Windows EXE.
2. Add original/stem audio preview, solo and mute.
3. Add progress reporting/cancel for Demucs.
4. Bundle the model/runtime so users do not need Python.
5. Reuse the engine/UI system for the VST3 target.
6. Add drag-export into DAWs and multi-output VST routing.

## Notes

The built-in BPM/key detector is an initial local implementation. It should be tested against a labelled validation set before being presented as production-grade analysis.
