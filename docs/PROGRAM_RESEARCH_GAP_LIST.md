# Nate VST Program Research Gap List

Research date: 2026-06-28

This document is a deeper "what else should we add" pass for Nate VST. It compares the current repo direction against larger synths, groove tools, samplers, and club effects, then translates the gaps into house, UK garage, tech-house, techno, and minimal-focused work.

The goal is not to copy any product. Use the references as product-pattern research only. Do not copy proprietary layouts, presets, samples, wavetables, DSP code, names, branding, or exact workflows.

## References Reviewed

Synth and hybrid instrument references:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- Minimal Audio Current: https://www.minimal.audio/products/current
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Korg Collection: https://www.korg.com/us/products/software/korg_collection/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Ableton Live instrument reference: https://www.ableton.com/en/live-manual/12/live-instrument-reference/
- Ableton Live MIDI tools: https://www.ableton.com/en/live-manual/12/midi-tools/

FX, browser, and motion references:

- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Output Portal: https://output.com/products/portal
- Devious Machines Infiltrator: https://deviousmachines.com/product/infiltrator/

UK garage and club-production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Nate VST Baseline

Nate VST already has a useful skeleton:

- JUCE/CMake/VST3 project that builds on macOS and installs into the user VST3 folder.
- House/UKG/tech-house/minimal/techno product direction.
- Subtractive synth layer with oscillator mix, sub, noise, filter character/slope, drive, envelopes, unison, Osc Warp, and output guard.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, and LIBRARY panels.
- Performance macros, XY macro pad, randomization, A/B snapshots, and low-end guidance.
- LFO curve editor, pump curve editor, modulation matrix, macro assignment editing, route bypass/delete, destination badges, and S&H/Smooth/Chaos movement.
- Sample waveform area, chop window, phrase markers, slice pads, reverse, pitch ramp, stutter, and UKG chop helpers.
- Piano-roll style 16-step sequencer with scale/chord helpers, velocity/probability/timing/length lanes, groove templates, host sync, undo, rotate, variation, and MIDI export.
- Addable/reorderable FX rack with delay, reverb, drive, pump, tone/EQ, width, guard, modulation FX, throws, and module presets.
- Preset library with recursive folders, categories, favorites, 1-5 ratings, metadata, search, sort, filters, macro previews, compact browser rows, and audition.
- Factory starter presets for UKG, house, tech-house, techno, and minimal.
- Local pluginval validation has been used during feature slices, but CI, automated non-GUI tests, and a repeatable Ableton release checklist are still missing.

The main gap is now professional depth: source engines, motion design, focused browsing, sampler chops, high-quality drive/filter behavior, and visual modulation need to feel closer to mature instruments.

## Program-by-Program Gap Map

### Serum 2 Pattern

Reference pattern:

- Modern wavetable workflow is still table-position focused, but now broader instruments pair wavetable with sampled, granular, spectral, and sequencing tools.
- Modulation must be fast to assign and visible while editing.
- FX and modulation are part of the main sound, not an afterthought.

Nate VST gaps:

- True wavetable oscillator, table preview, position modulation, and warp modes.
- Sample/granular/spectral-style source options for modern hybrid patches.
- Drag-style modulation assignment and rings on every important control.
- Better visual "what is moving" feedback for macros, LFOs, random sources, and envelopes.
- Clip/pattern workflow that can generate usable garage bass, stab, and pluck motifs.

### Arturia Pigments Pattern

Reference pattern:

- Hybrid instruments organize multiple engines, utilities, modulation, sequencing, and effects into clear focused areas.
- Strong visual feedback makes complex modulation approachable.
- A browser and preset system are central to the workflow.

Nate VST gaps:

- Multiple source engines need a cleaner source rack, not more knobs on one page.
- Utility engines such as sub/noise/sampler should be grouped as deliberate layers.
- The UI needs stronger focus modes so a producer sees only Source, Shape, Motion, Space, or Utility when needed.
- The library needs audio previews, smart folders, user tags, and pack workflows.

### Phase Plant Pattern

Reference pattern:

- Modular generator and effect lanes let advanced users build layered sounds without forcing every user into a fixed subtractive layout.
- Modulators are reusable, visible, and assignable across the patch.

Nate VST gaps:

- Layer architecture is still single-layer.
- Source, filter, sample, and FX routing is mostly fixed.
- There is no reusable source snippet or rack preset system yet.
- Modulation sources cannot yet be dropped onto destinations.
- Route modifiers such as smoothing, range, curves, unipolar/bipolar mode, and scaling are missing.

### Minimal Audio Current Pattern

Reference pattern:

- Modern synths increasingly combine synth engines, effects, modulation, and cloud/library workflows in one discovery-oriented product.
- Preset exploration and macro-friendly patches are a first-class part of the instrument.

Nate VST gaps:

- Library is strong for metadata but still lacks rendered audio preview and auto-advance.
- No "find similar" or favorites-aware generation.
- No construction-kit preset type that stores sequence, sample references, macro intent, BPM, and key together.
- No dependency warnings for samples and future wavetables.

### ZENOLOGY Pro Pattern

Reference pattern:

- Workstation-style instruments lean on partials/layers, large sound libraries, macro controls, model expansions, and curated browsing.
- The workflow is often preset-first, then edited.

Nate VST gaps:

- No partial/layer model beyond the current single synth plus sample path.
- No second layer, layer mute/solo, copy, blend, or independent layer filtering.
- No partial-style source browser for quickly choosing organ, bass, bell, stab, and noise roles.
- Factory content needs more complete pack identity and construction-kit metadata.

### u-he Pattern

Reference pattern:

- The product strength is often oscillator/filter quality, musical envelopes, character, CPU-aware quality settings, and carefully scoped modulation.
- Quality matters more than an inflated feature count.

Nate VST gaps:

- Drive and filter nonlinearities need oversampling options and gain compensation.
- Oscillator aliasing, automation zipper noise, denormal handling, and voice stealing need render/stress tests.
- Filter models need more character and better keytracking.
- UI/component architecture needs to be easier to maintain before adding very large engines.

### Korg modwave/wavestate Pattern

Reference pattern:

- Motion sequencing, wave sequencing, vector control, performance knobs, and randomization are central to Korg's modern motion instruments.
- Per-lane timing and modulation can make static sounds feel alive.

Nate VST gaps:

- Sequencer has useful note lanes but no per-step modulation lanes yet.
- No pattern scenes or pattern chaining.
- No vector/motion recorder for XY gestures.
- No motion lanes for wavetable/sample/filter/FX properties.
- Randomization does not yet create lane-based motion or "more garage/more warehouse" transforms.

### Ableton Pattern

Reference pattern:

- Ableton workflows reward drag-and-drop, automation-safe names, rack-style macro thinking, MIDI clip exchange, and fast save/reopen reliability.

Nate VST gaps:

- MIDI drag export is not direct from the UI into Ableton yet.
- MIDI drag import is missing.
- Ableton save/reopen/freeze/flatten checklist should become a repeatable release test.
- Automation naming and parameter migration need an audit before adding many engines.
- AU support, signing, notarization, installer/copy script, and release notes need a release workflow.

### ShaperBox, Saturn, Portal, And Infiltrator Pattern

Reference pattern:

- Club FX tools thrive on drawn curves, multiband processing, animated timing, per-effect presets, wet/dry routing, and performance throws.
- Good rhythmic effects are visual and lane-based.

Nate VST gaps:

- No multiband distortion yet.
- No volume/filter/noise/time shaper modules with drawable curves.
- No whole-rack preset system.
- Delay/reverb throws exist, but an internal send bus would make performance workflows deeper.
- No granular FX for vocal fills and transition texture.

### UK Garage Production Pattern

Reference pattern:

- UKG needs swung 2-step timing, late stabs, chopped vocals, organ and Reese/Dred bass, clean sub control, shuffle, call/response hooks, and tight sample cuts.

Nate VST gaps:

- Sample slicing is a starter workflow; it needs transient/manual markers, per-slice edits, choke, probability, fades, and slice lanes.
- Organ/drawbar-style source is missing.
- Dred/Reese recipe tools are missing as first-class actions.
- Groove templates need more microtiming and lane-specific swing.
- Factory packs need more UKG construction kits, not just individual presets.

## Long Add List

### 1. Immediate UI And Workflow Additions

1. Extract each editor page into its own component.
2. Add automated overlap checks for every panel.
3. Add screenshot regression captures for HOME, SYNTH, MOD, SAMPLE, SEQ, FX, LAB, and LIBRARY.
4. Add a debug layout bounds overlay.
5. Add UI scale choices: 85%, 100%, 125%, and 150%.
6. Add compact Ableton laptop mode.
7. Add a larger sound-design mode.
8. Add collapsible groups for dense pages.
9. Add task-focused page sub-tabs: Source, Shape, Motion, Space, Utility.
10. Add a selected-control inspector strip.
11. Add per-control value popovers while dragging.
12. Add consistent double-click reset coverage.
13. Add consistent fine-drag behavior coverage.
14. Add larger invisible hit areas on small knobs.
15. Add icon buttons for common actions.
16. Add stronger hover/active/disabled states.
17. Add output, voice count, and CPU estimate in the top bar.
18. Add global panic/all-notes-off.
19. Add Guard activity telemetry in the UI.
20. Add a signal-flow mini-map across source, sample, filter, FX, and output.
21. Add routing visualization for inserts, throws, sends, safety/Guard, and future parallel paths.
22. Add domain color language for source, modulation, sequence, effect, library, and safety states.

### 2. Source Engine Additions

23. Add true wavetable oscillator playback.
24. Add wavetable position modulation.
25. Add wavetable preview and playhead.
26. Add factory wavetables for UKG organ/bell, house chord, rubber bass, dub stab, and techno noise.
27. Add wavetable warp modes: bend, fold, mirror, sync-style, phase skew, spectral tilt, and formant tilt.
28. Add safe WAV wavetable import after factory playback is solid.
29. Add PWM for pulse sounds.
30. Add oscillator hard sync.
31. Add oscillator ring modulation.
32. Add protected FM/PM between oscillators.
33. Add focused 3-operator FM for bells, metallic stabs, pings, and techno hits.
34. Add additive/drawbar organ source for UKG and speed garage.
35. Add legal/original sampled-PCM starter sources for house organ and piano-style stabs.
36. Add noise colors: white, pink, brown, air, vinyl, metallic, and digital hash.
37. Add supersaw detune curve, phase spread, and mono-safe spread.
38. Add per-voice analog drift.
39. Add second synth layer.
40. Add per-layer mute, solo, copy, blend, and pan.
41. Add per-layer filter/amp envelopes once layer routing is stable.
42. Add source snippet presets.

### 3. Filter, Tone, And Drive Additions

43. Add dual filters with serial, parallel, split, and per-layer routing.
44. Add notch, comb, formant, vowel, all-pass, and morphing multimode filters.
45. Add keytracked cutoff with slope and center note.
46. Add protected filter FM.
47. Add pre/post filter drive routing.
48. Add bass-safe high-pass utility before distortion.
49. Add drive models: tape, tube, diode, transformer, hard clip, soft clip, foldback, wavefold, fuzz, and sampler-rate degrade.
50. Add gain compensation for every drive model.
51. Add loudness-matched drive audition.
52. Add drive oversampling choices.
53. Add nonlinear filter oversampling choices.
54. Add transient-preserving drive for plucks and stabs.
55. Add low-end monitor for sub mono, DC, buildup, and guard rate.
56. Add one-knob low-end tighten.
57. Add one-knob stab bite.

### 4. Modulation And Performance Additions

58. Add drag assignment from source chips to controls.
59. Add right-click "modulate with" menus.
60. Add visible modulation rings across more controls.
61. Add source-colored target badges.
62. Add hover-to-show routes for the parameter under the mouse.
63. Add selected-parameter route view.
64. Add per-route min/max range editing.
65. Add per-route curve shaping.
66. Add per-route smoothing and slew.
67. Add per-route unipolar/bipolar and invert.
68. Add per-route mute, solo, duplicate, copy, and paste.
69. Add per-route velocity scaling.
70. Add per-route keytracking scaling.
71. Add LFO 2 and LFO 3.
72. Add smooth random modulation source. First source pass implemented.
73. Add chaos/random-walk modulation source. First bounded source pass implemented.
74. Add MSEG multi-point envelope source.
75. Add step LFO with hold, slew, probability, and curve per step.
76. Add mod wheel, aftertouch, and MPE pressure/slide sources.
77. Add macro snapshots.
78. Add macro morphing between snapshots.
79. Add editable macro names per preset.
80. Add XY pad assignment editor.
81. Add XY gesture recorder.
82. Add performance-only page with macros, XY, throws, and safe output.

### 5. Sampler, Chop, And UKG Additions

83. Add drag-and-drop WAV/AIFF import onto the waveform.
84. Add missing sample and relink handling.
85. Add transient slice detection.
86. Add beat-grid slicing.
87. Add equal-region slicing.
88. Add manual slice marker creation.
89. Add per-slice start/end/nudge.
90. Add per-slice gain, pan, pitch, reverse, fade, and choke.
91. Add per-slice probability.
92. Add per-slice stutter and retrigger.
93. Add slice audition on click/hover.
94. Add keyboard mapping display.
95. Add slice-to-sequencer lane.
96. Add one-shot, gate, loop, and thru modes.
97. Add zero-cross snap.
98. Add click-safe fades.
99. Add formant-preserving pitch investigation.
100. Add short time-stretch/warp mode.
101. Add short granular sampler mode.
102. Add Akai-style 12-bit and sampler-rate color.
103. Add UKG chop templates: call/response, late swing, reverse pickup, stutter tag, sparse hook, and pitch-ramp answer.
104. Add organ bass recipe tools.
105. Add Dred/Reese bass recipe tools.
106. Add house organ/piano stab starter tools using original sources.
107. Add sample pack browser.

### 6. Sequencer, Piano Roll, And Groove Additions

108. Add multiple sequencer patterns per preset.
109. Add pattern scenes: A, B, fill, drop, breakdown, transition.
110. Add pattern chaining.
111. Add per-step ratchet/retrigger.
112. Add per-step slide/glide.
113. Add per-step accent tied to amp, filter, and drive.
114. Add per-step assignable modulation lane.
115. Add per-step sample slice lane.
116. Add per-step pump lane.
117. Add per-step delay throw lane.
118. Add per-step reverb throw lane.
119. Add step conditions: first, not first, fill, every 2, every 4, random.
120. Add scale quantize and key lock in the piano roll.
121. Add chord paint tools for minor 7, minor 9, major 7, sus, garage organ, and dub chords.
122. Add voicing spread and inversion tools.
123. Add humanize for timing, velocity, gate, and pitch.
124. Add groove library: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, broken percussion.
125. Add groove amount per lane.
126. Add swing microtiming visualization.
127. Add Euclidean generator.
128. Add bassline contour generator.
129. Add recombine tool: keep rhythm, change notes.
130. Add MIDI drag export into Ableton.
131. Add MIDI drag import from files or clips.
132. Add pattern undo history separate from global undo.

### 7. FX, Routing, And Club Processing Additions

133. Add multiband distortion with movable crossovers.
134. Add per-band drive, tone, dynamics, feedback, mix, mute, and solo.
135. Add mid/side drive, EQ, and width processing.
136. Add nonlinear FX oversampling.
137. Add compressor module.
138. Add transient shaper.
139. Add noise gate/expander.
140. Add frequency shifter.
141. Add tuned resonator bank.
142. Add delay diffusion.
143. Add dub delay feedback tone controls.
144. Add tape delay color.
145. Add reverse delay.
146. Add delay ducking.
147. Add granular FX for fills and vocal texture.
148. Add time/stutter shaper with beat-synced curves.
149. Add volume shaper with garage, house, offbeat, and straight pump presets.
150. Add filter shaper.
151. Add noise shaper.
152. Add DJ filter macro.
153. Add user-saveable per-FX slot presets.
154. Add whole-rack presets.
155. Add parallel wet/dry routing.
156. Add delay/reverb send bus.
157. Add source/sample pre/post FX routing choices.

### 8. Randomization And Sound Discovery Additions

158. Add lock buttons on every parameter group.
159. Add per-scope intensity controls.
160. Add useful-only guardrails for pitch, resonance, feedback, wet FX, sub level, output gain, and stereo bass.
161. Add user randomizer recipe builder.
162. Add randomization history.
163. Add A/B compare for generated patches.
164. Add transforms: darker, brighter, tighter, wider, dirtier, cleaner, bouncier, more minimal, more garage, more warehouse.
165. Add automatic preset names from detected traits.
166. Add random macro assignment generation.
167. Add random sequence generation with genre, key, and scale awareness.
168. Add random slice generation that avoids silence and tiny unusable cuts.
169. Add evolve/mutate that changes 10-20% while keeping identity.
170. Add breed/morph between preset A and preset B.
171. Add favorites-aware generation based on rated presets.
172. Add safety rollback when randomization creates silence or unsafe output.

### 9. Library, Presets, And Packs Additions

173. Add full-browser mode.
174. Add folder tree for user and factory libraries.
175. Add smart folders: Favorites, Five Star, Recent, UKG Bass, UKG Vocal Chops, House Stabs, Minimal Plucks, Dirty Techno, Macro Rich, User, Factory.
176. Add custom tag editor.
177. Add multi-select metadata editing.
178. Add preset notes.
179. Add pack manager import/export.
180. Add duplicate preset detection.
181. Add preset compare before overwrite.
182. Add preset version history.
183. Add missing sample and wavetable warnings in browser rows.
184. Add per-preset audio preview rendering.
185. Add audition auto-advance.
186. Add preview MIDI patterns by category.
187. Add find-similar presets using metadata and parameter distance.
188. Add dependency tracking for samples and wavetables.
189. Add pack artwork or color markers only if it stays uncluttered.
190. Add search tokens for macro assignments, engine type, FX type, sequence type, and sample type.
191. Add construction-kit browser category.
192. Add preset-pack authoring guide.

### 10. Factory Content Additions

193. Add UKG Essentials pack.
194. Add Speed Garage pack.
195. Add Deep House pack.
196. Add Tech House pack.
197. Add Minimal pack.
198. Add Warehouse Techno pack.
199. Add Dub Techno pack.
200. Add Organ House pack.
201. Add construction kits with sequence, macros, key, BPM, and recommended FX.
202. Add genre init buttons on HOME or LAB.
203. Add factory macro names and performance descriptions.
204. Add factory preset loudness validation.
205. Add factory mono-bass validation.
206. Add factory stuck-note validation.
207. Add factory preset safety metadata.

### 11. Quality, Testing, And Release Additions

208. Add denormal protection audit.
209. Add all-parameter smoothing audit.
210. Add automation zipper-noise sweep tests.
211. Add oscillator aliasing render tests.
212. Add filter resonance stability tests.
213. Add drive oversampling tests.
214. Add voice stealing audit.
215. Add stuck-note stress tests.
216. Add sustain-pedal tests.
217. Add host transport start/stop/jump tests.
218. Add save/reopen preset state tests.
219. Add Ableton checklist: scan, load, automate, save set, reopen, duplicate track, freeze, flatten, export, reopen.
220. Add UI open/close stress test while audio runs.
221. Add randomizer loudness validation.
222. Add sample import failure tests.
223. Add file-permission handling for user library saves.
224. Add crash-safe preset writes via temp file and atomic replace.
225. Add CMake CI and non-GUI checks.
226. Add signed/notarized macOS release workflow.
227. Add installer or reliable copy script for VST3 installation.
228. Add AU target after VST3 remains stable.
229. Add macOS Intel and Apple Silicon release checks.
230. Add plugin metadata audit.
231. Add user manual pages.
232. Add release checklist for Ableton Live versions.

## Best Next Build Order

1. Componentize `PluginEditor` and add layout-overlap checks.
2. Add selected-control inspector, stronger group focus, and UI scale modes.
3. Add drag/menu modulation assignment, selected-route view, and route min/max editing.
4. Add deeper random-walk controls, LFO 2/3, MSEG, and route movement processors after the first Chaos pass.
5. Add multiple sequencer patterns, ratchets, per-step modulation, and chord paint.
6. Add transient/manual slicing with per-slice controls.
7. Add UKG recipe tools for organ bass, Dred/Reese, late stabs, and vocal chop motifs.
8. Add wavetable playback, preview, and factory tables.
9. Add multiband drive with oversampling, gain compensation, and loudness tests.
10. Add full-browser mode, smart folders, tag editing, audio previews, and construction kits.
11. Add randomizer guardrails, transforms, history, and favorites-aware generation.
12. Add Ableton save/reopen/freeze/flatten release validation.

## Issue Mapping

- UI extraction and layout checks: #61
- Control feel: #29
- Drag modulation and selected-route inspection: #30, #40, #56
- Wavetable and source engines: #43
- UKG organ and bass engines: #33, #41
- UKG vocal slicing: #34, #55
- Sequencer lanes and pattern tools: #50
- Club FX and shapers: #51, #60
- FX presets and genre init buttons: #54
- Randomization and variation: #49
- Browser, tags, packs, and construction kits: #42
- Quality and oversampling: #58
- Umbrella backlog: #62
