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

- Synth engine: oscillator 1, oscillator 2, sub, noise colors with percussive decay, internal wavetable mode with Osc 1/Osc 2 WT position controls and modulation range overlays, Osc Warp source shaping, unison/spread, envelopes, multimode filter with a live response display and modulation range overlay, Filter Character and Slope modes, drive, editable modulation with per-route bypass/delete, activity-metered LFO 1/LFO 2/Mod Env/Velocity/S&H/Smooth/Chaos movement sources, and eight stable performance macros for club-ready tone, dirt, motion, space, weight, bounce, warp, and throw moves. Saw, square, triangle, and wavetable sources include a first bandlimited/procedural quality pass for cleaner bright stabs and bass harmonics.
- Sound Lab: page-based guided randomization with focused Generate, Mutate, Recipe, History, and Save areas, recipe intent/tempo/use-case summaries, Vary/Mutate/Wild mutation strengths, per-section Source/Env/Filter/Sample/FX/Seq/Macro generation strength, one-click section rolls, four recallable/generated-cue candidate slots with visible compare/diff detail, changed-section diff badges, save-from-slot with favorite/rating handoff, and A/B snapshot promotion, recipes, section locks, Random Lab undo/redo, useful-only validation with short render checks, auto-retry, and recipe-aware safe fallbacks for audible/source-safe/output-safe generated patches, generated preset draft naming, editable generated preset metadata notes with reusable note templates, category/pack/BPM save suggestions, and safety rules.
- Sample Lab: load audio, view the waveform with chop modulation range feedback, jump and audition phrase slices with eight pads, drag chop windows with visible handles, reverse/pitch/gain/mix it, store per-slice region/pitch/gain/pan/chance/reverse/stutter/choke behavior, and trigger Slice Keys from MIDI C3-G3.
- Sequencer: internal 16-step piano-roll style pattern area with root-aware note rows, scale helper, chord/stab modes including Maj 7, Dom 7, Sus, House 9, and Dub, voicing and strum controls, beat grouping, editable velocity/probability/timing/length/lock lanes, assignable lock destinations for cutoff, drive, Osc Warp, pump, delay, reverb, and wavetable position movement, appended groove modes for house shuffle, tech-house, minimal skip, and techno drive, transport/PPQ status, four pattern scenes for A/B/Fill/Drop variations, rotate, variation, undo, MIDI export, and quick pattern tools for deep-house chords, dub chords, offbeat bass, rolling bass, tech-house basslines, techno pulses, minimal plucks, UKG riffs, and generated ideas.
- FX: reorderable post-synth/post-sample rack with host-sync status, send-style delay/reverb throws, one-click and hold-to-restore delay/space/pump throws, mute drops, tone cleanup, three-band EQ, distortion, pump with live phase/reduction telemetry, tremolo/auto pan, ring modulation, comb resonance, phaser, flanger, chorus, delay, reverb, width, output trim, final-output oscilloscope/spectrum snapshot telemetry, and Guard safety clipping with glue compression, transient punch, clip blend, live output safety, and reduction status.
- Library: save, load, audition, search, sort, tag, favorite, rate, inspect, macro-value preview, preset notes, and metadata-label categorized `.natevstpreset` files from recursive user and factory preset folders, including House, Tech House, Techno, Minimal, UKG, pack, key, and tempo filters.

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
- `INFO`

The `HOME` panel is the default dashboard for quick patch shaping, source selection, core shape controls, club low-end guidance, a central live overview for source balance, macro motion, cutoff/drive/pump/send/output status, final-output oscilloscope shape, spectrum energy, stereo field/correlation, low-end side-risk visibility, six fast performance macros, one-gesture Motion/Space XY movement, compact randomization, A/B snapshots, and preset recall/audition only. The `SYNTH` panel exposes waveform, voice, Osc Warp, filter mode, Filter Character, Filter Slope, wavetable preview with position modulation feedback, and a live filter response display with modulation range feedback for deeper patch edits without crowding HOME. The `LAB` panel is the full randomization workspace, split into focused `Generate`, `Mutate`, `Recipe`, `History`, and `Save` pages for recipe selection with genre/tempo/use-case summaries, scoped generation, per-section generation strength, useful-only validation feedback from parameter and short-render checks, lock feedback, section-roll buttons, four generated-candidate recall slots, non-destructive candidate `Cue` buttons, visible History detail for active candidate sections/traits/diffs/validation, changed-section count badges, `Save Slot`, candidate save favorite/rating controls, `To A`/`To B` snapshot promotion, mutation controls, and editable generated-preset save metadata notes plus reusable macro/use-case templates without showing every control at once. The `MOD` panel includes LFO 1 with drawable MSEG tools, LFO 2, Mod Env 1, activity-metered modulation source rows, the full eight-macro performance bank, right-click control assignment, and an editable eight-slot matrix with per-route bypass/delete for safe synth, sample, and FX destinations, including send delay/reverb amounts. Waveform, oscillator 2 waveform, filter mode, and sequencer-rate controls use compact selectors. The `SAMPLE` panel includes a waveform overview with Sample Start/Mix/Pitch/Ramp/Stutter modulation feedback, draggable start/end region, phrase markers, and slice pads for fast vocal chops. The `SEQ` panel includes one-tap Bass, Stab, and UKG pattern shortcuts plus house, UKG, tech-house, minimal, and techno groove templates with expanded chord colors, voicing, strum, variation, undo, rotate, MIDI export, a per-step Lock lane, A/B/Fill/Drop pattern scene capture/recall, and a host-sync badge for checking Ableton transport lock while building club patterns. The `FX` panel includes one-click Delay Throw, Space Throw, Pump Drop, and Dry Reset actions plus hold-to-restore delay, space, pump, and mute-drop buttons above the reorderable rack; Delay and Reverb detail views include separate Send knobs so throw tails can be distinct from serial insert mix, and Guard exposes Push, Glue, Punch, Clip, and Ceiling for club-safe dynamics. The Pump display now shows HOST/INT timing, live phase, and ducking reduction as the first sidechain-visualization foundation, while HOME scope/spectrum/stereo field, the Guard slot, and top-bar output meter report live waveform, energy, width/correlation, LOW/SAFE/HOT/CLIP level status, and active Guard reduction. A compact top-bar output meter stays visible across panels, a persistent selected-control strip includes first-pass global `Undo Edit`/`Redo Edit`, and a bottom piano keyboard stays available for mouse auditioning. The `LIBRARY` panel is organized as a preset-vault workflow with quick filters for All, Favorites, House, Tech House, Minimal, UKG, Techno, and Factory, a central production-patch crate with performance-macro previews, one-click audition, adaptive preset rows, text search, sort modes, 1-5 star ratings, editable author/pack/key/BPM metadata, editable notes and note templates, selected-preset stats, eight-macro shape preview/search/sort/filtering, recursive category/subfolder saves, and preset filtering for favorites, recents, generated presets, rated presets, user/factory source, smart tags, packs, BPM ranges, and saved categories including House, Tech House, Techno, Minimal, and UKG. The `INFO` panel keeps About, workflow, topic explanations, and quick work-area jumps out of the main sound-design panels. The editor opens at 1040x760, keeps 940x710 as the minimum compact host size, and can resize up to 1440x980 in hosts that expose plugin resize handles.

At default and larger editor sizes, the MOD routing section also shows a compact route map above the matrix so active and bypassed movement is visible as source-to-destination paths. At the minimum compact size, the map hides so the editable matrix rows remain usable in Ableton.

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

The repo-managed factory pack currently includes 32 starter presets across UKG, house, tech-house, minimal, and techno.

See [PLAN.md](./PLAN.md) for the full roadmap, [docs/RANDOMIZATION_LIBRARY_RESEARCH_2026_06_28.md](./docs/RANDOMIZATION_LIBRARY_RESEARCH_2026_06_28.md) for the current randomization/library roadmap, [docs/DEEP_PROGRAM_RESEARCH_ADD_LIST_2026_06_28.md](./docs/DEEP_PROGRAM_RESEARCH_ADD_LIST_2026_06_28.md) for the newest 300-item program add-list, [docs/HOUSE_PLUGIN_DEEP_FEATURE_AUDIT_2026_06_28.md](./docs/HOUSE_PLUGIN_DEEP_FEATURE_AUDIT_2026_06_28.md) for the long house-plugin audit, [docs/NEXT_FEATURE_RESEARCH.md](./docs/NEXT_FEATURE_RESEARCH.md) for the current prioritized add list, [docs/PROGRAM_RESEARCH_GAP_LIST.md](./docs/PROGRAM_RESEARCH_GAP_LIST.md) for the latest program-by-program research gaps, [docs/DEEP_RESEARCH_BACKLOG.md](./docs/DEEP_RESEARCH_BACKLOG.md) for the implementation backlog, [docs/COMPETITOR_GAP_ANALYSIS.md](./docs/COMPETITOR_GAP_ANALYSIS.md) for the expanded competitor gap list, [docs/MODULATION_WORKFLOW.md](./docs/MODULATION_WORKFLOW.md) for the first modulation/performance design, [docs/UI_EFFECTS_ROADMAP.md](./docs/UI_EFFECTS_ROADMAP.md) for the next UI grouping and FX rack direction, and [docs/UK_GARAGE_WORKFLOW.md](./docs/UK_GARAGE_WORKFLOW.md) for the UK garage sound direction.

Current house/UKG workflow focus: cleaner oscillator sources, percussive noise ticks/air/vinyl/digital texture, internal wavetable movement, Osc Warp source motion, visible Filter Character/Slope/cutoff/resonance shaping for bass and stab color, HOME performance macros, output oscilloscope/spectrum/stereo analysis, and club low-end guidance, redesigned preset recall/save/library flow, MOD destination inspection with per-route bypass/delete, SEQ chord/voicing/strum plus one-finger Memory, host-sync status, selective groove transforms, and the first assignable per-step lock lane for rhythmic cutoff/drive/warp/pump/delay/reverb/WT movement, waveform-based sample chopping with click guards, slice styles, Slice Keys, and per-slice region/pan/chance/choke memory, section-scoped randomization, and an addable FX rack with send-style throws, Guard glue/punch/clip dynamics, and module presets.

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

The current test suite includes `EditorLayoutAudit`, which opens the real plugin editor and checks all panels, every Random Lab page, and FX detail views at minimum, default, wide, and maximum editor sizes for visible controls with empty or out-of-bounds layouts, `PresetSaveAudit`, which verifies nested category saves, normalized preset metadata, macro-value preview scanning, and load-by-name behavior, `FactoryPresetLibraryAudit`, which verifies the 32-preset expanded factory style pack scans with metadata/notes and loads by name, `RandomCandidateAudit`, which verifies generated-candidate capture, useful-only validation, auto-retry/fallback reporting, compare/diff summaries, non-destructive cue preview, save-from-slot, favorite/rating persistence, recall, and snapshot promotion, `GlobalEditHistoryAudit`, which verifies first-pass global undo/redo restores synth parameters, modulation routes, sequencer step data, and performance snapshot state, `PumpTelemetryAudit`, which verifies Pump reports live ducking telemetry and clears it when disabled, `GuardTelemetryAudit`, which verifies hot patches report Guard protection telemetry and disabled Guard clears the meter state, `GuardDynamicsAudit`, which verifies Guard punch, glue compression, and clip-ceiling behavior, `SequencerPatternSceneAudit`, which verifies pattern scene capture/recall and save/restore, `SequencerHousePatternAudit`, which verifies house chord colors, chord/bassline pattern presets, appended groove-mode compatibility, and the house/UKG/tech-house/minimal/techno groove templates, `NoiseSourceAudit`, which verifies noise-source choices, Tick decay, and sustained white/digital noise rendering, `EffectsSendAudit`, which verifies send delay tails and one-shot tail kill behavior, `SpectrumSnapshotAudit`, which verifies final-output spectrum snapshots start silent, capture rendered output, stay finite, and reset on prepare, and `StereoFieldTelemetryAudit`, which verifies stereo field telemetry resets cleanly and remains finite/in range after rendered output.

The generated VST3 can be copied to:

```text
~/Library/Audio/Plug-Ins/VST3
```

See [CHANGELOG.md](./CHANGELOG.md) for detailed implementation notes by iteration.
