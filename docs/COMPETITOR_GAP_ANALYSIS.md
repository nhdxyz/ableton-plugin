# Nate VST Competitor Gap Analysis

Research date: 2026-06-28

This is a planning artifact for Nate VST. It reviews what larger synths, samplers, groove tools, and club FX do well, compares that against the current repo, and turns the gaps into a long implementation backlog. It should guide GitHub issues and small implementation slices.

Do not copy proprietary UI layouts, presets, wavetables, samples, DSP code, branding, or exact product workflows. The goal is to learn product patterns and build a focused house, UK garage, tech-house, techno, and minimal instrument.

## Sources Reviewed

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- Vital: https://vital.audio/
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate / wavestate native manual: https://www.korg.com/us/products/synthesizers/wavestate/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Ableton Live 12 Simpler and instrument reference: https://www.ableton.com/en/live-manual/12/live-instrument-reference/
- Ableton Live 12 MIDI Tools: https://www.ableton.com/en/live-manual/12/midi-tools/
- Cableguys ShaperBox 3: https://www.cableguys.com/shaperbox
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Output Portal / granular primer: https://output.com/blog/granular-synthesis-101-a-portal-exploration
- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Nate VST Snapshot

Nate VST already has a strong starter direction:

- JUCE/VST3 foundation with Ableton-focused validation and pluginval checks.
- Subtractive synth core with oscillator 1, oscillator 2, sub, noise, unison, filter character/slope, drive, output guard, and bandlimited oscillator work.
- Genre-oriented HOME dashboard with macros, randomization, low-end guidance, A/B snapshots, preset recall, and auditioning.
- MOD panel with LFO, envelope, eight macros, matrix routes, macro assignment editor, curve presets, bypass/delete, and destination badges.
- SAMPLE panel with waveform view, start/end handles, phrase markers, slice pads, reverse, pitch, gain, mix, and UKG chop helpers.
- SEQ panel with piano-roll editing, chord memory, scale/groove templates, velocity/probability/timing lanes, host sync, and MIDI export.
- FX rack with add/select flow, reordering, delay, reverb, distortion, pump, tremolo, ring mod, comb, chorus, phaser, flanger, bitcrush, EQ/tone, width, guard, and throw controls.
- LIBRARY panel with folders, categories, favorites, 1-5 ratings, metadata, search, sort, filters, recursive preset scanning, and macro previews.
- Factory preset generation for UKG, house, tech-house, techno, and minimal.

The main gap is no longer "we need more controls." The gap is "we need deeper sound engines, better visual feedback, stronger genre workflows, and a more professional browser/routing experience."

## Product Direction

Nate VST should become a fast club-music instrument, not a generic copy of every flagship synth.

- Sound target: tight basses, swung stabs, organ/garage chords, minimal plucks, warehouse techno hits, dubby chords, vocal chops, gritty sampler movement, and performance macros that matter.
- Workflow target: producers should be able to build a useful sound in under one minute, then go deep only when needed.
- UI target: fewer always-visible controls, stronger grouping, clearer visual feedback, better drag/edit behavior, and professional preset browsing.
- Technical target: stable Ableton behavior, safe gain staging, clean automation, low CPU, and no hidden preset-format traps.

## Long Feature Backlog

### P0 - UI, Layout, And Interaction Quality

1. Split `PluginEditor` into focused child components for HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, top bar, and keyboard.
2. Add automated component-overlap tests at the fixed plugin size.
3. Add screenshot regression tests for every panel.
4. Add a debug bounds overlay for layout review.
5. Add resizable UI support with at least 85%, 100%, 125%, and 150% scales.
6. Add a compact mode for smaller laptop screens in Ableton.
7. Add a large mode for sound-design sessions.
8. Add consistent section spacing, row heights, labels, and header treatment.
9. Replace dense knob grids with collapsible task groups.
10. Add page-level side navigation or sub-tabs where a page has too many groups.
11. Add a focused inspector strip that follows the last touched control.
12. Add value readouts that appear while dragging.
13. Add double-click reset for every knob and slider.
14. Add modifier-key fine adjustment for knobs.
15. Increase rotary drag hit areas without making the visual knob oversized.
16. Add drag-speed tuning so knobs feel less jumpy.
17. Add mousewheel/trackpad handling that is opt-in or gentler than current rotary movement.
18. Add clear hover states for controls, destinations, and modulation badges.
19. Add tooltips that describe action and musical result, not implementation details.
20. Add icon buttons for save, refresh, favorite, previous, next, audition, copy, undo, and reset.
21. Replace text-heavy utility buttons with icons where meaning is standard.
22. Add disabled-state visuals for controls that do not affect the current mode.
23. Add per-panel "focus" views so not every section is shown at once.
24. Add signal-flow mini-map across synth, sample, FX, and output.
25. Add mini level meters for synth layer, sample layer, FX input, FX output, and final output.
26. Add CPU/voice count indicators in the top bar.
27. Add a global panic/all-notes-off button.
28. Add a safe-output warning when Guard is working too often.
29. Add consistent visual language for source, modulation, sequence, effect, and library surfaces.
30. Add a dedicated visual design pass after component extraction.

### P1 - Core Synth Engines

31. Add a true wavetable oscillator with table position modulation.
32. Add wavetable preview display with current frame, morph path, and animated playhead.
33. Add factory wavetables for house bass, dub chords, tech-house plucks, techno noise, and UKG organ/bell tones.
34. Add safe wavetable import from standard WAV tables.
35. Add later Serum-format import compatibility if licensing and file assumptions are clean.
36. Add wavetable interpolation quality choices.
37. Add wavetable warp modes: bend, mirror, fold, sync-style, phase skew, quantize, bit shape, formant, and spectral tilt.
38. Add dual warp slots per wavetable oscillator.
39. Add oscillator sync with modulation depth.
40. Add PWM for square/pulse shapes.
41. Add ring modulation between oscillators.
42. Add FM/PM between oscillators with protected gain and anti-aliasing.
43. Add a focused three-operator FM engine for UKG bells, pings, organ edges, and metallic stabs.
44. Add a supersaw/unison engine with detune curve, phase spread, stereo spread, and mono safety.
45. Add analog drift controls for pitch, phase, filter, envelope, and voice variance.
46. Add a "garage organ" source model using additive drawbar-style partials.
47. Add a simple sampled-PCM source layer for M1-style house organs, strings, and classic stabs, using legal/original samples only.
48. Add a resonator/physical-model source for plucked minimal tones.
49. Add an additive partial bank for hollow bells and glassy techno hits.
50. Add noise color models: white, pink, brown, metallic, vinyl, air, and digital hash.
51. Add per-source mute, solo, and routing.
52. Add two full synth layers with independent source, filter, amp, and pan.
53. Add layer blend macros for performance.
54. Add layer copy/paste.
55. Add layer randomization locks.
56. Add source-level preset snippets.
57. Add init patches by source type: Sub Bass, Organ Bass, Chord Stab, Minimal Pluck, Techno Hit, Vocal Chop.
58. Add MPE-ready note expression routing for pressure, slide/timbre, and per-note pitch bend.
59. Add microtuning file support later if the parameter and UI scope remains manageable.
60. Add offline/high-quality oscillator render mode for export.

### P2 - Filters, Drive, And Tone Shaping

61. Add dual filters with serial, parallel, split, and per-layer routing.
62. Add filter blend between LP, BP, HP, notch, comb, formant, and vowel.
63. Add modeled filter characters inspired by ladder, SEM-style state-variable, MS-style aggressive, and clean digital modes.
64. Add formant/vowel filter mode for UKG vocalized bass and talky stabs.
65. Add keytracked filter cutoff with slope and center note.
66. Add filter FM from oscillator, noise, or audio-rate LFO with safety limits.
67. Add filter drive pre/post switch.
68. Add per-filter saturation color.
69. Add a bass-safe high-pass utility that can live before or after distortion.
70. Add transient-preserving drive mode for plucks and stabs.
71. Add soft clip, hard clip, diode, tube, tape, transformer, foldback, wavefold, fuzz, and digital degrade models.
72. Add gain compensation for every drive mode.
73. Add loudness-matched audition for drive models.
74. Add oversampling controls for drive and filter nonlinearities.
75. Add "club low-end" monitor that flags sub buildup, stereo bass, and excessive DC.

### P3 - Sampling, Chops, And UK Garage Tools

76. Add drag-and-drop WAV/AIFF import onto the waveform display.
77. Add sample replace by dropping from Finder or Ableton clip export.
78. Add sample file missing/relink handling.
79. Add transient-based slice detection.
80. Add beat-grid slicing.
81. Add equal-region slicing.
82. Add fully manual slice markers.
83. Add per-slice start, end, nudge, pitch, gain, pan, reverse, fade, and choke.
84. Add slice audition on hover/click.
85. Add slice keyboard mapping display.
86. Add slice to sequencer lane.
87. Add slice randomization with density and repetition guardrails.
88. Add per-slice probability.
89. Add per-slice stutter/retrigger.
90. Add one-shot, gate, loop, and thru playback modes.
91. Add zero-crossing snap for chop boundaries.
92. Add click-safe fades per slice.
93. Add formant-preserving pitch shift for vocal chops if DSP quality is acceptable.
94. Add time-stretch/warp mode for rhythmic vocal loops.
95. Add short granular sampler mode.
96. Add tape-stop and start-rate modulation for sample playback.
97. Add Akai-style 12-bit and sampler-rate color mode.
98. Add automatic sample key/BPM metadata fields.
99. Add UKG vocal-chop templates: call/response, late swing, stutter tag, reverse pickup, and sparse two-note hook.
100. Add Dred/Reese bass recipe buttons tied to source, filter, pitch, and drive.
101. Add organ-bass recipe buttons for UKG and speed garage.
102. Add "M1-style organ stab" and "house piano stab" starter recipes using legal/original sources.
103. Add a sample-pack browser inside LIBRARY.
104. Add construction-kit preset type that stores sample, sequence, macros, key, BPM, and recommended FX.
105. Add non-destructive crop/export helpers for user-created chops.

### P4 - Sequencer, Piano Roll, And Groove

106. Add multiple sequencer patterns per preset.
107. Add pattern chaining.
108. Add pattern scene buttons for A, B, fill, drop, breakdown, and transition.
109. Add per-step ratchet/retrigger.
110. Add per-step note length.
111. Add per-step slide/glide.
112. Add per-step accent tied to filter, amp, and drive.
113. Add per-step modulation lanes assignable to any safe destination.
114. Add per-step sample slice lane.
115. Add per-step pump depth lane.
116. Add per-step delay throw lane.
117. Add per-step reverb throw lane.
118. Add per-step probability ranges, not just one value.
119. Add step conditions such as first, not first, fill, every 2, every 4, and random.
120. Add scale quantize and key lock in the piano roll.
121. Add chord paint tools for minor 7, minor 9, major 7, sus, stab voicings, and garage organ shapes.
122. Add voicing spread control for chord memory.
123. Add humanize with separate timing, velocity, gate, and pitch.
124. Add groove templates: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, and broken percussion.
125. Add groove amount per lane.
126. Add swing microtiming visualization.
127. Add Euclidean generator for hats, plucks, and percussion-style synth hits.
128. Add shape/contour generator for basslines.
129. Add recombine tool that keeps rhythm but changes pitches.
130. Add density control for generated patterns.
131. Add MIDI drag export from the internal sequencer into Ableton.
132. Add MIDI drag import from a dropped MIDI clip or file.
133. Add sequencer preset snippets.
134. Add pattern undo/redo history.
135. Add transport-jump stress tests for host sync.

### P5 - Modulation And Performance Control

136. Add drag-and-drop modulation assignment from source chips to controls.
137. Add right-click "modulate with" menu on every automatable control.
138. Add visible modulation rings around knobs.
139. Add source-colored target badges with amount indicators.
140. Add hover-to-show routes for a parameter.
141. Add global "show routes for selected parameter" mode.
142. Add animated source activity indicators.
143. Add per-route min/max range editing.
144. Add per-route curve shaping.
145. Add per-route smoothing.
146. Add per-route slew, sample/hold, quantize, rectify, and randomize modifiers.
147. Add per-route invert and unipolar/bipolar mode.
148. Add per-route mute, solo, duplicate, copy, and paste.
149. Add per-route velocity scaling.
150. Add per-route keytracking scaling.
151. Add LFO 2 and LFO 3.
152. Add random LFO, sample-and-hold, smooth random, and chaos sources.
153. Add MSEG/multi-point envelope source.
154. Add step LFO with probability, hold, slew, and shape per step.
155. Add macro snapshot states for eight performance macro positions.
156. Add macro morphing between snapshots.
157. Add macro names editable per preset.
158. Add macro assignment overview table in LIBRARY or MOD.
159. Add XY pad assignment editor with multiple destinations per axis.
160. Add motion recorder for XY pad gestures.
161. Add envelope follower source for a future audio-effect build.
162. Add mod wheel, aftertouch, MPE pressure, and MPE slide source mapping.
163. Add performance page that only shows the most important macros and XY controls.
164. Add MIDI learn if it can coexist cleanly with Ableton automation.
165. Add automation-safe parameter naming audit.

### P6 - FX, Routing, And Club Processing

166. Add multiband distortion with movable crossovers.
167. Add per-band drive, dynamics, tone, feedback, mix, mute, and solo.
168. Add mid/side processing for drive, EQ, and width.
169. Add oversampling modes for nonlinear FX.
170. Add compressor module with sidechain-style envelope behavior.
171. Add transient shaper for pluck/stab attack.
172. Add noise gate/expander for chopped samples and hats.
173. Add frequency shifter.
174. Add barber-pole phaser/flanger preset styles.
175. Add resonator bank tuned to key/scale.
176. Add convolution or small IR reverb if CPU and licensing are clean.
177. Add shimmer/pitch reverb only if it remains useful for club patches.
178. Add delay diffusion mode.
179. Add dub delay feedback tone controls.
180. Add tape delay color.
181. Add reverse delay.
182. Add delay ducking.
183. Add granular FX for fills, transitions, and vocal texture.
184. Add time shaper/stutter FX with beat-synced curves.
185. Add volume shaper with sidechain-style presets.
186. Add filter shaper for rises, drops, and rhythmic low-pass motion.
187. Add noise shaper for risers, tops, and transient brightness.
188. Add DJ filter macro for quick transitions.
189. Add per-FX slot presets.
190. Add whole-rack presets.
191. Add parallel wet/dry routing for selected FX.
192. Add send bus inside the plugin for delay/reverb throws.
193. Add pre/post source routing so sample can bypass synth drive or share it.
194. Add drag-reorder for FX rows if current buttons feel too slow.
195. Add automated gain staging tests for every FX module.

### P7 - Randomization And Sound Discovery

196. Add visible lock buttons on every parameter group.
197. Add randomization scopes for Source, Filter, Amp, Motion, FX, Sequencer, Sample, Library metadata, and Macros.
198. Add intensity amount per scope.
199. Add "useful only" guardrails for pitch, output gain, sub level, resonance, feedback, and wet FX.
200. Add genre seed packs for UKG, deep house, tech-house, minimal, warehouse techno, dub techno, speed garage, and organ house.
201. Add recipe builder for user-defined randomizers.
202. Add mutation history.
203. Add A/B compare for generated patches.
204. Add "make darker", "brighter", "tighter", "wider", "dirtier", "bouncier", "warmer", "more minimal", and "more garage" transforms.
205. Add automatic preset name suggestions from generated traits.
206. Add random macro assignment generation.
207. Add random sequence generation that respects selected genre and scale.
208. Add random sample-slice generation that avoids unusable silence.
209. Add evolve/mutate button that keeps the preset identity while changing 10-20%.
210. Add breed/morph between preset A and preset B.
211. Add favorites-aware randomizer that learns from rated presets.
212. Add panic rollback if randomization creates silence or unsafe level.
213. Add preset safety validator before saving generated presets.
214. Add generated-preset notes explaining what changed.
215. Add randomizer test renders for gain and stuck-note safety.

### P8 - Library, Presets, And Packs

216. Add a real multi-column browser table: name, category, pack, rating, key, BPM, tags, source, modified, macro summary.
217. Add sortable columns with stable secondary sort.
218. Add tag editor for user presets.
219. Add custom user tags.
220. Add multi-select metadata editing.
221. Add preset notes.
222. Add smart folders: Favorites, Five Star, Recent, UKG Bass, UKG Vocal Chops, House Stabs, Minimal Plucks, Dirty Techno, Macro Rich, User, Factory.
223. Add folder tree view for user and factory libraries.
224. Add pack manager with import/export.
225. Add duplicate preset detection.
226. Add preset compare before overwrite.
227. Add preset version history for user saves.
228. Add missing sample warnings in browser rows.
229. Add per-preset audio preview render.
230. Add audition auto-advance.
231. Add preview MIDI patterns by category.
232. Add "find similar presets" using metadata and parameter distance.
233. Add preset dependency tracking for samples and wavetables.
234. Add pack artwork or color markers if it does not clutter the UI.
235. Add search tokens for macro assignments, engine type, FX type, sequence type, and sample type.
236. Add construction-kit browser category.
237. Add author/profile metadata.
238. Add rating and favorite persistence tests.
239. Add library repair/reindex command.
240. Add preset migration tests for future format changes.

### P9 - Audio Quality, Stability, And Testing

241. Add denormal protection audit.
242. Add all-parameter smoothing audit.
243. Add zipper-noise tests for automation sweeps.
244. Add oscillator aliasing render tests.
245. Add filter resonance stability tests.
246. Add drive oversampling tests.
247. Add stuck-note stress tests.
248. Add voice stealing audit.
249. Add sustain-pedal behavior tests.
250. Add host transport start/stop/jump tests.
251. Add save/reopen preset state tests.
252. Add Ableton-specific checklist: scan, load, automate, save set, reopen set, duplicate track, freeze, flatten, export, and reopen.
253. Add UI open/close stress test while audio runs.
254. Add output guard telemetry for internal debug builds.
255. Add factory preset loudness validation.
256. Add randomizer loudness validation.
257. Add sample import failure tests.
258. Add file-permission handling for user library saves.
259. Add crash-safe preset writing through temp file then atomic replace.
260. Add CI job for CMake build and unit-style non-GUI checks.

### P10 - Format, Distribution, And Ecosystem

261. Add AU build target after VST3 remains stable.
262. Consider CLAP only after JUCE and distribution impact are understood.
263. Add signed and notarized macOS release workflow.
264. Add installer or reliable copy script for `~/Library/Audio/Plug-Ins/VST3`.
265. Add versioned release notes per build.
266. Add crash-report collection approach for local beta testing.
267. Add plugin metadata audit: manufacturer, name, version, category, and automation names.
268. Add factory content installer path policy.
269. Add user manual pages for all panels.
270. Add quick-start guide for UKG bass, UKG vocal chop, house chord stab, tech-house rolling bass, minimal pluck, and techno hit.
271. Add preset-pack authoring guide.
272. Add test matrix for macOS Intel and Apple Silicon.
273. Add release checklist for Ableton Live versions.
274. Add migration notes for changed parameter layouts.
275. Add GitHub issue templates for feature slices, UI review, DSP changes, and release QA.

## What We Should Build First

The best next slices are the ones that improve professional feel before adding another large engine:

1. Componentize the editor and add layout-overlap tests.
2. Fix knob drag behavior, double-click reset, value popovers, and fine adjustment.
3. Build a real multi-column LIBRARY browser using the existing metadata and macro previews.
4. Add a focused modulation-routing UX: drag assignment, visible rings, selected-parameter route view, and route range editing.
5. Add multiple SEQ patterns, ratchet, note length, and scale-locked chord paint.
6. Add transient/manual sample slicing with per-slice controls.
7. Add wavetable oscillator playback with preview before import/editor work.
8. Add multiband drive with oversampling and gain compensation.
9. Add UKG recipe tools: Dred/Reese bass, organ bass, late stabs, vocal chop motifs, and swung 2-step patterns.
10. Add preset preview rendering and smart folders.
11. Add randomizer locks directly on UI groups.
12. Add Ableton save/reopen/freeze/flatten test checklist.

## Issue Creation Recommendation

Track the backlog through one umbrella issue plus focused child issues. Good first issue titles:

- Componentize editor panels and add layout regression checks.
- Improve rotary controls with fine drag, double-click reset, and value popovers.
- Build multi-column preset browser with smart folders.
- Add drag-and-drop modulation assignment and visible modulation rings.
- Add sequencer pattern chaining, ratchets, note length, and chord paint.
- Add transient/manual slicing with per-slice controls.
- Add wavetable oscillator playback and preview display.
- Add multiband distortion with oversampling and gain compensation.
- Add UKG recipe builder for Dred bass, organ bass, vocal chops, and late stabs.
- Add preset audio preview rendering and construction-kit metadata.
