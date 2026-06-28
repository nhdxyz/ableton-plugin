# Nate VST Deep Research Backlog

Research date: 2026-06-28

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
- Stable JUCE/VST3 foundation with pluginval validation.
- PolyBLEP oscillator pass for better bright-note behavior.
- Filter character and slope options.
- Oscillator warp control.
- FX rack with add/select workflow, module summaries, sync delay, pump, width, guard, and custom pump curves.
- MOD panel with sources, destinations, route bypass/delete, curve presets, macro assignment editing, and destination badges.
- Sequencer with piano-roll editing, chord memory, editable velocity/probability/timing lanes, groove timing, templates, and host transport sync.
- Sampler/chop workflow for UKG vocal and stab material.
- Library with folders, categories, favorites, ratings, metadata, macro previews, search, sort, and recursive preset scanning.
- Factory preset generator for genre-targeted packs.

## Product Gaps

The largest remaining gap is no longer "does the plugin have enough controls." It is now "does each control feel fast, visual, musical, and reliable inside Ableton." The next work should favor interaction quality, source depth, browser usefulness, and club-specific timing tools over adding more unfocused knobs.

See `docs/COMPETITOR_GAP_ANALYSIS.md` for the expanded 2026-06-28 competitor refresh and a longer cross-product gap list. This file remains the shorter implementation backlog.

## Priority 0: UI And Workflow Corrections

1. Extract HOME, SYNTH, SAMPLE, MOD, SEQ, FX, LAB, and LIBRARY panels into smaller editor components.
2. Add automated layout overflow checks for every page at the fixed plugin size.
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

## Priority 1: Modulation And Movement

16. Add drag-style modulation assignment from source chips to destination controls.
17. Add right-click or menu assignment from any automatable control.
18. Add a macro assignment editor showing destination, amount, polarity, and range. First pass implemented for macro source, destination, amount, add/update, replace, clear, and summary; explicit min/max range editing remains open.
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
45. Add supersaw/unison engine improvements with phase, detune curve, and stereo spread.
46. Add analog drift with per-voice pitch, phase, and filter variation.
47. Add a second full synth layer.
48. Add per-layer mute/solo and blend controls.
49. Add partial/layer performance macros inspired by workstation-style browsing, without copying ZEN-Core.
50. Add source-level randomization locks.

## Priority 3: Sampler, Chops, And UKG Tools

51. Add transient-based slice detection.
52. Add manual slice markers in the SAMPLE waveform view.
53. Add per-slice start/end/nudge.
54. Add per-slice pitch and gain.
55. Add per-slice reverse.
56. Add slice randomization with musical density limits.
57. Add choke groups for vocal chops.
58. Add one-shot, gate, and loop modes per slice.
59. Add start-crossfade and end-crossfade to reduce clicks.
60. Add formant-preserving pitch mode if a suitable DSP approach is chosen.
61. Add time-stretch mode for short vocal material.
62. Add granular sampler mode for texture, pads, and fill effects.
63. Add "garage chop" presets for common slice timing layouts.
64. Add drag-and-drop WAV/AIFF sample import in the plugin UI.
65. Add a sample pack browser inside LIBRARY.

## Priority 4: Sequencer, Piano Roll, And Groove

66. Add multiple sequencer patterns per preset.
67. Add pattern chaining.
68. Add per-step probability. First editable lane pass implemented in the SEQ grid.
69. Add per-step ratchet/retrigger.
70. Add per-step note length.
71. Add per-step velocity lane. First editable lane pass implemented in the SEQ grid.
72. Add per-step modulation lane assignments.
73. Add per-step sample slice lane.
74. Add per-step pump depth lane.
75. Add per-step delay throw lane.
76. Add scale quantize and key lock for the piano roll.
77. Add chord paint tools for UKG stabs.
78. Add groove templates for 2-step, shuffle, swung 16ths, minimal offbeats, and techno pulse.
79. Add humanize with separate timing, velocity, and gate controls.
80. Add MIDI drag export from the internal sequencer into Ableton.

## Priority 5: FX And Club Processing

81. Add multi-band distortion.
82. Add movable crossovers and per-band mute/solo for drive.
83. Add amp/cabinet-style drive for rough warehouse tones.
84. Add tape, tube, diode, foldback, hard-clip, and sampler-rate drive characters.
85. Add pre/post gain matching for drive modules.
86. Add transient shaper for pluck/stab attack.
87. Add compressor.
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
99. Add send-style delay/reverb throw bus inside the plugin.
100. Add true slot-based FX instances only after fixed-module automation is proven stable.

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
114. Add preset notes.
115. Add pack artwork or pack color markers if the UI can support it cleanly.
116. Add smart folders such as Favorites, Five Star, Recent, UKG Bass, House Stabs, Dirty Techno.
117. Add audition auto-advance.
118. Add preset compare before overwrite.
119. Add duplicate preset detection.
120. Add import/export preset pack bundles.
121. Add library repair/reindex command.
122. Add "find similar presets" using metadata and parameter distance.
123. Add factory construction kits containing preset, sequence, macro, and recommended BPM/key metadata.

Implemented library sub-slice:

- Macro preview values are now parsed from saved preset XML and surfaced in Library search, status text, tooltips, a `Macro Rich` filter, and a `Macros` sort mode. The broader browser-column request remains open for a fuller multi-column browser view.
- A compact LIBRARY preset browser table now mirrors the active search/filter/sort result and shows name, category/folder, pack, key, BPM, rating, favorite marker, and macro summary with click-to-select and double-click-to-load behavior. A larger full-browser mode, smart folders, tag editing, and audio preview rendering remain open.

## Priority 8: Sound Quality And Safety

124. Add oversampling modes for drive, clipping, and aggressive filter behavior.
125. Add high-quality/offline render mode if the host exposes enough context.
126. Add denormal protection audit.
127. Add anti-click smoothing audit for all automatable parameters.
128. Add gain staging tests for randomizer and factory presets.
129. Add loudness-matched drive testing.
130. Add oscillator aliasing test renders.
131. Add voice stealing audit.
132. Add stuck-note and transport-jump stress tests.
133. Add output guard telemetry for how often limiting/clipping occurs.
134. Add preset safety validator before factory XML is accepted.

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

1. Build automated editor layout checks and component extraction for the largest pages.
2. Fix rotary control feel with fine drag, double-click reset, and value popovers.
3. Build a real multi-column LIBRARY browser from the existing metadata and macro previews.
4. Add drag/menu modulation assignment plus selected-parameter route views.
5. Add sequencer ratchets, note length, pattern slots, and chord paint.
6. Add sample slice markers and per-slice controls.
7. Add user tags, smart folders, and preset preview rendering in LIBRARY.
8. Add wavetable oscillator playback after UI layout risk is reduced.
9. Add multiband drive with oversampling, gain compensation, and loudness checks.
10. Add UKG recipe tools for Dred bass, organ bass, late stabs, and vocal chop motifs.
