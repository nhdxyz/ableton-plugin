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

- Synth engine: oscillator 1, oscillator 2, sub, noise, Osc Warp source shaping, unison/spread, envelopes, multimode filter, Filter Character and Slope modes, drive, editable modulation with per-route bypass/delete, LFO 1/LFO 2/Mod Env/Velocity/S&H/Smooth/Chaos movement sources, and eight stable performance macros for club-ready tone, dirt, motion, space, weight, bounce, warp, and throw moves. Saw, square, and triangle sources include a first bandlimited quality pass for cleaner bright stabs and bass harmonics.
- Sound Lab: guided randomization, Vary/Mutate/Wild mutation strengths, section-scoped changes, recipes, section locks, undo/redo, and safety rules.
- Sample Lab: load audio, view the waveform, jump and audition phrase slices with eight pads, drag chop windows with visible handles, reverse/pitch/gain/mix it, store per-slice region/pitch/gain/pan/chance/reverse/stutter/choke behavior, and trigger Slice Keys from MIDI C3-G3.
- Sequencer: internal 16-step piano-roll style pattern area with root-aware note rows, scale helper, chord/stab modes, voicing and strum controls, beat grouping, editable velocity/probability/timing/length lanes, groove cues, transport/PPQ status, rotate, variation, undo, MIDI export, and quick pattern tools for house chords, tech-house basslines, techno pulses, minimal plucks, UKG riffs, and generated ideas.
- FX: reorderable post-synth/post-sample rack with host-sync status, one-click and hold-to-restore delay/space/pump throws, mute drops, tone cleanup, three-band EQ, distortion, pump, tremolo/auto pan, ring modulation, comb resonance, phaser, flanger, chorus, delay, reverb, width, output trim, and Guard safety clipping.
- Library: save, load, audition, search, sort, tag, favorite, rate, and metadata-label categorized `.natevstpreset` files from recursive user and factory preset folders, including House, Tech House, Techno, Minimal, UKG, pack, key, and tempo filters.

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

The `HOME` panel is the default dashboard for quick patch shaping, source selection, core shape controls, club low-end guidance, six fast performance macros, one-gesture Motion/Space XY movement, randomization, A/B snapshots, and preset recall/audition. The `SYNTH` panel exposes waveform, voice, Osc Warp, filter mode, Filter Character, and Filter Slope choices for deeper patch edits without crowding HOME. The `MOD` panel includes LFO 1, Mod Env 1, the full eight-macro performance bank, right-click control assignment, and an editable eight-slot matrix with per-route bypass/delete for safe synth, sample, and FX destinations. Waveform, oscillator 2 waveform, filter mode, and sequencer-rate controls use compact selectors. The `SAMPLE` panel includes a waveform overview, draggable start/end region, phrase markers, and slice pads for fast vocal chops. The `SEQ` panel includes one-tap Bass, Stab, and UKG pattern shortcuts plus house, tech-house, minimal, and techno templates with chord, voicing, strum, variation, undo, rotate, MIDI export, and a host-sync badge for checking Ableton transport lock while building club patterns. The `FX` panel includes one-click Delay Throw, Space Throw, Pump Drop, and Throw Off actions plus hold-to-restore delay, space, pump, and mute-drop buttons above the reorderable rack, with a matching host-sync badge for tempo-based motion. A compact top-bar output meter stays visible across panels, and a bottom piano keyboard stays available for mouse auditioning. The `LIBRARY` panel adds one-click audition, a multi-column preset browser, text search, sort modes, 1-5 star ratings, editable author/pack/key/BPM metadata, macro preview search/sort/filtering, recursive category/subfolder saves, and preset filtering for favorites, recents, rated presets, user/factory source, smart tags, packs, BPM ranges, and saved categories including House, Tech House, Techno, Minimal, and UKG.

User presets are stored at:

```text
~/Library/Application Support/Nate VST/Presets
```

New user preset saves are organized under category folders, for example:

```text
~/Library/Application Support/Nate VST/Presets/UKG/Bass
```

Factory presets can be placed separately at:

```text
~/Library/Application Support/Nate VST/Factory Presets
```

See [PLAN.md](./PLAN.md) for the full roadmap, [docs/DEEP_PROGRAM_RESEARCH_ADD_LIST_2026_06_28.md](./docs/DEEP_PROGRAM_RESEARCH_ADD_LIST_2026_06_28.md) for the newest 300-item program add-list, [docs/HOUSE_PLUGIN_DEEP_FEATURE_AUDIT_2026_06_28.md](./docs/HOUSE_PLUGIN_DEEP_FEATURE_AUDIT_2026_06_28.md) for the long house-plugin audit, [docs/NEXT_FEATURE_RESEARCH.md](./docs/NEXT_FEATURE_RESEARCH.md) for the current prioritized add list, [docs/PROGRAM_RESEARCH_GAP_LIST.md](./docs/PROGRAM_RESEARCH_GAP_LIST.md) for the latest program-by-program research gaps, [docs/DEEP_RESEARCH_BACKLOG.md](./docs/DEEP_RESEARCH_BACKLOG.md) for the implementation backlog, [docs/COMPETITOR_GAP_ANALYSIS.md](./docs/COMPETITOR_GAP_ANALYSIS.md) for the expanded competitor gap list, [docs/MODULATION_WORKFLOW.md](./docs/MODULATION_WORKFLOW.md) for the first modulation/performance design, [docs/UI_EFFECTS_ROADMAP.md](./docs/UI_EFFECTS_ROADMAP.md) for the next UI grouping and FX rack direction, and [docs/UK_GARAGE_WORKFLOW.md](./docs/UK_GARAGE_WORKFLOW.md) for the UK garage sound direction.

Current house/UKG workflow focus: cleaner oscillator sources, Osc Warp source motion, Filter Character/Slope modes for bass and stab color, HOME performance macros and club low-end guidance, MOD destination inspection with per-route bypass/delete, SEQ chord/voicing/strum plus one-finger Memory, host-sync status, and selective groove transforms, waveform-based sample chopping with click guards, slice styles, Slice Keys, and per-slice region/pan/chance/choke memory, section-scoped randomization, and an addable FX rack with throw controls and module presets.

## Local Development

Expected tools:

- Xcode or Xcode Command Line Tools.
- CMake.
- Ninja or Make.
- Ableton Live for host testing.
- pluginval for local VST3 validation.

The initial project scaffold uses CMake FetchContent to download JUCE during configuration.

## Build

Once CMake is installed:

```sh
cmake -S . -B build -G Ninja
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```

The current test suite includes `EditorLayoutAudit`, which opens the real plugin editor and checks all panels plus FX detail views for visible controls with empty or out-of-bounds layouts.

The generated VST3 can be copied to:

```text
~/Library/Audio/Plug-Ins/VST3
```

See [CHANGELOG.md](./CHANGELOG.md) for detailed implementation notes by iteration.
