# ArtistStudio NEONRACK FX

A separate ArtistStudio multi-effects VST3 rack for Windows.

## v0.1 modules

- **NEON FILTER** — resonant low-pass with pre-drive
- **RIFT DISTORTION** — morphing soft-clip / wavefold distortion
- **AURA SATURATOR** — warm asymmetric saturation with air shaping
- **CHORUS MATRIX** — stereo modulation / widening
- **PRISM DELAY** — stereo modulated delay with spread and saturated feedback
- **ORBIT EQ** — musical low / focus / air shaping EQ

## Rack workflow

NEONRACK is intentionally separate from Stem Lab.

There are six rack slots. Each slot can be:

- empty
- assigned to any available module
- bypassed without removing it
- removed
- moved up or down in the signal chain

Each effect type can appear once in the rack in v0.1. The visible rack order is the audio processing order.

The selected rack module opens its detailed controls on the right. Global Mix and Output remain available independently of the selected module.

## Build

```powershell
cmake -S fx-rack -B fx-rack/build -G "Visual Studio 18 2026" -A x64
cmake --build fx-rack/build --config Release --parallel 2
```

The VST3 bundle is generated under `fx-rack/build`.

## Install on Windows

Copy:

```text
ArtistStudio NEONRACK FX.vst3
```

to:

```text
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in Ableton Live, FL Studio, REAPER, Cubase or another VST3 host.

## Next rack modules

Planned additions include Reverb, Phaser, Flanger, Compressor, multiband/OTT-style dynamics, Dimension, Pitch/Width, Trance Gate, Transient Shaper and a modulation system.

A later version can add drag-and-drop rack ordering, macro controls, tempo-sync, presets and modulation routing.
