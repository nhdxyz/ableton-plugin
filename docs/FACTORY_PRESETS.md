# Nate VST Factory Presets

Factory presets are repo-managed `.natevstpreset` XML files in:

`Resources/Factory Presets`

The first pack focuses on UK garage workflows:

- `UKG 2-Step Bass`
- `UKG Shuffle Bass`
- `UKG Organ Stab`
- `UKG Chord Stab`
- `UKG Bell Pluck`
- `UKG Vocal Chop Starter`
- `UKG Late Stab`

Regenerate the pack after parameter or template changes:

```sh
python3 tools/generate_factory_presets.py
```

Local macOS builds copy the generated files into:

`~/Library/Application Support/Nate VST/Factory Presets`

Preset files should not contain user-specific absolute paths. Sample-based starter presets should leave `sample_file` empty and configure sampler controls only.

When adding a new parameter, update `DEFAULTS` in `tools/generate_factory_presets.py`, regenerate the pack, and verify every factory file contains the new `PARAM` node.
