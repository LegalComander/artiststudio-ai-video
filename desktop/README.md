# ArtistStudio Stem Lab Desktop

Standalone Windows-first version of ArtistStudio Stem Lab.

## v0.2 scope

- Blue/cyan neon JUCE desktop interface
- Drag/drop or choose WAV, MP3, FLAC, AIFF
- Local BPM estimation
- Local musical-key estimation
- Track length + sample-rate analysis
- Local Demucs 4-stem separation
- Vocals / drums / bass / other WAV output
- Fast `htdemucs` and maximum-quality `htdemucs_ft` modes
- No Replicate/API requirement for desktop separation
- In-app **Install Local AI** setup
- Private ArtistStudio-managed Python environment under the user's application-data folder
- Automatic Demucs 4.1.0 installation and verification
- Automatic Python 3.12 installation through Windows Package Manager when Python is missing and `winget` is available

## Architecture

```text
Desktop UI (JUCE)
  -> TrackAnalyzer (C++ / JUCE DSP)
  -> StemEngine
       -> ArtistStudio-managed local Python environment
       -> Demucs 4.1.0
       -> ONNX/native engine later
  -> WAV stems in Documents/ArtistStudio Stem Lab/Stems
```

The stem engine is isolated from the UI so it can later be reused by the VST3 target and eventually swapped for a native ONNX Runtime backend without redesigning the product.

## First-run user flow

1. Open **ArtistStudio Stem Lab**.
2. Press **Install Local AI** once.
3. Stem Lab creates a private AI environment and installs Demucs locally.
4. The button changes to **AI Engine Ready** after verification.
5. Drop in a track and press **Separate 4 Stems**.

No Replicate token or cloud account is required for desktop separation. Internet access is required for the one-time local AI dependency download.

If Python is already installed, Stem Lab uses it only to create its own private environment. It does not install Demucs into the user's normal Python environment.

## Windows build

Requirements:

- Visual Studio with Desktop development with C++
- CMake 3.22+
- Internet access during configure so CMake can fetch JUCE 9.0.2

From the repository root on a current Visual Studio 2026 system:

```powershell
cmake -S desktop -B desktop/build -G "Visual Studio 18 2026" -A x64
cmake --build desktop/build --config Release
```

The executable is created under the CMake artefacts directory in `desktop/build`.

## Output

Completed stems are written to:

```text
Documents/ArtistStudio Stem Lab/Stems/<model>/<track name>/
```

with:

- `vocals.wav`
- `drums.wav`
- `bass.wav`
- `other.wav`

## Next milestones

1. Add original/stem audio preview with solo and mute.
2. Add live separation progress and a Cancel button.
3. Add GPU/runtime diagnostics and memory-aware settings for laptop GPUs.
4. Add drag-export into DAWs.
5. Create the VST3 target using the same analyzer and stem engine.
6. Replace the Python runner with a native/bundled inference backend when the model/runtime packaging is production-ready.

## Notes

The built-in BPM/key detector is an initial local implementation. It should be tested against a labelled validation set before being presented as production-grade analysis.
