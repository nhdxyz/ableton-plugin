# Nate VST Factory Presets

Factory presets are repo-managed `.natevstpreset` XML files in:

`Resources/Factory Presets`

The factory pack currently includes 502 UK garage and broader club workflow starters. Representative named starters include:

- `UKG 2-Step Bass`
- `UKG Shuffle Bass`
- `UKG Dred Bass`
- `UKG Organ Stab`
- `UKG Chord Stab`
- `UKG Bell Pluck`
- `UKG Vocal Chop Starter`
- `UKG Late Stab`
- `House Chord Memory`
- `Synth Stab Clean Minor Chord`
- `Deep House Seventh Stab`
- `Deep House Mellow Keys`
- `Deep House Sub Chug`
- `Deep House Velvet Pad`
- `House Organ Skank`
- `Afro House Perc Bell`
- `Afro House Marimba Skank`
- `Afro Melodic House Pluck`
- `Progressive House Rise Pad`
- `Progressive House Pluck Stack`
- `Lo-Fi House Dust Keys`
- `Lo-Fi House Vinyl Chord`
- `Deep House Ninth Chord Kit`
- `Piano House Lift Riff`
- `Detroit House Chord Stab`
- `Chicago House Acid Bass`
- `Classic House Piano Chord`
- `Classic House Warehouse Piano Stab`
- `Classic House M1 Organ Bass`
- `Funky House Clav Stab`
- `Melodic House Arp Pluck`
- `Jackin Organ Bass Kit`
- `Disco Filter Sweep Stab`
- `Dub House Chord Throw`
- `Dirty House Tool Stab`
- `Garage House Chord Kit`
- `Romanian Minimal Pluck`
- `Piano House Riff Keys`
- `Disco House Filter Stab`
- `Italo Disco Arp Lead`
- `Nu Disco Filter Lead`
- `Balearic Sunset Pad`
- `Latin House Perc Organ`
- `Organic House Kalimba Pluck`
- `Electro House Sync Lead`
- `Legacy Electro Octave Sequencer Line`
- `Legacy Electro Cold Sync Hook`
- `Acid House 303 Line`
- `Indie Dance Rubber Bass`
- `Hard House Rave Stab`
- `Speed Garage Organ Bass`
- `Bass House Wobble Hook`
- `Hardgroove Techno Perc Stab`
- `Classic Trance Supersaw Anthem Lead`
- `Classic Trance Offbeat Rolling Bass`
- `Classic Techno Hoover Alarm Lead`
- `Classic Techno Detroit Minor Stab`
- `Amapiano Log Drum Pluck`
- `Future Garage Atmos Pad`
- `Jackin House Organ Bass`
- `Minimal Micro Perc Sequence`
- `Minimal FM Bubble`
- `Deep Minimal Sub Groove`
- `Peak Time Techno FM Lead`
- `Peak Time Techno Rumble Bass`
- `Melodic Techno Reese Lead`
- `Deep Tech Organ Roller`
- `Deep Tech Chord Tool`
- `French House Filter Chord`
- `Soulful House M1 Keys`
- `Garage House Swing Lead`
- `Microhouse Dust Pluck`
- `Raw Hypnotic Techno Stab`
- `Afro Tech Perc Bass`
- `Tribal Tech House Perc Bass`
- `Breaks House Reese Pluck`
- `Tech House Acid Roller`
- `Tech House Rubber Bass`
- `Tech House Rubber Hook Kit`
- `Tech House Top Pluck`
- `Tech House Perc Pluck`
- `Tech House Rolling Bass`
- `Tech House Perc Stab`
- `Minimal Hypno Sequence`
- `Minimal Click Pluck`
- `Minimal Sub Pulse`
- `Minimal Glass Pluck`
- `Minimal Dub Chord`
- `Minimal Noise Tick`
- `Techno Metallic Ping`
- `Techno Pulse Bass`
- `Techno Warehouse Stab`
- `Techno Rumble Stab`
- `Detroit Techno Minor Chord`
- `Berlin Dub Techno Stab`
- `Dub Techno Chord Wash`
- `Melodic Techno Pluck Lead`
- `Melodic Techno Dark Pulse`
- `Warehouse Techno Rumble Hit`
- `Hardgroove Rave Chord`
- `Electro Breaks Sync Bass`
- `Warehouse Noise Sweep`
- `Warehouse Riser Noise`
- `Breakbeat Garage Reese`
- `Acid Minimal Squiggle`

Regenerate the pack after parameter or template changes:

```sh
python3 tools/generate_factory_presets.py
```

Local macOS builds copy the generated files into:

`~/Library/Application Support/Nate VST/Factory Presets`

Preset files should not contain user-specific absolute paths. Sample-based starter presets should leave `sample_file` empty and configure sampler controls only.

When adding a new parameter, update `DEFAULTS` in `tools/generate_factory_presets.py`, regenerate the pack, and verify every factory file contains the new `PARAM` node.

Factory presets store `mod_slot_*_enabled` values for all eight MOD routes. Defaults keep routes enabled so old and generated patches sound the same, while users can bypass individual assignments from the MOD panel for comparison.

Factory presets store the current sampler engine parameters: `sample_engine_mode`, `sample_grain_size`, `sample_grain_spray`, and `sample_spectral_freeze`. Default presets keep Classic mode unless a generated/sample-oriented patch intentionally needs granular, cloud, or freeze behavior.

Waveform choices are append-only for preset and Ableton automation compatibility. Indices 0-4 remain Sine, Saw, Square, Triangle, and Wavetable; appended indices 5 and 6 are the procedural Organ and House Piano sources used by targeted house chord, piano, and organ-bass presets.

The UKG factory presets include conservative MOD-panel defaults where useful, such as synced curve-LFO cutoff motion on basses, slower Mod Env filter movement on Dred-style bass, Smooth random drift on selected warp/sample-start moves, and Mod Env/velocity cutoff movement on stabs and plucks. Presets also seed Osc Warp, Filter Character, and Filter Slope values, with restrained source warp on bass/stab patches, Warm/24 dB for tighter UKG bass weight, Dirty/24 dB for Dred pressure, and cleaner 12 dB settings for organ, chord, bell, and vocal-chop starters. Presets seed subtle EQ curves for bass weight, low-mid cleanup, vocal chop presence, and stab brightness. Stab, bell, late-hit, and vocal chop presets use light Tremolo/Auto Pan settings for synced motion. Bell, vocal chop, and late-stab presets use restrained Ring Mod and Comb resonance for metallic edge and tuned texture. Techno presets can seed restrained Chaos motion for unstable drive or FX movement.

The house, tech-house, minimal, trance, and techno factory presets use the newer SEQ template direction with saved groove, scale, probability, chord/voicing, strum, step timing, and per-step ratchet defaults. Bass presets keep mono-safe low-end, Width mono-below protection, Pump movement, source warp where harmonic edge helps, 24 dB slope where tighter filtering helps, and Guard enabled where needed; pluck and stab presets lean on Osc Warp, Filter Character/Slope, Comb, Ring Mod, Delay, Reverb, Width, and EQ for club-ready texture. House Chord Memory, House Organ Skank, Soulful House M1 Keys, Classic House Piano Chord, Deep House Ninth Chord Kit, Piano House Lift Riff, and Jackin Organ Bass Kit now seed the appended procedural Organ and House Piano source modes so house chord and organ patches start from stronger original source material instead of only generic oscillator blends. Selected UKG, tech-house, minimal, deep-house, and garage-house presets seed `seq_step_*_ratchet` values for quick repeat fills while preserving a 1x default for all other steps. Pump-enabled presets now store curve type, custom curve points, rate, and depth so UKG patches can prefer Garage ducking, tech-house/techno can stay tighter, minimal pulses can use gated movement, and custom duck shapes recall consistently. The latest expansion brings the repo-managed pack to 502 presets across the original UKG/house/techno starters plus larger packs for Afro House, Amapiano, ambient/evolving texture tools, bass house, hardgroove, deep/minimal variants, garage, 80s electro/synth-pop, 90s classic house, classic trance, classic techno, Club Synth Stabs, and construction-kit style patches with key, BPM, sequence, macro, and pack intent.

Factory presets also include deterministic browser metadata. `preset_tags` are generated from category, role, sequencer usage, mono-safety, FX choices, and optional era/style pack tags. `preset_pack`, `preset_key`, `preset_bpm`, and optional `preset_notes` are assigned from generator data so the Library can search, sort, filter, and inspect curated usage context. The Library tag filter uses the generated tags for quick browsing by role and production need, such as `Bass`, `Chord`, `Pluck`, `Stab`, `Sequenced`, `Mono Safe`, `Pump`, `Wide`, `FX`, `Vocal Chop`, `Construction Kit`, `Organ`, `Bass House`, `Amapiano`, `Hardgroove`, `Future Garage`, `Speed Garage`, `Deep House`, `Piano House`, `Jackin House`, `Disco House`, `Dub House`, `Deep Tech`, `Indie Dance`, `Italo Disco`, `Balearic House`, `Acid House`, `Nu Disco`, `Afro Tech`, `Afro Melodic`, `Progressive House`, `Hard House`, `Peak Time Techno`, `Detroit Techno`, `Melodic Techno`, `Minimal FM`, `Deep Minimal`, `Lo-Fi House`, `French House`, `Soulful House`, `Garage House`, `Microhouse`, `Raw Techno`, `Tribal Tech House`, `Breaks House`, `80s Electro`, `Synth Pop`, `Classic House`, `Classic Trance`, `Classic Techno`, and broader genre tags. The Library also parses saved macro values from `macro_1` through `macro_8` so presets can be searched, filtered, sorted, and previewed by performance-macro intensity without changing preset files.

`FactoryPresetRenderAudit` now renders every repo-managed factory preset through the real preset loader and synth/sample/FX path. It verifies finite, non-silent, peak-contained output and uses transient-aware thresholds for short ticks, hits, sweeps, and plucks. Microhouse Dust Pluck and Minimal FM Bubble were lifted to `-6 dB` output after this audit exposed them as too quiet for reliable factory audition.

All factory presets seed the eight performance macros. The generator derives Weight from sub/mono settings, Bounce from Pump depth, Warp from oscillator 2 tuning and unison movement, and Throw from the Space macro unless a preset overrides those values directly.
