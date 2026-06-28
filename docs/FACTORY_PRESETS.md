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

Factory presets store `mod_slot_*_enabled` values for all eight MOD routes. Defaults keep routes enabled so old and generated patches sound the same, while users can bypass individual assignments from the MOD panel for comparison.

The UKG factory presets include conservative MOD-panel defaults where useful, such as synced curve-LFO cutoff motion on basses, slower Mod Env filter movement on Dred-style bass, Smooth random drift on selected warp/sample-start moves, and Mod Env/velocity cutoff movement on stabs and plucks. Presets also seed Osc Warp, Filter Character, and Filter Slope values, with restrained source warp on bass/stab patches, Warm/24 dB for tighter UKG bass weight, Dirty/24 dB for Dred pressure, and cleaner 12 dB settings for organ, chord, bell, and vocal-chop starters. Presets seed subtle EQ curves for bass weight, low-mid cleanup, vocal chop presence, and stab brightness. Stab, bell, late-hit, and vocal chop presets use light Tremolo/Auto Pan settings for synced motion. Bell, vocal chop, and late-stab presets use restrained Ring Mod and Comb resonance for metallic edge and tuned texture. Techno presets can seed restrained Chaos motion for unstable drive or FX movement.

The house, tech-house, minimal, and techno factory presets use the newer SEQ template direction with saved groove, scale, probability, chord/voicing, strum, and step timing defaults. Bass presets keep mono-safe low-end, Width mono-below protection, Pump movement, source warp where harmonic edge helps, 24 dB slope where tighter filtering helps, and Guard enabled where needed; pluck and stab presets lean on Osc Warp, Filter Character/Slope, Comb, Ring Mod, Delay, Reverb, Width, and EQ for club-ready texture. Pump-enabled presets now store curve type, custom curve points, rate, and depth so UKG patches can prefer Garage ducking, tech-house/techno can stay tighter, minimal pulses can use gated movement, and custom duck shapes recall consistently.

Factory presets also include deterministic browser metadata. `preset_tags` are generated from category, role, sequencer usage, mono-safety, and FX choices. `preset_pack`, `preset_key`, and `preset_bpm` are assigned from the generator metadata map so the Library can search, sort, and filter by pack, musical key, and target tempo. The Library tag filter uses the generated tags for quick browsing by role and production need, such as `Bass`, `Chord`, `Pluck`, `Stab`, `Sequenced`, `Mono Safe`, `Pump`, `Wide`, `FX`, `Vocal Chop`, and genre tags. The Library also parses saved macro values from `macro_1` through `macro_8` so presets can be searched, filtered, sorted, and previewed by performance-macro intensity without changing preset files.

All factory presets seed the eight performance macros. The generator derives Weight from sub/mono settings, Bounce from Pump depth, Warp from oscillator 2 tuning and unison movement, and Throw from the Space macro unless a preset overrides those values directly.
