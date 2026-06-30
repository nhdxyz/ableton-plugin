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
- Serato Sample: https://serato.com/sample
- Native Instruments Kontakt browser/preset workflow: https://www.native-instruments.com/ni-tech-manuals/kontakt-manual/en/browser-and-presets
- XLN Audio XO: https://www.xlnaudio.com/products/xo
- Algonaut Atlas: https://algonaut.audio/

UK garage and club-production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Nate VST Baseline

Nate VST already has a useful skeleton:

- JUCE/CMake/VST3 project that builds on macOS and installs into the user VST3 folder.
- House/UKG/tech-house/minimal/techno product direction.
- Subtractive synth layer with oscillator mix, sub, noise, filter character/slope, drive, envelopes, unison, Osc Warp, and output guard.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, and INFO panels.
- Performance macros, XY macro pad, randomization, A/B snapshots, and low-end guidance.
- LFO curve editor, pump curve editor, modulation matrix, macro assignment editing, route bypass/delete, destination badges, and S&H/Smooth/Chaos movement.
- Sample waveform area, chop window, phrase markers, visual selected/custom slice overlays, expanded chop focus overlay, slice pads, Slice Keys, per-slice region/pitch/gain/pan/probability/reverse/stutter/choke memory, pitch ramp, and UKG chop helpers.
- Piano-roll style 16-step sequencer with scale/chord helpers, velocity/probability/timing/length/lock/ratchet/condition/slide lanes, groove templates, host sync, undo, rotate, variation, and MIDI export with groove/ratchet/condition/slide timing.
- Addable/reorderable FX rack with delay, reverb, drive, pump, tone/EQ, width, guard, modulation FX, throws, and module presets.
- Preset library with recursive folders, categories, favorites, 1-5 ratings, metadata, search, sort, filters, macro previews, compact browser rows, and audition.
- Factory starter presets for UKG, house, tech-house, techno, and minimal.
- CTest now includes GUI layout coverage plus non-GUI library/render smoke coverage for the factory pack. CI, a repo-backed pluginval gate, and a repeatable Ableton release checklist are still missing.

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

- Sequencer has useful note lanes plus a first assignable per-step Lock lane for safe cutoff, drive, warp, pump, delay, reverb, and wavetable-position movement; deeper multi-lane and sample-accurate modulation remains open.
- Pattern scenes exist as A/B/Fill/Drop, first-pass live scene-chain step/control playback is implemented, Build 4-Bar Chain can generate A/B/Fill/Drop phrase scenes, and the expanded SEQ focus can force Auto, 2-bar, or 4-bar chain length; deeper pattern chaining with arranger controls, deeper per-scene automation/rate behavior, repeat counts, and follow behavior remains open.
- No vector/motion recorder for XY gestures.
- No motion lanes for wavetable/sample/filter/FX properties.
- Randomization does not yet create lane-based motion or "more garage/more warehouse" transforms.

### Ableton Pattern

Reference pattern:

- Ableton workflows reward drag-and-drop, automation-safe names, rack-style macro thinking, MIDI clip exchange, and fast save/reopen reliability.
- Ableton's own sampling devices reinforce that sample loading should be immediate, visible, and drag/drop friendly.

Nate VST gaps:

- First-pass current-pattern and scene-chain `.mid` drag export is direct from the SEQ UI; richer Ableton clip handoff still needs host validation.
- MIDI drag import is missing.
- WAV/AIFF audio drag/drop into SAMPLE is implemented; preserve-settings sample replacement, relink, and audio drag-out are still missing.
- Ableton save/reopen/freeze/flatten checklist should become a repeatable release test.
- Automation naming and parameter migration need an audit before adding many engines.
- AU support, signing, notarization, installer/copy script, and release notes need a release workflow.

### Serato Sample Pattern

Reference pattern:

- Fast sampling tools prioritize tempo/key detection, playable cue pads, quick chopping, pitch/time controls, and immediate MIDI triggering.
- Newer sampling workflows increasingly include stem-style isolation and confidence-aware metadata, even if Nate VST should start with simpler legal/user-provided sample workflows.

Nate VST gaps:

- WAV/AIFF drag/drop audio loading onto the waveform is implemented.
- No sample key/BPM detection or confidence display.
- First fast cue-style chop map is implemented as waveform slice-state overlays with click selection/audition; richer color labels, pad roles, hover audition, and larger library-scale maps remain open.
- No preserve-settings sample replacement for trying different vocals or stabs inside the same chop pattern.
- No audio drag-out/render-to-audio workflow for moving a generated chop back into Ableton.

### Kontakt, XO, And Atlas Browser Pattern

Reference pattern:

- Kontakt shows the value of tag filters and structured browsing for huge libraries.
- XO and Atlas show that visual sample discovery, similarity search, sample maps, quick audition, and context-aware replacement can become creative composition tools.

Nate VST gaps:

- Library metadata is stronger for presets than samples.
- No sample-folder indexing, descriptors, duplicate detection, or similarity search.
- No sample map or visual "nearby sounds" workflow.
- Library preset audition now has a first runtime role-aware house/UKG phrase pass; sample audition-in-context and editable preview phrases remain open.
- No sample dependency report that can explain which presets, construction kits, and chop maps need a missing file.

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

- Sample slicing has first Slice Keys, visual eight-pad slice overlays, expanded chop focus editing, and per-slice region/pitch/gain/pan/probability/reverse/stutter/choke/nudge/fade memory; it still needs transient/manual markers, per-slice playback modes, full relink, and slice lanes.
- First procedural Organ source mode is implemented; deeper drawbar controls, source snippets, and sampled organ content remain open.
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
18. Add global panic/all-notes-off. First bottom-keyboard panic pass implemented for keyboard, chord memory, synth MIDI, and sampler voices.
19. Add Guard activity telemetry in the UI.
20. Extend the implemented HOME signal-flow mini-map across source/sample split, sends, parallel routing, and future layer paths.
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
34. Add additive/drawbar organ source for UKG and speed garage. First procedural Organ waveform pass is implemented as an appended source mode; editable drawbars and source snippets remain open.
35. Add legal/original sampled-PCM starter sources for house organ and piano-style stabs. First procedural House Piano waveform pass is implemented; bundled sampled/PCM content remains open.
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
55. Extend the current HOME low-end assistant/analyzer with DC checks, long-window buildup history, phase-cancellation history, and guard-hit rate.
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
71. Add LFO 2 and LFO 3. First LFO 2 source pass implemented; LFO 3 remains open.
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
84. Expand missing sample handling into full relink/dependency workflow beyond the first stale-audio clear and missing-path UI status guard.
85. Add transient slice detection.
86. Add beat-grid slicing.
87. Add equal-region slicing.
88. Add manual slice marker creation.
89. Add per-slice start/end/nudge. First stored start/end region memory and stepped nudge controls are implemented; manual marker nudge remains open.
90. Add per-slice gain, pan, pitch, reverse, fade, and choke. First gain/pan/pitch/reverse/stutter/choke/fade memory pass implemented; deeper per-slice envelope editing remains open.
91. Add per-slice probability. First trigger-probability pass implemented with the SAMPLE `Ghost` action.
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
106. Add house organ/piano stab starter tools using original sources. First procedural Organ/House Piano source modes are implemented and seeded into selected factory presets; larger starter tools and packs remain open.
107. Add sample pack browser.

### 6. Sequencer, Piano Roll, And Groove Additions

108. Add multiple sequencer patterns per preset.
109. Add pattern scenes: A, B, fill, drop, breakdown, transition.
110. Add deeper pattern chaining beyond the first live scene-chain playback, Build 4-Bar Chain, and forced chain-length passes.
111. Add per-step ratchet/retrigger. First saved lane/playback/export pass implemented; repeat curves, per-ratchet probability, and deeper velocity falloff remain open.
112. Add deeper per-step slide/glide. First saved lane/live-export overlap pass implemented.
113. Add per-step accent tied to amp, filter, and drive.
114. Add per-step assignable modulation lane. First single-destination Lock lane implemented; multiple simultaneous lanes, per-step destination changes, and sample-accurate scheduling remain open.
115. Add per-step sample slice lane.
116. Add per-step pump lane.
117. Add per-step delay throw lane.
118. Add per-step reverb throw lane.
119. Add deeper step conditions: first, not first, every 4, random, follow, and per-repeat probability. First Always/Odd/Even/Fill condition lane is implemented.
120. Add scale quantize and key lock in the piano roll.
121. Add deeper chord paint tools for minor 7, minor 9, major 7, sus, garage organ, and dub chords beyond the first Chord Stab Paint transform.
122. Add voicing spread and inversion tools.
123. Add humanize for timing, velocity, gate, and pitch.
124. Add groove library: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, broken percussion.
125. Add groove amount per lane.
126. Add swing microtiming visualization.
127. Add Euclidean generator.
128. Add bassline contour generator. First Bass Contour Shape transform is implemented; deeper multi-phrase generation remains open.
129. Add recombine tool: keep rhythm, change notes.
130. Add richer MIDI drag/export validation in Ableton. First current-pattern and scene-chain `.mid` drag-out is implemented.
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
175. Expand the implemented smart-crate filter pass, centered browser actions, and folder/pack/style crate rail into full smart folders with editable user crates, New This Week, UKG Vocal Chops, House Stabs, Dirty Techno, Macro Rich, User, and Factory views.
176. Add custom tag editor.
177. Add multi-select metadata editing.
178. Expand implemented preset notes into structured production tips and batch-editable metadata.
179. Add pack manager import/export.
180. Expand duplicate preset handling beyond same-folder overwrite detection with rename-as-copy, version history, and near-duplicate detection.
181. Extend the implemented preset-load compare/revert, Save Target preview, and two-click overwrite guard into full edited-vs-saved safe-overwrite compare.
182. Add preset version history.
183. Add missing sample and wavetable warnings in browser rows.
184. Expand the implemented rendered per-preset preview cache/playback into background/batch generation, waveform/level row badges, manual regeneration, and preview loudness consistency.
185. Add audition auto-advance.
186. Expand the implemented role-aware audition phrases into editable preview MIDI patterns by category.
187. Add find-similar presets using metadata and parameter distance.
188. Add dependency tracking for samples and wavetables.
189. Add pack artwork or color markers only if it stays uncluttered.
190. Add search tokens for macro assignments, engine type, FX type, sequence type, and sample type.
191. Add construction-kit browser category.
192. Add preset-pack authoring guide.

### 10. Factory Content Additions

193. Expand the current UKG Essentials/Basslines presets into a larger pack.
194. Expand the first implemented speed-garage factory preset into a fuller Speed Garage pack.
195. Expand the current Deep House/House Chords presets into a larger pack.
196. Expand the current Tech House Tools presets into a larger pack.
197. Expand the current Minimal Tools presets into a larger pack.
198. Expand the current Warehouse Techno presets into a larger pack.
199. Expand the current Dub Techno preset into a larger pack.
200. Add Organ House pack.
201. Add construction kits with sequence, macros, key, BPM, and recommended FX.
202. Add genre init buttons on HOME or LAB.
203. Add factory macro names and performance descriptions.
204. Expand factory preset loudness validation beyond the first `FactoryPresetRenderAudit` finite/non-silent/peak smoke gate into role targets and pack consistency.
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

## 2026-06-28 Deeper Research Expansion

This pass extends the first 232-item list after checking more current product pages and adjacent production tools. It is additive: keep the earlier priorities, but treat this section as the next source of feature tickets once the current UI, modulation, sampler, sequencer, and browser foundations are stable.

Additional references reviewed:

- Vital: https://vital.audio/
- Korg opsix native: https://www.korg.com/us/products/software/opsix_native/
- XLN Audio XO: https://www.xlnaudio.com/products/xo
- Sononym: https://sononym.net/
- Ableton Live 12 MIDI Tools: https://www.ableton.com/en/live-manual/12/midi-tools/
- Ableton Live 12 file/browser workflows: https://www.ableton.com/en/live-manual/12/managing-files-and-sets/

Key extra lessons:

- Serum 2 and Pigments are no longer just "wavetable synth" references. The bigger pattern is hybrid source engines: wavetable, sample, granular, spectral, analog, harmonic/modal, and sequencing living in one instrument.
- Phase Plant, modwave, opsix, and ZENOLOGY show that deeper engines need readable structure: generator lanes, partials, operator mixers, motion lanes, or source racks. Nate VST should not keep adding everything to one fixed subtractive page.
- Vital, Saturn 2, and modwave show that visual feedback matters as much as the feature itself: animated waveforms, filter responses, live modulation visualization, source movement, and per-band displays make dense sound design understandable.
- ShaperBox and Infiltrator show that club tools win through rhythmic control: drawable curves, step activation, swing, multiband routing, quick preset macros, random patterns, and tempo-locked performance movement.
- XO and Sononym point to the next browser leap: similarity, audio-derived descriptors, duplicate detection, fast audition, recently used sources, sample maps, and "find similar" discovery.
- Ableton Live 12 raises the workflow bar around scale-aware transformations, generative MIDI tools, MPE lanes, clip export/import, browser tags, and drag-to-save musical ideas.
- UK garage remains a product filter, not just a preset category. Features should bias toward shuffled 2-step timing, late stabs, organ and Reese/Dred bass, vocal chops, sub control, call/response hooks, and fast clip export to Ableton.

## Additional Long Add List

### 12. Hybrid Source Engine And Layer Architecture

233. Add a source rack with slots for Synth, Wavetable, Sample, Granular, FM, Organ, Noise, and Resample sources.
234. Add source-slot enable, mute, solo, level, pan, width, tune, transpose, and phase controls.
235. Add source-slot reorder once automation and preset migration are designed.
236. Add layer A/B architecture with independent source, filter, amp, envelope, modulation receive, and FX-send controls.
237. Add a Layer Blend macro for quick house chord/body balancing.
238. Add a Layer Spread macro that widens only safe upper harmonics while leaving mono sub centered.
239. Add layer copy, paste, duplicate, reset, and randomize commands.
240. Add layer freeze/resample to capture CPU-heavy hybrid patches as a sample layer.
241. Add a source browser by role: Sub, Rubber Bass, Organ, Chord, Pluck, Stab, Bell, Vocal, Noise, Texture.
242. Add source snippets so users can save only the oscillator/source setup separate from the whole preset.
243. Add legal factory PCM snippets for original house organ, garage organ, rave stab, piano stab, string stab, and noise hits.
244. Add a "club mono source" mode that forces pitch/sub fundamentals through a mono-safe path.
245. Add source dependency tracking for samples, future wavetables, and resampled layers.
246. Add missing-source warnings directly in the source rack and preset browser.
247. Add per-source quality mode so granular/wavetable/FM sources can scale CPU.
248. Add per-source randomization locks so useful core identity survives mutation.
249. Add source-level undo snapshots for fast compare after deep edits.
250. Add source output meters before the shared filter/FX chain.

### 13. Wavetable, Spectral, And Visual Oscillator Depth

251. Add a true wavetable oscillator with stable phase, interpolation, anti-aliasing, and position modulation.
252. Add a wavetable display with current frame, table index, playhead, and modulation overlay.
253. Add a small waveform editor for normalizing, smoothing, phase rotating, and trimming imported tables.
254. Add safe factory tables focused on house bass, UKG organ/bell, dub stab, garage vocal-ish formants, techno noise, and minimal plucks.
255. Add wavetable import from WAV tables after the playback engine is stable.
256. Add table folders, favorites, tags, and recently used wavetables.
257. Add warp modes: bend, mirror, fold, phase skew, hard sync style, harmonic tilt, odd/even emphasis, quantize, and formant tilt.
258. Add dual warp slots with independent modulation amounts.
259. Add oscillator FM and phase distortion depth controls with safety limiting.
260. Add ring modulation and amplitude modulation between source slots.
261. Add a spectral tilt stage for bright, clean modern plucks without overusing EQ.
262. Add transient-safe spectral smear only for texture sources, not bass.
263. Add harmonic mask presets for hollow garage organ, nasal bass, dark dub stab, and metallic techno hit.
264. Add an oscilloscope/spectrogram switch in SYNTH so waveform changes are visible while tuning.
265. Add oscillator keytracking and note-dependent warp for basses that open up higher on the keyboard.
266. Add retrigger/free-run phase modes per oscillator and per layer.
267. Add per-voice drift for pitch, phase, filter cutoff, envelope time, and stereo placement.
268. Add oscillator aliasing tests for high notes, unison, FM, sync, and warp modes.

### 14. FM, Organ, Partial, And Classic Club Sources

269. Add a three-operator quick FM engine for UKG bells, pings, metallic stabs, and short bass knocks.
270. Add a six-operator advanced FM mode only after the quick FM path is musical.
271. Add an operator mixer view inspired by clear per-operator level/route feedback, not by copying a specific UI.
272. Add operator output scopes so users can see which operator is moving.
273. Add FM algorithms for organ, bell, hollow bass, metallic stab, and techno hit.
274. Add FM randomization by algorithm, operator ratio, envelope shape, and safe output.
275. Add a drawbar-style organ source with click, leakage, foldback, percussion, key click, and vibrato/chorus.
276. Add garage organ recipes: clean organ stab, dark organ bass, speed-garage organ lead, and clipped organ chord.
277. Add a partial/additive bank for bell, glass, chord shimmer, and synthetic vocal tones.
278. Add partial-level randomization with harmonic locks.
279. Add "M1-style" and "classic workstation-style" recipe workflows using original/legal source material only.
280. Add a Reese/Dred source recipe that layers detuned mid harmonics over a protected centered sub.
281. Add bass phase-scope warnings when a Reese layer weakens the mono low end.
282. Add FM/organ/source presets as first-class browser items.

### 15. Sampler, Resampling, And Sample Discovery

283. Add drag audio from Finder or Ableton-exported files directly onto SAMPLE.
284. Add a sample-file browser with waveform thumbnails and quick audition.
285. Add sample similarity clustering for user one-shots after a lightweight descriptor pipeline exists.
286. Add audio descriptors: brightness, transient strength, length, loop/one-shot, pitch confidence, tempo confidence, noisiness, and low-end weight.
287. Add "find similar sample" from the current chop or one-shot.
288. Add duplicate and near-duplicate sample detection for the user sample folder.
289. Add sample pack indexing with folder include/exclude rules.
290. Add "recently used samples" and "missing in this set" browser filters.
291. Add one-shot maps for kick, clap, snare, hat, perc, vocal, stab, bass hit, and noise categories.
292. Add a sample-map view for large chop/one-shot libraries without turning Nate VST into a drum machine first.
293. Add transient slicing with sensitivity, minimum slice length, and silence rejection.
294. Add beat-grid, equal-region, manual, and detected-transient slice modes.
295. Add per-slice pitch, formant, gain, pan, fade in/out, reverse, choke, probability, stutter, and FX send.
296. Add per-slice color/tag labels: Ghost, Main, Pickup, Response, Reverse, Fill, Dirty, Clean.
297. Add zero-cross snap and automatic micro-fades for every slice.
298. Add vocal chop templates: late pickup, call/response, two-note hook, reverse tag, stutter fill, and sparse ghost chop.
299. Add sample start modulation per sequencer step for vocal chop rolls.
300. Add resample current synth, current sequence, or current macro gesture into the sampler.
301. Add render-to-audio export for Ableton drag-out.
302. Add time-stretch modes: one-shot, repitch, beats, texture/grain, and future formant-safe vocal.
303. Add BPM/key detection only if confidence can be shown and low-confidence values do not mislead the user.
304. Add sample dependency packing for construction kits.

### 16. Sequencer, MIDI Tools, And Groove Generation

305. Add multiple pattern slots per preset: A, B, Fill, Break, Drop, Alt, and Init.
306. Add deeper pattern chaining with arranger-style scene editing, repeat count, follow probability, and deeper per-scene automation/rate behavior beyond the first live step/control-chain and forced-length passes.
307. Add pattern clip export by dragging MIDI from the SEQ panel to Ableton.
308. Add MIDI import by dropping a MIDI file into the sequencer.
309. Add scale-aware note generation like Seed/Rhythm-style tools, biased toward club patterns.
310. Add transform tools: chop, connect, recombine, span/gate, strum, time warp, quantize, velocity shape, and rotate.
311. Add UKG transform presets: late snare ghost, shuffled hats, short organ stab, swung bass pickup, and reverse vocal lead-in.
312. Add house transform presets: offbeat chord, rolling bass, filtered stab, and sidechain-aware gate.
313. Add techno/minimal transform presets: pulse, skip, sparse pluck, delayed hit, and machine repeat.
314. Add Euclidean rhythm generation for plucks, percussive synth hits, and FX triggers.
315. Add density, accent, split, shift, and probability controls for generated patterns.
316. Expand per-step ratchet/retrigger beyond the first repeat-count lane with repeat curves, per-ratchet probability, and deeper velocity falloff.
317. Add deeper per-step slide/glide for basslines beyond the first overlap pass.
318. Add per-step chord type and inversion locks.
319. Add per-step modulation lanes assignable to cutoff, drive, warp, sample start, pump, delay throw, reverb throw, width, and pitch.
320. Add per-step FX activation lanes for Infiltrator-style rhythmic throws inside Nate VST.
321. Add per-lane swing amounts rather than only global swing.
322. Add MPE lanes for pressure, slide, pitch bend, and per-note expression later.
323. Add a groove library with named templates and editable microtiming offsets.
324. Add pattern mutation history with before/after audition.
325. Add "keep rhythm/change notes" and "keep notes/change rhythm" recombine tools.
326. Add pattern safety checks to avoid stuck notes, all-silence patterns, and unusably dense MIDI exports.

### 17. Modulation, MSEG, And Movement Visualization

327. Add MSEG/function generators with breakpoint curves, loop regions, one-shot mode, and sync/free timing.
328. Add step-LFO mode with per-step value, curve, glide, probability, skip, and randomize.
329. Add multiple LFOs with names, colors, and clear target summaries.
330. Add LFO-to-LFO and modulation-processor routing only after the UI can explain it.
331. Add modulation processors: scale, offset, invert, rectify, smooth, lag, clamp, quantize, sample/hold, slew, and curve remap.
332. Add per-route min/max range editing on destination controls.
333. Add per-route unipolar/bipolar, additive/multiply, and pre/post behavior labels.
334. Add visible modulation rings around important knobs.
335. Add hover route overlays: source, destination, amount, range, and active value.
336. Add source activity meters for macros, envelopes, LFOs, random sources, sequencer lanes, and future audio follower.
337. Add drag-to-modulate from source chips to controls.
338. Add right-click "modulate with" menus on automatable controls.
339. Add a modulation assignment browser sorted by source, destination, page, amount, and active status.
340. Add macro scenes: A, B, C, D, Drop, Break, Fill, and Clean.
341. Add XY gesture recording into motion lanes.
342. Add morph pad between snapshots with per-parameter locks.
343. Add envelope follower from sample/synth output for movement-driven effects.
344. Add note source modulation: keytrack, note random, note age, legato state, and voice index.

### 18. FX, Routing, And Club Performance Processing

345. Add multiband distortion with at least low/mid/high bands, crossover display, per-band drive, tone, mix, and gain compensation.
346. Add HQ oversampling modes for nonlinear FX, with CPU and latency notes.
347. Add dynamic drive that responds to envelope follower or sidechain input.
348. Add clipper/limiter with true output ceiling and oversampled peak protection.
349. Add transient shaper with attack, sustain, tone, and mix.
350. Add compressor with sidechain input support after host sidechain routing is designed.
351. Add sidechain ducking from actual Ableton sidechain input, not only internal pump.
352. Add volume shaper with drawable curve, phase display, and MIDI/host retrigger.
353. Add filter shaper with drawable curves, multiple filter types, resonance safety, and per-band option.
354. Add time shaper for reverse, tape stop, half-time, stutter, and glitch fills.
355. Add pitch shaper for short risers, garage tape-ups, and octave throw effects.
356. Add noise shaper for vinyl, air, hiss, rumble, crowd/noise swells, and gated texture.
357. Add width shaper that animates stereo image while preserving mono bass.
358. Add pan shaper for syncopated stereo percussion and stab movement.
359. Add reverb shaper with rhythmic gated tails and ducking.
360. Add rhythmic FX sequencer where each enabled effect can be turned on/off per step.
361. Add FX macro lane controls for Amount, Tone, Motion, Space, Width, and Throw.
362. Add per-effect presets and whole-rack presets.
363. Add serial, parallel, send, and split-band rack routing.
364. Add granular FX for vocal fills, shimmer fragments, and transition texture.
365. Add DJ-style loop/beat-repeat throw with timed release and restore.
366. Add performance latch/momentary modes for throws, with clear Ableton automation behavior.

### 19. Browser, Presets, Packs, And Discovery

367. Add full-screen or wide browser mode with tree, results, details, macro preview, and audio preview.
368. Add rendered preset audio preview files using standard house/UKG/techno MIDI phrases; runtime role-aware audition phrases are implemented.
369. Add auto-preview with output safety and host-transport awareness.
370. Expand the implemented smart-crate filter pass, centered browser actions, and folder/pack/style crate rail into smart collections for New This Week, editable user crates, UKG Vocal Chops, House Stabs, Dirty Techno, Clean Subs, and pack folders.
371. Add editable user tags with batch metadata editing.
372. Extend the existing search tokens with modulation count, FX type, sequencer type, sample dependencies, and date.
373. Add "find similar preset" using parameter distance, macro roles, tags, and future audio preview descriptors.
374. Add "more like this" randomization seeded from favorites or high-rated presets.
375. Extend the implemented Library preset-load compare/revert into edited-vs-saved and generated-slot compare states.
376. Add preset version history for user saves.
377. Expand the implemented two-click overwrite guard with rename-as-copy, version history, duplicate detection, and visual compare.
378. Add pack manager with import/export, missing dependency report, and pack validation.
379. Add construction-kit preset type that bundles patch, sequence, sample references, MIDI phrase, macro intent, BPM, key, and suggested Ableton use.
380. Add subfolder templates for UKG/Bass, UKG/Vocal Chops, UKG/Organ, House/Chords, Tech House/Bass, Techno/Stabs, Minimal/Plucks.
381. Expand preset notes with richer production tips fields for factory content and batch editing.
382. Add "used in current project" or "recent in this session" collection if host-safe state tracking is practical.

### 20. House And UKG Content Direction

383. Expand UKG factory presets into a construction-kit pack with bass, organ, vocal, stab, and groove variants.
384. Expand the first implemented Speed Garage Organ Bass into a fuller pack with clipped bass, shuffled drums, late stabs, and siren/noise throws.
385. Expand Deep House/House Chords presets with more warm chord stabs, plucks, sub chugs, pads, and mellow macro maps.
386. Expand Tech House Tools presets with more rubber bass, perc plucks, short stabs, tight pump, and low CPU presets.
387. Expand Minimal Tools presets with more clicks, short resonant plucks, microtimed bass pulses, and sparse FX.
388. Expand Warehouse Techno presets with more stabs, drones, noise hits, acid-ish pulses, and guarded distortion.
389. Expand Dub Techno presets with more chord echoes, filtered noise, slow modulation, and space macros.
390. Add Organ House pack with original organ source material, chord memory, and drawbar macro assignments.
391. Add vocal chop starter kits with legal/original vocal snippets or user-drop placeholders.
392. Add genre QA passes: every factory preset should have macro names, macro ranges, output safety, mono-bass check, and Ableton audition notes.

## Best Next Build Order

1. Componentize `PluginEditor` and add layout-overlap checks.
2. Add selected-control inspector, stronger group focus, and UI scale modes.
3. Add drag/menu modulation assignment, selected-route view, and route min/max editing.
4. Add deeper random-walk controls, LFO 3, MSEG, and route movement processors after the first Chaos and LFO 2 passes.
5. Add deeper pattern chaining beyond the first forced-length mode, deeper per-scene automation/rate behavior, deeper slide/glide controls beyond the first overlap pass, richer condition options, MIDI import, richer per-step modulation, and deeper chord paint beyond the first Chord Stab Paint transform.
6. Add transient/manual slicing with deeper per-slice playback controls beyond the current eight-pad visual overlay and stored slice-memory/nudge/fade pass.
7. Add UKG recipe tools for organ bass, Dred/Reese, late stabs, and vocal chop motifs.
8. Add wavetable playback, preview, and factory tables.
9. Add multiband drive with oversampling, gain compensation, and loudness tests.
10. Add full-browser mode, smart folders, tag editing, audio previews, and construction kits.
11. Add randomizer guardrails, transforms, history, and favorites-aware generation.
12. Add Ableton save/reopen/freeze/flatten release validation.

## Best Next Build Order After This Expansion

1. Componentize the editor, add screenshot/layout regression checks, and introduce UI scale modes before adding more dense panels.
2. Add selected-control inspector, modulation rings, drag/right-click modulation assignment, and per-route min/max editing.
3. Add deeper pattern chaining beyond the first forced-length mode, deeper per-scene automation/rate behavior, richer per-step modulation lanes, deeper slide/glide controls beyond the first overlap pass, deeper condition vocabulary, and MIDI drag import/validation so the sequencer becomes more Ableton-useful.
4. Add transient/manual slicing, deeper per-slice playback controls beyond the first zero-cross/nudge/fade pass, and UKG vocal-chop templates.
5. Add true wavetable playback, visual table display, safe factory tables, and oscillator aliasing tests.
6. Add multiband drive, oversampling, gain compensation, clipper/limiter, and drive loudness tests.
7. Add MSEG/function generators, step LFO, route processors, and source activity meters.
8. Add source rack/layer architecture before adding granular, FM, organ, and partial engines.
9. Add full browser mode, audio previews, smart collections, tag editing, find-similar, and construction-kit metadata.
10. Add UKG/house factory content packs with macro QA, output QA, mono-bass QA, and Ableton audition notes.
11. Add real Ableton workflow validation: scan, load, automate, save/reopen, duplicate track, freeze, flatten, export, and reopen.

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
