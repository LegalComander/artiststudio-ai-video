# ArtistStudio Stem Lab v0.3

Producer workflow upgrade for the Windows standalone app and VST3 beta.

## Added
- Original track waveform preview
- Original-track playback
- Four-stem preview mixer
- Vocal / drums / bass / other solo and mute
- Per-stem volume controls
- Separation progress UI
- Cancel separation
- Open output folder
- Shared preview engine architecture for standalone and VST3
- Host-safe VST3 validation through pluginval

Heavy stem separation remains outside the real-time DAW audio callback.
