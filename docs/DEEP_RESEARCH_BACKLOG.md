# Nate VST Deep Research Backlog

Research date: 2026-06-28

Current house-focused refresh: 2026-06-29, see `docs/HOUSE_VST_COMPETITOR_RESEARCH_2026_06_29.md`.

This document turns the current reference review into an implementation backlog for Nate VST. It is intentionally feature-focused and avoids copying proprietary UI, presets, wavetables, samples, DSP code, branding, or product-specific workflows.

## Reference Set

Primary synth and instrument references:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Korg Collection: https://www.korg.com/us/products/software/korg_collection/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/

Primary FX, groove, and club-workflow references:

- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Output Portal: https://output.com/products/portal
- Ableton Simpler manual: https://www.ableton.com/en/live-manual/12/live-instrument-reference/#simpler
- Ableton Live MIDI tools: https://www.ableton.com/en/live-manual/12/midi-tools/
- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Nate VST Strengths

- Focused house, UKG, tech-house, techno, and minimal direction.
- Stable JUCE/VST3 foundation with broad local CTest coverage; a repo-backed pluginval gate and repeatable Ableton validation checklist remain open.
- PolyBLEP oscillator pass for better bright-note behavior.
- Filter character and slope options.
- Oscillator warp control.
- FX rack with add/select workflow, module summaries, sync delay, pump, width, guard, and custom pump curves.
- MOD panel with sources, destinations, route bypass/delete, curve presets, macro assignment editing, and destination badges.
- Sequencer with piano-roll editing, chord memory, editable velocity/probability/timing/length/lock/ratchet/condition/slide lanes, groove timing, templates, first-pass Chord Stab Paint, first-pass `Build 4-Bar Chain` scene generation, first-pass live scene-chain step/control playback, expanded-focus `Auto` / `2 Bar` / `4 Bar` chain length mode, single-pattern and scene-chain MIDI export, first-pass MIDI drag-out from the `MIDI` and `Chain` buttons, scenes, and host transport sync.
- Sampler/chop workflow for UKG vocal and stab material, including Slice Keys, first-pass transient Detect slicing, direct boundary dragging for existing slice regions, visual selected/custom slice overlays, expanded chop focus editing, and first per-slice region/pitch/gain/pan/probability/reverse/stutter/choke memory.
- Live/clickable SYNTH house source rack with expanded source-layer focus overlay for Sub, Body, Character/Stab, Transient/Noise, and Chop roles.
- Library with folders, categories, favorites, ratings, metadata, macro previews, search, sort, recursive preset scanning, visual save preview, left crate rail folder/pack/style coverage, centered browser actions, HOME Patch Snapshot recall/performance state, rendered preview cache/playback, visible-row preview warming, preview level badges, and compact-width browser rows that keep preset name/metadata readable.
- Factory preset generator for genre-targeted packs.

## Product Gaps

The largest remaining gap is no longer "does the plugin have enough controls." It is now "does each control feel fast, visual, musical, and reliable inside Ableton." The next work should favor interaction quality, source depth, browser usefulness, and club-specific timing tools over adding more unfocused knobs.

See `docs/HOUSE_VST_COMPETITOR_RESEARCH_2026_06_29.md` for the current house-focused competitor refresh. `docs/PROGRAM_RESEARCH_GAP_LIST.md` and `docs/COMPETITOR_GAP_ANALYSIS.md` remain older broad research matrices. This file remains the shorter implementation backlog.

## Priority 0: UI And Workflow Corrections

1. Extract HOME, SYNTH, SAMPLE, MOD, SEQ, FX, LAB, LIBRARY, and INFO panels into smaller editor components.
2. Add automated layout overflow checks for every page and supported editor size. First CTest-backed editor bounds audit implemented for minimum, default, wide, and maximum editor sizes.
3. Add screenshot-based UI regression checks for desktop scale factors.
4. Add a debug overlay that marks component bounds when checking overlap.
5. Standardize section headers, spacing, and row heights across pages.
6. Convert repeated dense knob rows into task groups: Source, Shape, Motion, Space, Utility.
7. Enlarge rotary hit areas without making visual knobs oversized.
8. Add per-control fine-drag behavior hints through tooltips.
9. Add shift-drag or command-drag fine adjustment if JUCE host behavior allows it cleanly.
10. Add double-click reset handling for every slider.
11. Add value popovers while dragging knobs.
12. Make modulation badges opt-in per page if visual density gets too high.
13. Add a focused inspector strip that follows the last touched control.
14. Add page-level mini meters for output, voice count, CPU estimate, and safety limiting.
15. Add a compact "init from genre" strip on HOME instead of more permanent controls.

Implemented control-feel sub-slice:

- Rotary, horizontal, compact curve, and macro-assignment amount sliders now expose Shift/Cmd fine adjustment, value popups, typed values, and double-click reset through a shared slider setup path. Rotary drag distance was reduced so primary sound-shaping controls move with less effort while scroll-wheel editing stays disabled to prevent accidental trackpad changes inside Ableton.

Implemented UI test sub-slices:

- `EditorLayoutAudit` now instantiates the real plugin editor and checks HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, INFO, and every FX detail module for visible empty-bounds or editor-overflow failures through CTest.
- The editor now opens at 1040x760, preserves 940x710 as the compact minimum, exposes host resize limits up to 1440x980, and `EditorLayoutAudit` sweeps minimum, minimum-tall, default, default-tall, wide-min-height, wide, and maximum sizes across every panel, every Random Lab page, and every FX detail module. The SYNTH house layer rack has explicit visible-bounds/overlap checks, the bottom audition keyboard now scales white-key width from the visible note range to the available bottom-bar area, the SEQ piano-roll grid has adaptive lane height plus an expanded pattern focus overlay, and the audit checks key-width/painted-coverage, keyboard-control compression, and compact/expanded SEQ row/cell readability so wide hosts do not leave a blank keyboard tail and compact hosts keep the sequencer usable.
- Laptop-key auditioning now installs the editor key listener across the component tree with text-editor/combo-box exclusion, and the 12 Hz UI timer skips hidden visual refreshes for off-page analyzers, wavetable/sample views, MOD detail surfaces, and sequencer displays.
- First-pass global `Undo Edit`/`Redo Edit` is available from the persistent selected-control strip, with `GlobalEditHistoryAudit` verifying full-state restore across synth parameters, modulation routes, sequencer step data, and performance snapshots.
- A dedicated `INFO` panel now keeps About/workflow/topic explanations and quick LAB/MOD/FX/LIBRARY jumps out of the primary sound-design panels, reducing always-on text clutter.
- HOME now has a central `HomeOverviewDisplay` for source balance, macro state, cutoff/drive, pump/send/output meters, and Guard reduction, plus an editable eight-point macro shape map for Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw. A larger in-editor macro focus overlay can be opened from HOME or MOD for precision macro-shape and assignment editing. This reduces the need to show every sound-design control on the default dashboard.
- HOME now has a compact signal-flow strip for Source, Filter, Motion, FX, Guard, and Output active-state visualization; deeper split, send, and parallel routing views remain future work.
- LIBRARY now has dedicated Crates, Browser, Save Target, and Preset Profile work areas plus an adaptive `PresetLibrarySummary` profile card, a compact crate rail with folder/pack/style coverage, centered browser actions, and a redesigned Save Plan card with destination trail and labeled readiness, while HOME has a compact Patch Snapshot for role/source/safety, performance meters, compare, sequencer/pump, and random-candidate state above recall/audition.
- SYNTH now has a dedicated `FilterResponseDisplay` above the filter controls, making cutoff, resonance, mode, character, slope, envelope amount, and drive visible without adding another row of knobs.
- SYNTH now has a themed `HouseLayerRackDisplay` in the source card, showing existing Osc 1, Osc 2, Sub, Noise, and Sample Mix state as Sub, Body, Character/Stab, Transient/Noise, and Chop layers.
- HOME now includes `OutputOscilloscopeDisplay` and `OutputSpectrumDisplay` fed by the real-time-safe final-output snapshot ring, making waveform shape, transient punch, clipping risk, held spectrum peaks, and sub/low/mid/presence/air energy visible without opening a separate analysis page.
- The filter response display now shows MOD-matrix route depth for Filter Cutoff, Filter Res, Filter Env, and Drive, including cutoff range shading and source summaries.
- Osc 1/Osc 2 WT position controls and MOD destinations are implemented; the previous compact source-graph slot is now used by the broader house layer rack until deeper wavetable visual editing lands.
- The sample waveform display now shows MOD-matrix route depth for Sample Start, Mix, Pitch, Ramp, and Stutter, including start-range shading, source summaries for chop motion, selected/custom slice badges, and an expanded chop focus overlay.

## Priority 1: Modulation And Movement

16. Add drag-style modulation assignment from source chips to destination controls.
17. Add right-click or menu assignment from any automatable control. First pass implemented for MOD-targetable sliders.
18. Add a macro assignment editor showing destination, amount, polarity, and range. First passes implemented hidden source/destination/amount backing controls plus a draggable visual macro assignment pad with route counts, assigned destination chips, add/update, clear, summary tooltips, and a larger audited focus overlay; explicit min/max range editing remains open.
19. Add per-route curve shaping.
20. Add per-route unipolar/bipolar mode.
21. Add per-route smoothing.
22. Add per-route invert.
23. Add per-route mute, solo, duplicate, and copy/paste.
24. Add animated modulation source activity indicators.
25. Add LFO phase and depth readouts near modulated controls.
26. Add MSEG-style multi-point envelope source beyond LFO curve mode.
27. Add additional LFOs with lightweight source selection.
28. Add random LFO/sample-and-hold source.
29. Add step LFO lane with probability and hold/slew.
30. Add envelope follower source for a future audio-effect version.
31. Add mod wheel and aftertouch source mapping.
32. Add velocity curve editor.
33. Add keytrack source with slope and center note.
34. Add global "show routes for selected parameter" mode.
35. Add macro snapshots for eight performance macro states.

Implemented movement sub-slices:

- `S&H` is now an appended MOD source for stepped random movement, routed through synth, sample, and FX modulation paths without changing existing source indices. Macro assignment editing remains limited to the eight performance macros.
- `Smooth` is now an appended MOD source after `S&H` for slewed random drift across synth, sample, and FX destinations. It keeps old source indices intact and follows the existing LFO timing path while interpolating between held random targets.
- `Chaos` is now an appended MOD source after `Smooth` for bounded random-walk movement across synth, sample, and FX destinations.
- `LFO 2` is now an appended MOD source after `Chaos`, with independent compact controls and synth/sample/FX routing while preserving old source indices. LFO 3, true MSEG as a separate source, route processors, drag assignment, and step-LFO work remain open.
- MOD-targetable sliders now expose right-click source assignment menus that reuse the existing matrix and inspector route allocation path. Drag-from-source assignment, route ranges, route curves, and route processors remain open.
- Rotary and horizontal MOD-targetable sliders now show source-colored modulation rings, stripes, and badges while preserving their original control tooltips and appending active route source/depth status. MOD-targetable controls also update the selected-control route summary on hover. Graphical hover route overlays and deeper route range editing remain open.
- MOD matrix slots now include a duplicate action that copies the route into the next free slot, plus right-click amount menus for invert, quick +/-25% or +/-50% depths, duplicate, and clear. Deeper copy/paste, solo, range, and curve editing remain open.

## Priority 2: Oscillator And Source Engines

36. Add a true wavetable oscillator.
37. Add wavetable position modulation.
38. Add wavetable warp modes such as bend, fold, mirror, sync-style, and phase skew.
39. Add safe user wavetable import later, after factory wavetable playback is stable.
40. Add wavetable preview display.
41. Add oscillator sync.
42. Add ring modulation.
43. Add frequency modulation or phase modulation between oscillators.
44. Add a simple three-operator FM mode for UKG bells, metallic stabs, and techno pings.
45. Add source-noise colors for air, vinyl, attack tick, rumble, and digital grit.

Implemented wavetable sub-slice:

- Added an internal generated wavetable mode as waveform index 4 for Osc 1 and Osc 2, preserving existing waveform indices.
- Added Osc 1/Osc 2 wavetable position parameters and MOD destinations for WT position movement.
- Added Osc 1/Osc 2 WT position controls and seeded selected factory presets with the new source mode.
- Made the 3D wavetable display's top morph-frame rail a direct WT-position scan control, matching the visual editor expectation from modern wavetable instruments and preventing rail clicks from accidentally editing custom points.
- Added a compact SYNTH-panel filter response display for immediate visual feedback while shaping house basses, UKG organ stabs, minimal plucks, and techno filter sweeps.
- Added HOME output oscilloscope/spectrum analyzers for actual rendered waveform shape, transient punch, clipping risk, held spectrum peaks, and energy across sub, low, mid, presence, and air ranges.
- Added the first modulation overlay on the filter response display so movement assigned in MOD is visible on the tone-shaping graph.
- Added a first visual house layer rack so Osc 1/Osc 2, sub, noise, and sample mix roles can be read directly in the SYNTH source card.
- The house layer rack now stays live from the editor timer, shows actual noise color, sample playback mode/slice style, sample gain-adjusted chop level, and Weight-macro sub support, and opens into an expanded source-layer focus overlay with click-to-focus layer controls.
- Added sample chop modulation and slice-state overlays so Sample Start movement, related chop route depths, and eight-pad custom/selected slice behavior can be read directly on the waveform.

Implemented percussive noise sub-slice:

- Added White, Pink, Brown, Air, Tick, Vinyl, and Digital noise colors with a Noise Decay parameter for transient ticks and longer texture layers.
- Kept bass randomization sub-safe while giving UKG/house stabs, bells, minimal blips, and Noise FX role-aware attack/noise choices.
- Added `NoiseSourceAudit` for choice stability, decaying Tick behavior, and sustained white/digital rendering.

Implemented house source-character sub-slice:

- Added appended procedural Organ and House Piano waveform modes after the existing Sine, Saw, Square, Triangle, and Wavetable indices so saved presets and Ableton automation mappings remain stable.
- Exposed the new source modes in the SYNTH selectors and HOME source summaries, and biased organ/chord random recipes toward them without broadening bass/noise recipes.
- Seeded selected house chord, piano, and organ-bass factory presets with the new original source modes and added `SourceCharacterAudit` for waveform choice ordering plus finite/audible source rendering.
- Deeper drawbar controls, source snippets, sampled PCM-style house content, real source-rack lanes with per-layer controls, and source import remain open.

46. Add supersaw/unison engine improvements with phase, detune curve, and stereo spread.
47. Add analog drift with per-voice pitch, phase, and filter variation.
48. Add a second full synth layer.
49. Add per-layer mute/solo and blend controls.
50. Add partial/layer performance macros inspired by workstation-style browsing, without copying ZEN-Core.
51. Add source-level randomization locks.

## Priority 3: Sampler, Chops, And UKG Tools

51. Add transient-based slice detection. First explicit `Detect` action implemented for loaded samples, writing eight ordered custom Slice Keys regions through the existing slice state.
52. Add manual slice markers in the SAMPLE waveform view. First direct boundary-drag edit pass is implemented for the existing eight Slice Keys regions; marker add/remove and variable-count markers remain open.
53. Add per-slice start/end/nudge. First stored start/end region memory, direct boundary dragging, and stepped nudge controls are implemented for eight pads; variable marker nudge remains open.
54. Add per-slice pitch and gain. First memory/playback pass implemented for eight pads.
55. Add per-slice reverse. First memory/playback pass implemented for eight pads.
56. Add slice randomization with musical density limits. First `Dice` pass implemented for the selected slice.
57. Add choke groups for vocal chops. First per-slice choke/open pass implemented for Slice Keys; named multi-group choke remains open.
58. Add one-shot, gate, and loop modes per slice.
59. Add start-crossfade and end-crossfade to reduce clicks.
60. Add formant-preserving pitch mode if a suitable DSP approach is chosen.
61. Add time-stretch mode for short vocal material.
62. Add granular sampler mode for texture, pads, and fill effects.
63. Add "garage chop" presets for common slice timing layouts.
64. Add drag-and-drop WAV/AIFF sample import in the plugin UI.
65. Add a sample pack browser inside LIBRARY.

Implemented sampler sub-slice:

- The SAMPLE recorder now exposes the actual 8-second rolling capture-buffer fill as a compact progress meter plus a `REC -> READY -> USE -> PLAY` state rail, with recording, ready-to-commit, loaded-sample, and full/rolling states visible in the main SAMPLE panel.
- Slice Keys mode maps the eight SAMPLE pads across C3-G3, reads per-slice region/pitch/gain/pan/probability/reverse/stutter/choke memory at trigger time, and falls back to equal regions plus the selected slice style for untouched pads.
- The SAMPLE UI now exposes Store, Recall, Dice, Rev, Choke, Pan, and Ghost actions with status/tooltips that show default or custom slice regions and behavior.
- The waveform slice lane now supports direct boundary dragging for existing Slice Keys regions in both compact and expanded chop editing views.
- Factory presets now serialize the sample-slice state explicitly, and the UKG vocal chop starter opens in Slice Keys mode once a user loads a sample.

## Priority 4: Sequencer, Piano Roll, And Groove

66. Add multiple sequencer patterns per preset.
67. Add deeper pattern chaining beyond the first live scene-chain playback, Build 4-Bar Chain, and forced chain-length passes.
68. Add per-step probability. First editable lane pass implemented in the SEQ grid.
69. Add per-step ratchet/retrigger. First saved lane/playback/export pass implemented; repeat curves, per-ratchet probability, and deeper velocity falloff remain open.
70. Add per-step note length. First editable lane/playback/export pass implemented in the SEQ grid.
71. Add per-step velocity lane. First editable lane pass implemented in the SEQ grid.
72. Add per-step modulation lane assignments. First assignable Lock lane implemented; expand to multiple lanes, per-step destinations, and sample-accurate lock scheduling.
73. Add per-step sample slice lane.
74. Add per-step pump depth lane.
75. Add per-step delay throw lane.
76. Add scale quantize and key lock for the piano roll.
77. Add deeper chord paint tools for UKG stabs beyond the first Chord Stab Paint transform.
78. Add groove templates for 2-step, shuffle, swung 16ths, minimal offbeats, and techno pulse. First dedicated template pass implemented for House Shuffle, UKG 2-Step Push, Tech House Tight, Minimal Skip, and Techno Drive.
79. Add humanize with separate timing, velocity, and gate controls.
80. Add MIDI drag export from the internal sequencer into Ableton. First-pass current-pattern and scene-chain `.mid` file drag-out is implemented; host validation and richer clip/audio handoff remain open.

Implemented sequencer sub-slice:

- The SEQ panel now has four pattern scenes, `A`, `B`, `Fill`, and `Drop`, with capture/recall buttons that store the full 16-step pattern, per-step lanes, chord/voicing/strum settings, groove settings, rate/root/gate/accent/probability, and lock destination/depth; captured scenes can now be played as a first-pass live step/control chain, exported as one chained MIDI file in A/B/Fill/Drop order, and forced through Auto, 2-bar, or 4-bar chain length for live playback and Chain MIDI save/drag from the expanded SEQ focus.
- Pattern scenes serialize into plugin state and preset state without becoming host automation parameters, and `SequencerPatternSceneAudit` verifies capture, recall, summary, and save/restore behavior.
- Chord mode choices now include appended Maj 7, Dom 7, Sus, House 9, and Dub colors, and the SEQ preset menu includes Deep Chord, Dub Chord, Off Bass, and Rolling Bass starters. `SequencerHousePatternAudit` verifies these chord colors and presets.
- Groove mode choices now append House Shuf, Tech, Minimal, and Drive while keeping the original indexes stable, and the SEQ transform menu can apply genre templates that shape per-step timing, velocity, probability, length, lock feel, swing, gate, accent, probability, and lock routing together. `SequencerHousePatternAudit` verifies the appended choices and template behavior.
- Per-step ratchets are now saved in preset/session state, editable from the compact SEQ `Rat` lane and the expanded SEQ pattern focus overlay, rendered by playback, seeded in selected factory patterns, and included in exported MIDI; the first per-step `Cond` lane adds Always/Odd/Even/Fill phrase states for playback, scenes, state, and MIDI export; the first `Slide` lane saves legato overlap into live playback and exported/dragged MIDI. `SequencerRatchetAudit` verifies clamp, restore, playback timing, two-scene Fill parity, slide overlap, and MIDI ticks for repeat, condition, and slide behavior.

## Priority 5: FX And Club Processing

81. Add multi-band distortion.
82. Add movable crossovers and per-band mute/solo for drive.
83. Add amp/cabinet-style drive for rough warehouse tones.
84. Add tape, tube, diode, foldback, hard-clip, and sampler-rate drive characters.
85. Add pre/post gain matching for drive modules.
86. Add transient shaper for pluck/stab attack. First Guard-stage Punch pass implemented.
87. Add compressor. First Guard-stage Glue pass implemented; full dedicated compressor module remains future work.
88. Add sidechain-style ducking input for a future audio-effect build.
89. Add frequency shifter for minimal and techno motion.
90. Add resonator-bank module beyond the current single comb flavor.
91. Add chorus ensemble mode.
92. Add shimmer or pitch reverb mode only if it stays useful for club sound design.
93. Add delay diffusion mode.
94. Add reverse delay and tape delay color.
95. Add ping-pong and Haas-safe width options.
96. Add per-FX slot presets.
97. Add whole-rack presets.
98. Add parallel wet/dry routing for selected FX.
99. Add send-style delay/reverb throw bus inside the plugin. First internal send-bus pass implemented for delay/reverb send amounts, throw buttons, modulation destinations, and one-shot tail kill.
100. Add true slot-based FX instances only after fixed-module automation is proven stable.

Implemented FX sub-slice:

- Pump now publishes live phase, gain, peak reduction, and active-state telemetry from the DSP path, and the FX Pump curve display shows HOST/INT timing source, a moving phase marker, and a reduction meter. External Ableton sidechain input remains tracked separately because it requires bus-layout and host-validation work.
- The top-bar output meter now classifies output as `LOW`, `SAFE`, `HOT`, or `CLIP` from peak/RMS levels, and the FX Guard slot summary reports live peak headroom plus active Guard gain reduction.
- HOME now includes a compact Club Monitor that combines low-end meter, stereo-risk, Pump, Guard, and peak history into one dashboard readout for house bass and club-safety decisions.
- Guard now includes first-pass club dynamics: Glue compression, Punch transient lift, and Clip blend before the safety ceiling, with `GuardDynamicsAudit` coverage for punch, compression, and ceiling behavior. Dedicated compressor, limiter metering, and multiband dynamics remain future work.
- Drive now includes first-pass Bass Safe routing that keeps the low band closer to the clean reference while saturating upper content, with `DriveBassSafetyAudit` coverage. Full multiband drive, oversampling, and louderness-compensated audition remain future work.
- Delay and Reverb now have separate send amounts and dedicated send DSP state for throw tails, with appended MOD destinations for `FX Send Delay` and `FX Send Reverb`, plus `EffectsSendAudit` coverage for send tails and one-shot tail kill.
- Open implementation issues now track the broader house-production set: sidechain/pump (#64), compressor/clipper/transient safety (#65), pattern scenes and chord-stab tools (#66), visual analysis meters (#67), preset previews/construction kits (#68), percussive noise source (#69), send-style delay/reverb buses (#70), dedicated groove engine (#71), and chord/bassline designer (#72).

## Priority 6: Randomization And Sound Discovery

101. Add per-section randomization scopes: Source, Filter, Motion, FX, Sequencer, Sample, Library metadata.
102. Add lock buttons directly on parameter groups.
103. Add intensity control for randomization depth.
104. Add "useful only" randomization mode with gain and pitch guardrails.
105. Add recipe builder for user-defined randomizers.
106. Add randomization history with A/B compare.
107. Add mutation lanes for repeated small changes.
108. Add genre seed packs: UKG, deep house, tech house, minimal, warehouse techno.
109. Add "make darker/brighter/tighter/wider/dirtier/bouncier" transform buttons.
110. Add automatic preset naming suggestions based on generated traits.

## Priority 7: Library, Packs, And Metadata

111. Add custom user tags.
112. Add tag editing for existing user presets.
113. Add multi-select preset metadata editing.
114. Expand preset notes into structured production tips, source-recipe context, and batch-editable metadata.
115. Add pack artwork or pack color markers if the UI can support it cleanly.
116. Expand the implemented smart-crate filter pass, centered browser actions, and folder/pack/style crate rail into a full browser sidebar with editable user crates, New This Week, and pack crates.
117. Add audition auto-advance.
118. Extend the implemented preset-load compare/revert, Save Target preview, and two-click overwrite guard into full edited-vs-saved safe-overwrite compare.
119. Expand duplicate preset handling beyond same-folder overwrite detection with rename-as-copy, version history, and near-duplicate detection.
120. Add import/export preset pack bundles.
121. Add library repair/reindex command.
122. Add "find similar presets" using metadata and parameter distance.
123. Add factory construction kits containing preset, sequence, macro, and recommended BPM/key metadata.

Implemented library sub-slice:

- Macro preview values are now parsed from saved preset XML and surfaced in Library search, status text, tooltips, a `Macro Rich` filter, a `Macros` sort mode, and a compact eight-value browser strip for Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw. Narrow Library rows now collapse that dense strip into preview/source/rating columns so preset identity stays readable.
- A compact LIBRARY preset browser table now mirrors the active search/filter/sort result and shows name, category/folder, pack, key, BPM, rating, favorite marker, macro value strip, and preview-ready/stale/render-on-audition status with click-to-select and double-click-to-load behavior. The Crates column now includes quick filters, smart-crate/search/sort controls, centered browser actions, and a compact crate-map summary for visible/total, user/factory, folder, pack, generated, macro-rich, favorite, rated, and style-tagged counts, while Preset Profile shows a selected-preset role/trait/cue profile with Tone/Dirt/Motion/Space meters. The filter menu includes first-pass smart crates, while a larger full-browser sidebar, editable user crates, tag editing, and richer preview UI remain open.
- The Library Audition button now starts rendered preview cache/playback for bass, chord/stab, garage-chop, groove, FX, lead/pluck, and general presets without loading the active patch, and the Library `Warm` action can batch-render the selected/visible crate rows with row level badges from cached preview peak/RMS metadata. Fully background rendering, waveform badges, auto-preview, and editable preview phrase templates remain open.
- Library Save now has a two-click same-folder overwrite guard, and same-name presets in different category folders are preserved for real subfolder organization.

## Priority 8: Sound Quality And Safety

124. Add oversampling modes for drive, clipping, and aggressive filter behavior.
125. Add high-quality/offline render mode if the host exposes enough context.
126. Add denormal protection audit.
127. Add anti-click smoothing audit for all automatable parameters.
128. Expand gain staging tests for randomizer and factory presets beyond the first `FactoryPresetRenderAudit` finite/non-silent/peak smoke gate.
129. Add loudness-matched drive testing.
130. Add oscillator aliasing test renders.
131. Add voice stealing audit.
132. Add stuck-note and transport-jump stress tests.
133. Add output guard telemetry for how often limiting/clipping occurs.
134. Add preset safety validator before factory XML is accepted.

Implemented safety sub-slice:

- The persistent output meter now shows a club-production safety state from live output levels, and the Guard FX summary exposes current peak headroom plus active gain reduction. Deeper guard-hit history, limiter/clipper metering, and preset-level gain staging audits remain open.
- `FactoryPresetRenderAudit` now renders every repo-managed factory preset through the real preset loader and synth/sample/FX path, failing non-finite, silent, and runaway-peak output. Role-specific loudness targets, mono-bass validation, stuck-note checks, and broader gain-staging reports remain open.

## Priority 9: Ableton And Distribution

135. Add AU build target after VST3 is stable.
136. Add signed/notarized release workflow for macOS distribution.
137. Add installer or scripted copy workflow for the VST3 bundle.
138. Add versioned preset migration tests.
139. Add host automation name audit.
140. Add Ableton-focused test checklist: scan, load, automate, save set, reopen set, freeze/flatten, duplicate track.
141. Add MIDI learn if it can coexist cleanly with Ableton automation.
142. Add user manual pages for the main production workflows.
143. Add quick-start templates for UKG bass, house chord stab, tech-house rolling bass, minimal blip, and techno drone.

## Recommended Next Slices

1. Expand rendered preset previews beyond the implemented visible-row warmer and level badges into background generation, Library waveform badges, explicit regeneration controls, and preview loudness consistency. The earlier sample/FX Mod Env and Velocity source-parity and nonzero tail-reporting correctness slice is now implemented and covered by focused audits.
2. Move the house source-character pass forward with drawbar controls, legal/original digital-PCM-style chord/piano stabs, rubber/Reese/acid bass snippets, true wavetable import/playback, richer warp modes, and focused FM/operator color.
3. Add marker add/remove, beat/time slicing, per-slice playback modes, full relink handling, and slice-to-sequencer lanes beyond the first transient Detect, direct boundary dragging, stored start/end/pan/probability/nudge/fade, visual overlay, and missing-file stale-audio guard passes. `SamplerChopAudit` now protects the current load reset, transient detection, Slice Keys, probability, nudge/fade playback changes, missing-file restore clearing, and UKG chop setup baseline.
4. Add deeper sequencer arrangement tools: arranger-style chain editing beyond first-pass `Build 4-Bar Chain` and `Auto` / `2 Bar` / `4 Bar` modes, deeper per-scene automation/rate/follow behavior beyond first-pass live step/control chaining, richer Ableton audio/loop drag-out or export staging beyond first-pass MIDI drag-out, deeper chord-paint variants beyond first-pass Chord Stab Paint, deeper bassline contour generation beyond the first Bass Contour Shape transform, deeper slide/glide controls beyond the first overlap pass, richer condition vocabulary, MIDI import, and richer per-step modulation lanes.
5. Add the #84 house drum/kick-bass construction lane: kit pads, sample discovery, groove/accent lanes, kick/bass phase-length checks, rendered loop/one-shot drag-out, and construction-kit stems.
6. Define the #83 expansion-ready content pack manifest before the next large factory/content push: versioning, license metadata, preview media, dependencies, relink, and user/factory separation.
7. Add deterministic house performance transforms: make it more house, tighten low end, more groove, darker dub, shorter stab, delay throw variation, and reset mix-safe.
8. Add drag-to-modulate plus route min/max ranges, route curves, route slew, macro snapshots, and XY assignment editing.
9. Add multiband/bass-safe drive with oversampling, gain compensation, clearer clip/limiter metering, and loudness checks. First Bass Safe Drive routing is implemented; full multiband/oversampling/loudness work remains open.
10. Grow the compact LIBRARY browser and first smart-crate pass into a full browser with sidebar crates, tag editing, construction kits, relationship-aware compatible preset groups, dependency tracking, and pack import/export.
11. Build expandable Sequencer, FX, and Mod Matrix focus panels after the Macro and Sample focus overlays.
12. Run Ableton workflow validation: scan, load, automate, save/reopen, duplicate track, freeze/flatten, missing sample behavior, direct MIDI/audio drag-out, and preset loudness consistency.
