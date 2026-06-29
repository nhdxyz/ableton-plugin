# Nate VST Deep Feature Audit

Research date: 2026-06-28

This is a focused "what else do we need to add?" audit for Nate VST. It compares the current plugin direction against large modern synths, samplers, groove tools, and club effects, then translates the gaps into a house, UK garage, tech-house, minimal, and techno roadmap.

The goal is not to clone any product. These are product-pattern references only. Do not copy proprietary UI layouts, preset names, samples, wavetables, DSP code, branding, or exact workflows.

## References Reviewed

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Ableton Live 12 instrument reference: https://www.ableton.com/en/live-manual/12/live-instrument-reference/
- Ableton Live 12 audio effect reference: https://www.ableton.com/en/live-manual/12/live-audio-effect-reference/
- Cableguys ShaperBox 3: https://www.cableguys.com/shaperbox
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Devious Machines Infiltrator 2: https://deviousmachines.com/product/infiltrator/
- Serato Sample: https://serato.com/sample
- XLN Audio XO: https://www.xlnaudio.com/products/xo
- Algonaut Atlas 2: https://algonaut.audio/
- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Main Takeaways

Modern flagship instruments are winning on workflow depth, not only feature count:

- Hybrid sources are normal now: wavetable, sample, granular, spectral, virtual analog, additive/harmonic, FM, and utility/noise engines.
- Modulation has to be visual, fast, and inspectable: drag assignment, target rings, source colors, route ranges, route curves, and activity meters.
- Motion tools are becoming lane-based: shape sequencers, wave sequencing, per-lane timing, random skips, performance gestures, and recordable XY motion.
- Club FX are visual and rhythmic: drawable volume/filter/time/noise/pitch curves, multiband processing, sidechain overlays, and per-effect presets.
- Sampling workflows are immediate: drag/drop audio, BPM/key analysis, cue pads, per-pad editing, random chops, beat-grid slicing, colored waveforms, and sample replacement.
- Library workflows are creative tools: tags, ratings, smart collections, audio previews, "similar sound" search, construction kits, and missing-dependency reporting.
- UK garage needs dedicated behavior: swung 2-step timing, chopped vocals, organ bass, Dred/Reese bass, late stabs, reverse pickups, stutters, and clean mono subs.

## Highest-Value Gaps For Nate VST

1. True wavetable oscillator with visual table display, position modulation, warp modes, and curated factory tables.
2. Source layer architecture: two full layers with mute, solo, blend, pan, width, filter routing, and layer presets.
3. Drag-style modulation workflow with modulation rings, route inspector, per-route range, curves, smoothing, and colored source activity.
4. MSEG and step-mod lanes for filter, pitch, wavetable position, FX mix, pump, delay, sample slice, and macro movement.
5. Serious UKG sampler: drag/drop import, transient slicing, beat-grid slicing, manual markers, per-slice fades, nudge, mode, probability, choke, and slice lanes.
6. Time-stretch, formant-safe pitch, and short granular modes for vocal chops.
7. Groove sequencer expansion: multiple patterns, pattern scenes, per-step modulation, ratchets, slides, step conditions, chord paint, MIDI drag-out, and MIDI drag-in.
8. Club FX upgrade: multiband distortion, volume/filter/time/noise/pitch shapers, sidechain input, transient shaper, compressor, limiter/clipper, and granular FX.
9. Library upgrade: rendered audio previews, smart folders, sample browser, construction kits, dependency checks, and "find similar" later.
10. UI architecture upgrade: split `PluginEditor` into pages/components, add layout tests, screenshot regression, scale modes, and focused subviews.

## Long Add List

### 1. UI And Interaction

1. Split the giant editor into page components: HOME, SYNTH, MOD, SAMPLE, SEQ, FX, LIBRARY, top bar, and keyboard.
2. Add screenshot regression tests for every panel.
3. Add automated layout-overlap checks at supported UI scales.
4. Add resizable UI support: 85%, 100%, 125%, 150%, and 200% later.
5. Add compact Ableton laptop mode.
6. Add focused edit modes so pages show Source, Shape, Motion, Space, or Utility instead of everything at once.
7. Extend the implemented HOME signal-flow strip into deeper synth/sample split, send, parallel, Guard, and output routing views.
8. Add a selected-control inspector with value, automation name, modulation routes, reset, and MIDI learn.
9. Add bigger invisible hit boxes on small knobs.
10. Add more predictable drag sensitivity classes for knobs and sliders.
11. Add value popovers while dragging every continuous control.
12. Add double-click reset coverage everywhere.
13. Add optional mousewheel editing, disabled by default for trackpad safety.
14. Add icon buttons for save, load, favorite, audition, undo, redo, copy, paste, clear, up, down, and random.
15. Add clearer inactive/disabled styling for controls that do not affect the current mode.
16. Add a global panic/all-notes-off button. First bottom-keyboard panic pass implemented for keyboard, chord memory, synth MIDI, and sampler voices.
17. Add top-bar CPU estimate, voice count, output guard, and oversampling status.
18. Add a first-run "compact help" overlay only if it can be dismissed permanently.

### 2. Source Engines

19. Add a true wavetable oscillator.
20. Add animated wavetable preview and current-position marker.
21. Add factory wavetables for UKG organ, bell, rubber bass, dub stab, house chord, acid edge, and techno noise.
22. Add WAV wavetable import.
23. Add wavetable interpolation modes.
24. Add wavetable warp modes: bend, fold, mirror, sync, phase skew, formant tilt, spectral tilt, and bit shape.
25. Add oscillator hard sync.
26. Add PWM for square/pulse sounds.
27. Add ring modulation between oscillators.
28. Add protected FM/PM between oscillators.
29. Add a focused three-operator FM engine for UKG bells, metallic stabs, pings, and techno hits.
30. Add additive/drawbar organ source for UKG and speed garage.
31. Add legal original PCM-style starter sources for house organ, piano-ish stabs, strings, and digital bells.
32. Add granular sample oscillator for texture and vocal cloud patches.
33. Add spectral/resynthesis investigation for future vocal and texture engines.
34. Add resonator/physical-model source for minimal plucks.
35. Add colored noise models: white, pink, brown, air, vinyl, metallic, digital hash, and attack tick.
36. Add analog drift for pitch, phase, filter, envelope, and pan.
37. Add mono-safe unison mode for bass.
38. Add supersaw/unison detune curves.
39. Add voice-stack modes: octave, chord, spread, and Reese.
40. Add source snippet presets.

### 3. Layers, Filters, Drive, And Tone

41. Add two full synth layers with independent source, filter, amp, pan, width, tune, and output trim.
42. Add layer mute, solo, copy, paste, and swap.
43. Add layer randomization locks.
44. Add layer macro targets for Weight, Width, Dirt, and Motion.
45. Add dual filters with serial, parallel, split, and per-layer routing.
46. Add notch, formant, vowel, comb, all-pass, and morphing multimode filters.
47. Add modeled filter characters inspired by clean digital, ladder, state-variable, MS-style, and diode-like behavior.
48. Add filter keytracking with slope and center note.
49. Add filter FM with safety limits.
50. Add pre/post-filter drive routing.
51. Add drive models: tape, tube, transformer, diode, hard clip, soft clip, foldback, wavefold, fuzz, and digital degrade.
52. Add drive gain compensation and loudness-matched audition.
53. Add oversampling modes for nonlinear filters and drive.
54. Add bass-safe high-pass before heavy drive.
55. Add transient-preserving drive for plucks and stabs.
56. Add low-end assistant upgrades: DC detection, sub stereo warning, mono compatibility, and guard hit-rate history.

### 4. Modulation And Motion

57. Add drag assignment from mod source chips onto controls.
58. Add modulation rings around knobs and amount stripes on horizontal sliders.
59. Add source-colored route badges on targets.
60. Add hover-to-inspect route overlays.
61. Add per-route min/max ranges.
62. Add per-route curve shaping.
63. Add per-route smoothing/slew.
64. Add per-route invert and unipolar/bipolar behavior.
65. Add route mute, solo, duplicate, copy, paste, and clear.
66. Add MSEG with multi-point drawing.
67. Add step LFO with per-step value, hold, curve, probability, and slew.
68. Add extra LFOs after the MSEG architecture is stable.
69. Add modulation processors: scale, offset, clamp, quantize, lag, rectify, and randomize.
70. Add audio-rate modulation targets for FM, filter FM, and oscillator warp.
71. Add envelope follower modulation source.
72. Add macro scenes A/B/C/D.
73. Add macro morphing between scenes.
74. Add recordable XY gestures.
75. Add Korg-style motion controller experiment: physics-like XY inertia, friction, return-to-center, and bounce.
76. Add mod wheel, aftertouch, MPE pressure, MPE slide, and per-note pitch bend sources.

### 5. Sampler And UKG Chops

77. Add drag-and-drop WAV/AIFF import onto the waveform.
78. Add sample replacement while preserving slice settings.
79. Add missing sample relink flow.
80. Add transient slice detection.
81. Add beat-grid slice detection.
82. Add equal-region slicing with user-selected count.
83. Add manual marker creation, deletion, and nudge.
84. Add zero-cross snapping at slice boundaries.
85. Add per-slice fade in/out.
86. Add per-slice playback mode: one-shot, gate, loop, thru, reverse, and ping-pong later.
87. Add per-slice start offset and end nudge as small buttons/handles.
88. Add per-slice pitch envelope/ramp with UKG pickup presets.
89. Add per-slice FX send.
90. Add per-slice color labels and role labels: kick, stab, vox, fill, ghost, pickup, answer.
91. Add keyboard mapping display for Slice Keys.
92. Add slice audition on hover or click.
93. Add slice-to-sequencer lane.
94. Add random chop generator with lockable slices.
95. Add UKG chop templates: call/response, late swing, reverse pickup, sparse hook, two-note vocal, stutter tag, and pitch-ramp answer.
96. Add sample BPM and key detection.
97. Add formant-safe pitch shifting investigation.
98. Add short time-stretch/warp mode for rhythmic phrases.
99. Add short granular sampler mode for fills.
100. Add Akai-style 12-bit and sampler-rate color.
101. Add resample-current-patch-to-sampler.
102. Add render-current-sequence-to-audio.

### 6. Sequencer, Piano Roll, And Groove

103. Add multiple patterns per preset.
104. Add pattern scenes: A, B, fill, drop, breakdown, transition, and random variation.
105. Add pattern chaining.
106. Add 32-step and 64-step modes.
107. Add per-step ratchet/retrigger.
108. Add per-step slide/glide.
109. Add per-step accent mapped to amp, filter, and drive.
110. Add per-step modulation lanes with safe destination picking.
111. Add per-step sample slice lane.
112. Add per-step pump depth lane.
113. Add per-step delay throw and reverb throw lanes.
114. Add step conditions: first, not first, every 2, every 4, fill, random, and follow.
115. Add probability ranges rather than a single probability value.
116. Add chord paint tools for minor 7, minor 9, major 7, sus, garage organ, and dub chords.
117. Add scale quantize and key lock.
118. Add groove template library: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, and broken percussion.
119. Add lane-specific groove amount.
120. Add swing microtiming visualization.
121. Add Euclidean generator for minimal blips and percussion-style synth hits.
122. Add bassline contour generator for UKG and tech-house.
123. Add recombine tool: keep rhythm, change notes.
124. Add MIDI drag-out directly from the UI into Ableton.
125. Add MIDI drag-in from dropped clips/files.
126. Add host-transport jump stress tests.

### 7. FX, Routing, And Club Processing

127. Add true multiband distortion with movable crossovers.
128. Add per-band drive, dynamics, tone, feedback, width, mix, mute, and solo.
129. Add mid/side processing in drive, EQ, and width modules.
130. Add oversampling choices for nonlinear FX.
131. Add compressor with sidechain-style envelope behavior.
132. Add limiter/clipper with clear gain-reduction and clip meters.
133. Add transient shaper for stabs, plucks, bass clicks, and vocal chops.
134. Add noise gate/expander for sample cleanup.
135. Add frequency shifter.
136. Add resonator bank and tuned comb bank.
137. Add volume shaper with house, UKG, offbeat, and straight pump presets.
138. Add filter shaper with drawable rhythmic cutoff curves.
139. Add time/stutter shaper.
140. Add noise shaper for tops, risers, and transient brightness.
141. Add pitch/formant shaper for vocal movement.
142. Add reverse delay.
143. Add tape delay color.
144. Add delay ducking.
145. Add delay diffusion.
146. Add granular FX for fills and transition textures.
147. Add tape stop/time slowdown module.
148. Add DJ filter macro.
149. Add external sidechain input for real Ableton kick ducking.
150. Add parallel wet/dry routing for selected FX.
151. Add internal send bus for delay/reverb throws.
152. Add whole-rack preset save/load.
153. Add per-FX slot preset save/load.

### 8. Browser, Presets, Samples, And Packs

154. Add rendered audio preview files per preset; runtime role-aware MIDI audition phrases are implemented as the first preview pass.
155. Add auto-play preview while arrowing through presets.
156. Add macro preview columns in the browser.
157. Expand smart collections beyond the first implemented filter-menu crate pass with New This Week, editable user crates, pack crates, full-browser sidebar browsing, and multi-select metadata actions.
158. Add editable user tag manager.
159. Add pack installer/exporter.
160. Add construction-kit presets storing patch, sample, sequence, key, BPM, macro intent, and recommended FX.
161. Extend preset compare/revert into generated-slot and edited-vs-saved workflows. First explicit Library load compare/revert, same-folder overwrite guard, and Save Target visual preview passes are implemented.
162. Add random mutation history.
163. Add "similar preset" later using metadata first, audio fingerprinting later.
164. Add sample folder indexing.
165. Add sample preview and waveform thumbnails in the library.
166. Add duplicate sample detection.
167. Add missing-file dependency report.
168. Add "replace sample with similar" later.
169. Add project-specific crates/playlists.
170. Expand implemented author, pack, and notes metadata with license fields and batch editing.
171. Expand beyond the current 88-preset factory pack with Club FX, license metadata, pack-level QA, vocal tags, and more tech-house loop starters. First Speed Garage, Bass House, Chicago House, Classic House, Funky House, Melodic House, Romanian Minimal, Electro Breaks, Deep Tech, Indie Dance, Italo Disco, Balearic House, Acid House, Nu Disco, Afro Tech, Afro Melodic, Progressive House, Hard House, Peak Time Techno, Detroit Techno, Melodic Techno, Deep Minimal, Lo-Fi House, Minimal FM, French House, Soulful House, Garage House, Microhouse, Raw Techno, Tribal Tech House, Breaks House, Hardgroove, Future Garage, Amapiano-inspired, Warehouse Techno, Dub Stabs, smart-crate, and curated preset metadata coverage is implemented.

### 9. Randomization And Sound Discovery

172. Add randomize-by-intent controls: more garage, tighter, darker, brighter, dirtier, cleaner, wider, more minimal, more warehouse, more bounce.
173. Add section locks for every major feature group.
174. Add randomization strength per section.
175. Add macro-safe randomization that preserves playable level and pitch center.
176. Add genre-aware sequence generation.
177. Add garage chop randomizer with repetition guardrails.
178. Add bass recipe generator: organ bass, Dred, Reese, rubber tech-house, minimal sub, acid pulse.
179. Add stab recipe generator: late garage chord, dub stab, house organ stab, techno hit.
180. Add FX-rack randomizer with loudness and wet/dry safety.
181. Add "mutate preset" with undoable history.
182. Add A/B/C/D snapshots.
183. Add snapshot morphing.

### 10. Ableton Integration, Quality, And Release

184. Add Ableton save/reopen regression checklist.
185. Add freeze/flatten validation in Ableton.
186. Add automation-name audit before large new parameter batches.
187. Add parameter migration tests for old presets and DAW states.
188. Add non-GUI unit tests for preset IO, sample state, sequencer state, and parameter defaults.
189. Add DSP smoke tests for silence, NaN, denormal, and clipping.
190. Add render stress tests for high polyphony, heavy modulation, and transport jumps.
191. Add pluginval to CI or a documented local release gate.
192. Add AU build support for macOS.
193. Add macOS signing and notarization workflow.
194. Add installer or copy script for VST3, AU, presets, and factory assets.
195. Add changelog generation from commits/issues.
196. Add issue templates for feature, bug, UI, DSP, and Ableton validation.
197. Add release checklist for Intel and Apple Silicon.

## Best Next Slices

1. UI architecture and screenshot/layout tests. This protects every future feature.
2. Wavetable oscillator plus visual table display. This is the biggest missing synth-engine signal compared with modern competitors.
3. Drag-style modulation and visible modulation rings. This immediately makes the plugin feel more professional.
4. UKG sampler drag/drop plus transient/manual slicing. This directly supports the genre target.
5. Volume/filter/time shaper module. This gives the FX rack a modern rhythmic club-processing identity.
6. Construction-kit preset type. This ties synth, sample, sequence, key, BPM, and macros into a workflow that is more useful for actual house production than isolated patches.
