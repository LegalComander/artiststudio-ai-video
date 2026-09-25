# ArtistStudio NEONRACK FX

A separate ArtistStudio modular multi-effects VST3 rack for Windows.

## v0.2 modules

- **NEON FILTER** — resonant low-pass with pre-drive
- **RIFT DISTORTION** — morphing soft-clip / wavefold distortion
- **AURA SATURATOR** — warm asymmetric saturation with air shaping
- **CHORUS MATRIX** — stereo modulation / widening
- **PRISM DELAY** — stereo modulated delay with spread and saturated feedback
- **ORBIT EQ** — musical low / focus / air shaping EQ
- **PHOTON PHASER** — animated stereo phase movement
- **SPACE REVERB** — wide room/space reverb with freeze
- **PULSE COMPRESSOR** — parallel-ready compressor with makeup gain

## Rack workflow

NEONRACK is intentionally separate from Stem Lab.

There are six rack slots. Each slot can be:

- empty
- assigned to any available module
- bypassed without removing it
- removed
- moved with the ▲ / ▼ controls
- **dragged by the ≡ handle to a new position**

Each effect type can appear once in the rack. The visible rack order is the actual audio processing order.

The selected rack module opens its detailed controls on the right.

## Global controls

v0.2 adds four live macros that influence multiple modules at once:

- **DRIVE** — increases saturation/distortion energy
- **SPACE** — increases delay/reverb/chorus space
- **MOTION** — increases modulation depth/rate/spread
- **TONE** — shifts brightness and tonal shaping

Global Mix and Output remain independent.

## Factory presets

- INIT / CLEAN
- DNB IMPACT
- VOCAL NEON
- BASS MELTDOWN
- DREAM SPACE
- MASTER GLOW

Presets set both the rack order and key effect values, while remaining fully editable.

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

## Next ideas

Potential next modules include Flanger, Dimension, multiband/OTT-style dynamics, Pitch/Width, Trance Gate, Transient Shaper and tempo-synced modulation routing.
