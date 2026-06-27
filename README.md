# Nate VST

Mac-first VST3 instrument plugin for Ableton Live, focused on house, tech house, techno, minimal, and related electronic production.

The long-term direction is a hybrid synth with subtractive synthesis, wavetable-style movement, sampling/slicing, random sound generation, internal pattern tools, and an effects area.

## Current Target

- macOS first.
- Ableton Live as the main host.
- VST3 first.
- JUCE + CMake.
- C++20.

## Product Direction

The first useful version should load in Ableton, respond to MIDI, generate sound, expose automatable parameters, and save state in a Live Set.

Planned major areas:

- Synth engine: oscillator 1, oscillator 2, sub, noise, unison/spread, envelopes, multimode filter, drive, modulation, and eight stable performance macros for club-ready tone, dirt, motion, space, weight, bounce, warp, and throw moves.
- Sound Lab: guided randomization, Vary/Mutate/Wild mutation strengths, section-scoped changes, recipes, section locks, undo/redo, and safety rules.
- Sample Lab: load audio, view the waveform, jump and audition phrase slices with eight pads, drag chop windows with visible handles, reverse/pitch/gain/mix it, and trigger it from MIDI.
- Sequencer: internal 16-step piano-roll style pattern area with root-aware note rows, scale helper, chord/stab modes, voicing and strum controls, beat grouping, groove cues, rotate, variation, undo, MIDI export, and quick pattern tools for house chords, tech-house basslines, techno pulses, minimal plucks, UKG riffs, and generated ideas.
- FX: reorderable post-synth/post-sample rack with one-click and hold-to-restore delay/space/pump throws, mute drops, tone cleanup, three-band EQ, distortion, pump, tremolo/auto pan, ring modulation, comb resonance, phaser, flanger, chorus, delay, reverb, width, output trim, and Guard safety clipping.
- Library: save, load, audition, search, tag, and favorite categorized `.natevstpreset` files from the user and factory preset folders, including House, Tech House, Techno, Minimal, UKG, and utility filters.

## Interface

The editor uses focused panels instead of one tall stacked page:

- `HOME`
- `SYNTH`
- `LAB`
- `MOD`
- `SAMPLE`
- `SEQ`
- `FX`
- `LIBRARY`

The `HOME` panel is the default dashboard for quick patch shaping, source selection, core shape controls, club low-end guidance, four fast performance macros, one-gesture Motion/Space XY movement, randomization, A/B snapshots, and preset recall/audition. The `MOD` panel includes LFO 1, Mod Env 1, the full eight-macro performance bank, and an editable eight-slot matrix for safe synth destinations. Waveform, oscillator 2 waveform, filter mode, and sequencer-rate controls use segmented selectors. The `SAMPLE` panel includes a waveform overview, draggable start/end region, phrase markers, and slice pads for fast vocal chops. The `SEQ` panel includes one-tap Bass, Stab, and UKG pattern shortcuts plus house, tech-house, minimal, and techno templates with chord, voicing, strum, variation, undo, rotate, and MIDI export controls for fast club sketching. The `FX` panel includes one-click Delay Throw, Space Throw, Pump Drop, and Throw Off actions plus hold-to-restore delay, space, pump, and mute-drop buttons above the reorderable rack. A compact top-bar output meter stays visible across panels, and a bottom piano keyboard stays available for mouse auditioning. The `LIBRARY` panel adds one-click audition, text search, and preset filtering for favorites, recents, user/factory source, smart tags, and saved categories including House, Tech House, Techno, Minimal, and UKG.

User presets are stored at:

```text
~/Library/Application Support/Nate VST/Presets
```

Factory presets can be placed separately at:

```text
~/Library/Application Support/Nate VST/Factory Presets
```

See [PLAN.md](./PLAN.md) for the full roadmap, [docs/MODULATION_WORKFLOW.md](./docs/MODULATION_WORKFLOW.md) for the first modulation/performance design, [docs/UI_EFFECTS_ROADMAP.md](./docs/UI_EFFECTS_ROADMAP.md) for the next UI grouping and FX rack direction, and [docs/UK_GARAGE_WORKFLOW.md](./docs/UK_GARAGE_WORKFLOW.md) for the UK garage sound direction.

Current house/UKG workflow focus: HOME performance macros and club low-end guidance, SEQ chord/voicing/strum plus one-finger Memory and selective groove transforms, waveform-based sample chopping with click guards, section-scoped randomization, and an addable FX rack with throw controls.

## Local Development

Expected tools:

- Xcode or Xcode Command Line Tools.
- CMake.
- Ninja or Make.
- Ableton Live for host testing.
- pluginval for validation later.

The initial project scaffold uses CMake FetchContent to download JUCE during configuration.

## Build

Once CMake is installed:

```sh
cmake -S . -B build -G Ninja
cmake --build build --config Debug
```

The generated VST3 can be copied to:

```text
~/Library/Audio/Plug-Ins/VST3
```

See [CHANGELOG.md](./CHANGELOG.md) for detailed implementation notes by iteration.
