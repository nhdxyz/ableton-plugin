# Nate VST Factory Presets

Factory presets are repo-managed `.natevstpreset` XML files in:

`Resources/Factory Presets`

The first pack includes UK garage and broader club workflow starters:

- `UKG 2-Step Bass`
- `UKG Shuffle Bass`
- `UKG Dred Bass`
- `UKG Organ Stab`
- `UKG Chord Stab`
- `UKG Bell Pluck`
- `UKG Vocal Chop Starter`
- `UKG Late Stab`
- `House Chord Memory`
- `Deep House Sub Chug`
- `Tech House Rubber Bass`
- `Tech House Perc Pluck`
- `Minimal Click Pluck`
- `Minimal Sub Pulse`
- `Techno Pulse Bass`
- `Techno Warehouse Stab`

Regenerate the pack after parameter or template changes:

```sh
python3 tools/generate_factory_presets.py
```

Local macOS builds copy the generated files into:

`~/Library/Application Support/Nate VST/Factory Presets`

Preset files should not contain user-specific absolute paths. Sample-based starter presets should leave `sample_file` empty and configure sampler controls only.

When adding a new parameter, update `DEFAULTS` in `tools/generate_factory_presets.py`, regenerate the pack, and verify every factory file contains the new `PARAM` node.

The UKG factory presets include conservative MOD-panel defaults where useful, such as synced curve-LFO cutoff motion on basses, slower Mod Env filter movement on Dred-style bass, and Mod Env/velocity cutoff movement on stabs and plucks. Presets also seed subtle EQ curves for bass weight, low-mid cleanup, vocal chop presence, and stab brightness. Stab, bell, late-hit, and vocal chop presets use light Tremolo/Auto Pan settings for synced motion. Bell, vocal chop, and late-stab presets use restrained Ring Mod and Comb resonance for metallic edge and tuned texture.

The house, tech-house, minimal, and techno factory presets use the newer SEQ template direction with saved groove, scale, probability, chord/voicing, strum, and step timing defaults. Bass presets keep mono-safe low-end, Width mono-below protection, Pump movement, and Guard enabled where needed; pluck and stab presets lean on Comb, Ring Mod, Delay, Reverb, Width, and EQ for club-ready texture.
