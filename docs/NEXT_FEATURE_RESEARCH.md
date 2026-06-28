# Nate VST Next Feature Research

Research date: 2026-06-28

This is the current "what else should we add" list for Nate VST. It compares the repo against current reference products and narrows the next backlog toward the actual product identity: a fast house, UK garage, tech-house, minimal, and techno instrument for Ableton.

The goal is not to clone any product. These notes translate broad product patterns into Nate VST-specific features.

## References Reviewed

Synth and hybrid instrument references:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Ableton Meld, Wavetable, and Roar manual sections: https://www.ableton.com/en/live-manual/12/live-instrument-reference/

FX, motion, and browser workflow references:

- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- Devious Machines Infiltrator: https://deviousmachines.com/product/infiltrator/
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Output Portal: https://output.com/products/portal

UK garage and club-production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Repo Position

Nate VST already has a real foundation:

- JUCE/VST3/CMake project with Ableton-focused validation.
- Subtractive synth core with oscillator 1, oscillator 2, sub, noise, unison, filter character/slope, drive, envelopes, and output guard.
- A first bandlimited oscillator quality pass.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, and LIBRARY panels.
- Randomization recipes, section scope, locks, undo/redo, and genre seeds.
- Eight performance macros plus an XY macro pad.
- Modulation matrix, LFO curve editing, pump curve editing, macro assignment editing, route bypass/delete, and S&H/Smooth/Chaos sources.
- Sample waveform display, chop window, phrase markers, slice pads, reverse, pitch ramp, stutter, and UKG chop helpers.
- 16-step piano-roll style sequencer with chord memory, templates, groove modes, velocity/probability/timing lanes, MIDI export, and host sync.
- Addable/reorderable FX rack with delay, reverb, drive, pump, tone/EQ, width, guard, modulation FX, throws, and module presets.
- Preset library with recursive folders, categories, search, sort, favorites, 1-5 rating, metadata, macro previews, filters, and audition.

The missing work is not "add one more page of knobs." The missing work is depth, visual feedback, source quality, library maturity, and genre-specific generation.

For the latest program-by-program comparison against Serum 2, Pigments, Phase Plant, Current, ZENOLOGY Pro, u-he, Korg, Ableton, ShaperBox, Saturn, Portal, Infiltrator, and UK garage production references, see `docs/PROGRAM_RESEARCH_GAP_LIST.md`. That document is the longer research matrix; this file remains the shorter prioritized add list.

## Highest-Level Gaps

1. The synth source engine is still shallow compared with modern hybrid synths.
2. Modulation is functional, but assignment and feedback are not yet fast enough.
3. The sampler is a chop starter, not yet a serious UKG vocal and phrase instrument.
4. The sequencer is useful, but not yet a full groove/pattern tool.
5. The FX rack is broad, but still needs pro-level drive, shaper, and routing depth.
6. Randomization needs stronger "useful musical result" guardrails.
7. The preset browser needs smart folders, audio previews, and pack workflows.
8. The UI needs component extraction, layout testing, and stronger focus views before adding large engines.
9. Audio quality needs oversampling, smoothing, gain-staging, and render tests.
10. Ableton release workflow needs save/reopen/freeze/flatten testing and macOS signing/notarization.

## Long Add List

### A. UI, Layout, And Interaction

1. Split `PluginEditor` into one component per page: HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, top bar, and keyboard.
2. Add an automated component-overlap test for every panel at the current plugin size. First editor bounds audit implemented through CTest; component extraction and richer overlap rules remain open.
3. Add screenshot regression captures for every panel after each significant UI change.
4. Add a debug bounds overlay toggle for manual layout reviews.
5. Add UI scale choices: 85%, 100%, 125%, and 150%.
6. Add a compact Ableton-laptop layout that shows fewer controls per page.
7. Add a larger sound-design layout once component extraction is complete.
8. Replace text-heavy buttons with icon buttons where the action is standard: save, reload, favorite, audition, undo, redo, copy, clear, move up, move down.
9. Add a global selected-control inspector that shows value, automation name, modulation routes, and reset action.
10. Add value popovers for every draggable control.
11. Add consistent double-click reset behavior across all sliders, buttons, and curve points.
12. Add per-control drag sensitivity classes: coarse, normal, fine, and bipolar.
13. Add larger invisible hit areas around small rotary controls.
14. Add page-level grouping around user tasks: Source, Shape, Motion, Space, Utility.
15. Add collapsible groups for dense pages instead of always showing every section.
16. Add a professional HOME redesign focused on preset, source, macros, randomize, and output safety.
17. Add a signal-flow strip showing synth, sample, filter, FX, and output order.
18. Add voice count, CPU estimate, and guard activity in the top bar.
19. Add stronger disabled-state styling for inactive modules and mode-dependent controls.
20. Add color-coded domain language: source, filter, modulation, sequence, effects, library.

### B. Source Engines

21. Add a true wavetable oscillator with position modulation.
22. Add a wavetable preview display with current frame and animated position.
23. Add factory wavetables for UKG organ/bell, house chord, tech-house rubber bass, dub stab, and techno noise.
24. Add safe WAV wavetable import after factory playback is stable.
25. Add wavetable interpolation quality modes.
26. Add wavetable warp modes: bend, fold, mirror, sync-style, phase skew, spectral tilt, and formant-like tilt.
27. Add oscillator hard sync.
28. Add PWM for square/pulse sounds.
29. Add ring modulation between oscillators.
30. Add protected FM/PM between oscillators.
31. Add a focused 3-operator FM engine for UKG bells, pings, metallic stabs, and techno hits.
32. Add an additive/drawbar-style organ source for speed garage and UKG bass.
33. Add legal/original sampled-PCM starter sources for house piano/organ-style stabs.
34. Add noise source colors: white, pink, brown, metallic, vinyl, air, digital hash.
35. Add supersaw/unison improvements: detune curve, phase spread, stereo spread, and mono-safe mode.
36. Add analog drift for pitch, phase, filter, envelope, and pan.
37. Add a second synth layer with layer mute, solo, copy, blend, and routing.
38. Add per-layer filter and amp envelopes once the layer model is stable.
39. Add layer performance macros for Weight, Width, Dirt, and Motion.
40. Add source snippet presets: Sub Bass, Reese, Organ Bass, Chord Stab, Minimal Pluck, Techno Hit.

### C. Filters, Drive, And Tone

41. Add dual filters with serial, parallel, split, and per-layer routing.
42. Add more filter types: notch, formant, vowel, comb, all-pass, and morphing multimode.
43. Add modeled filter flavors inspired by clean digital, ladder, SEM-style, MS-style, and aggressive diode behavior.
44. Add filter keytracking with slope and center note.
45. Add filter FM with protected depth.
46. Add pre/post filter drive routing.
47. Add a bass-safe high-pass utility before heavy drive.
48. Add drive models: tape, tube, diode, transformer, hard clip, soft clip, foldback, wavefold, fuzz, and digital degrade.
49. Add gain compensation for every drive mode.
50. Add drive audition with loudness matching.
51. Add oversampling choices for drive and filter nonlinearity.
52. Add transient-preserving drive for plucks and stabs.
53. Add club low-end telemetry: sub mono check, DC check, low-end build-up warning, and guard hit rate.
54. Add a one-knob "low-end tighten" transform for house and UKG bass patches.
55. Add a one-knob "stab bite" transform for chord and organ stabs.

### D. Modulation And Performance

56. Add drag assignment from modulation source chips to controls.
57. Add right-click "modulate with" menus on automatable controls. First pass implemented for MOD-targetable sliders.
58. Add visible modulation rings around modulated knobs and stripes/badges on modulated horizontal controls. First horizontal slider feedback pass implemented.
59. Add source-colored modulation badges with route amount.
60. Add hover-to-show routes for the parameter under the mouse. First pass implemented through the selected-control inspector; graphical hover overlays remain open.
61. Add route min/max range editing.
62. Add per-route curve shaping.
63. Add per-route smoothing and slew.
64. Add per-route unipolar/bipolar and invert. First quick-invert amount menu pass implemented for MOD matrix amount sliders.
65. Add per-route mute, solo, duplicate, copy, and paste. First duplicate-to-next-free-slot pass implemented in the MOD matrix.
66. Add LFO 2 and LFO 3. First LFO 2 source pass implemented; LFO 3 remains open.
67. Add smooth random and chaos sources in addition to current S&H. First Smooth and Chaos source passes implemented; deeper random-walk controls remain open.
68. Add a true MSEG source with multi-point draw/edit.
69. Add step LFO with per-step hold, slew, probability, and shape.
70. Add macro snapshots and morphing between macro states.
71. Add editable macro names per preset.
72. Add XY pad assignment editor with multiple destinations per axis.
73. Add XY gesture recording for motion playback.
74. Add mod wheel, aftertouch, and MPE pressure/slide sources.
75. Add a performance-only page that hides deep edit controls and shows macros, XY, throws, and safe output.

### E. Sampler, Chops, And UKG

76. Add drag-and-drop WAV/AIFF import onto the waveform display.
77. Add missing-file and relink handling for saved presets.
78. Add transient-based slice detection.
79. Add beat-grid slicing.
80. Add equal-region slicing with user-selectable count.
81. Add fully manual slice marker creation and editing.
82. Add per-slice start, end, nudge, gain, pan, pitch, reverse, fade, and choke. First pitch/gain/reverse/stutter/choke memory and Slice Keys playback pass implemented for eight equal-region pads; marker start/end, pan, fades, and probability remain open.
83. Add per-slice probability.
84. Add per-slice stutter and retrigger.
85. Add slice audition on hover/click.
86. Add keyboard mapping display for slices.
87. Add slice-to-sequencer lane.
88. Add one-shot, gate, loop, and thru playback modes per slice.
89. Add zero-cross snap per slice marker.
90. Add click-safe fades per slice.
91. Add formant-preserving pitch shift investigation for vocal chops.
92. Add short time-stretch/warp mode for rhythmic vocal phrases.
93. Add short granular sampler mode for texture and fills.
94. Add Akai-style 12-bit and sampler-rate color.
95. Add UKG chop templates: call/response, late swing, reverse pickup, stutter tag, sparse hook, and pitch-ramp answer.
96. Add organ-bass starter recipes.
97. Add Dred/Reese starter recipes with centered sub and moving upper harmonics.
98. Add house piano/organ stab starter recipes using original/legal sources.
99. Add a sample pack browser inside LIBRARY.
100. Add construction-kit presets that store sample, sequence, macros, key, BPM, and recommended FX.

### F. Sequencer, Piano Roll, And Groove

101. Add multiple sequencer patterns per preset.
102. Add pattern scenes: A, B, fill, drop, breakdown, transition.
103. Add pattern chaining.
104. Add per-step note length. First saved lane/playback/export pass implemented.
105. Add per-step ratchet/retrigger.
106. Add per-step slide/glide.
107. Add per-step accent tied to amp, filter, and drive.
108. Add per-step modulation lanes assignable to safe destinations.
109. Add per-step sample slice lane.
110. Add per-step pump depth lane.
111. Add per-step delay throw lane.
112. Add per-step reverb throw lane.
113. Add probability ranges and step conditions such as first, not first, fill, every 2, every 4, and random.
114. Add scale quantize and key lock in the piano roll.
115. Add chord paint tools for minor 7, minor 9, major 7, sus, garage organ stabs, and dub chords.
116. Add voicing spread and inversion tools.
117. Add humanize with timing, velocity, gate, and pitch amount.
118. Add groove template library: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, and broken percussion.
119. Add groove amount per lane.
120. Add swing microtiming visualization.
121. Add Euclidean generator for minimal blips, hats, and percussion-style synth hits.
122. Add bassline contour generator for rolling tech-house and UKG movement.
123. Add recombine tool: keep rhythm, change notes.
124. Add MIDI drag export directly from the plugin UI into Ableton.
125. Add MIDI drag import from dropped MIDI files/clips.

### G. FX, Routing, And Club Processing

126. Add multiband distortion with movable crossovers.
127. Add per-band drive, dynamics, tone, feedback, mix, mute, and solo.
128. Add mid/side processing for drive, EQ, and width.
129. Add oversampling modes for nonlinear FX.
130. Add compressor with sidechain-style envelope behavior.
131. Add transient shaper for plucks, stabs, and vocal chops.
132. Add noise gate/expander for chopped samples and tight basses.
133. Add frequency shifter for minimal and techno movement.
134. Add tuned resonator bank.
135. Add convolution or small IR reverb only if CPU and licensing are clean.
136. Add delay diffusion.
137. Add dub delay tone and feedback shaping.
138. Add tape delay color.
139. Add reverse delay.
140. Add delay ducking.
141. Add granular FX for fills, transitions, and vocal texture.
142. Add time/stutter shaper with beat-synced curves.
143. Add volume shaper with garage, house, offbeat, and straight pump presets.
144. Add filter shaper for low-pass movement and drops.
145. Add noise shaper for risers, tops, and transient brightness.
146. Add DJ filter macro for breakdowns and transitions.
147. Add per-FX slot presets that users can save.
148. Add whole-rack presets.
149. Add parallel wet/dry routing for selected FX.
150. Add internal send bus for delay/reverb throws.

### H. Randomization And Sound Discovery

151. Add lock buttons directly on every parameter group.
152. Add per-scope intensity: Source, Filter, Amp, Motion, FX, Seq, Sample, Library, Macros.
153. Add "useful only" guardrails for pitch, output gain, sub level, resonance, feedback, wet FX, and stereo bass.
154. Add a user recipe builder for randomization.
155. Add randomization history beyond the current one-step undo/redo.
156. Add A/B compare for generated patches.
157. Add transforms: darker, brighter, tighter, wider, dirtier, cleaner, bouncier, more minimal, more garage, more warehouse.
158. Add automatic preset naming from detected traits.
159. Add random macro assignment generation.
160. Add random sequence generation that respects selected genre, key, and scale.
161. Add random slice generation that avoids silence and overly tiny cuts.
162. Add evolve/mutate that keeps patch identity while changing 10-20%.
163. Add breed/morph between preset A and preset B.
164. Add favorites-aware generation based on highly rated presets.
165. Add safety rollback when randomization creates silence or unsafe levels.

### I. Library, Presets, And Packs

166. Add a larger full-browser mode.
167. Add a folder tree for user and factory libraries.
168. Add smart folders: Favorites, Five Star, Recent, UKG Bass, UKG Vocal Chops, House Stabs, Minimal Plucks, Dirty Techno, Macro Rich, User, Factory.
169. Add tag editor for user presets.
170. Add custom user tags.
171. Add multi-select metadata editing.
172. Add preset notes.
173. Add pack manager with import/export.
174. Add duplicate preset detection.
175. Add preset compare before overwrite.
176. Add preset version history for user saves.
177. Add missing sample and missing wavetable warnings in browser rows.
178. Add per-preset audio preview rendering.
179. Add audition auto-advance.
180. Add preview MIDI patterns per category.
181. Add "find similar presets" using metadata and parameter distance.
182. Add preset dependency tracking for samples and wavetables.
183. Add pack artwork or pack color markers only if it does not clutter the UI.
184. Add search tokens for macro assignments, engine type, FX type, sequence type, and sample type.
185. Add construction-kit browser category.

### J. Genre-Specific Factory Content

186. Add a UKG Essentials pack: organ bass, Dred/Reese bass, late stab, bell pluck, vocal chop, shuffle bass.
187. Add a Speed Garage pack: organ bass, Reese bass, swung bass, offbeat stab, chopped vocal.
188. Add a Deep House pack: dub chord, warm sub, M1-style stab using original sources, soft pluck, pad stab.
189. Add a Tech House pack: rubber bass, rolling bass, percussive pluck, stab hit, noise riser.
190. Add a Minimal pack: click pluck, sine sub pulse, resonant blip, tuned percussion hit, tiny noise accent.
191. Add a Warehouse Techno pack: drive stab, FM ping, pulse bass, metallic hit, distorted drone.
192. Add preset construction kits with suggested BPM, key, sequence, macro intent, and recommended Ableton MIDI clip usage.
193. Add "starter from genre" buttons on HOME or LAB.
194. Add factory macro names and descriptions that explain performance intent.
195. Add factory safety validation for loudness, stuck notes, mono bass, and guard activity.

### K. Quality, Testing, And Ableton Release

196. Add denormal protection audit.
197. Add all-parameter smoothing audit.
198. Add zipper-noise automation sweep tests.
199. Add oscillator aliasing render tests.
200. Add filter resonance stability tests.
201. Add drive oversampling tests.
202. Add voice stealing audit.
203. Add stuck-note stress tests.
204. Add sustain-pedal behavior tests.
205. Add host transport start/stop/jump tests.
206. Add save/reopen preset state tests.
207. Add Ableton checklist: scan, load, automate, save set, reopen set, duplicate track, freeze, flatten, export, reopen.
208. Add UI open/close stress test while audio runs.
209. Add factory preset loudness validation.
210. Add randomizer loudness validation.
211. Add sample import failure tests.
212. Add file-permission handling for user library saves.
213. Add crash-safe preset writing through temp file and atomic replace.
214. Add CI for CMake build and non-GUI checks.
215. Add signed/notarized macOS release workflow.
216. Add installer or reliable VST3 copy script.
217. Add AU target after VST3 remains stable.
218. Add macOS Intel and Apple Silicon release checks.
219. Add plugin metadata audit: manufacturer, plugin name, category, version, automation names.
220. Add user manual pages for the main workflows.

## Multi-Agent Research Refresh

Two focused research passes were run on 2026-06-28: one for modern synth/source engines and one for club samplers, FX, groove tools, and Ableton workflow. The result does not change the whole roadmap, but it clarifies what should happen next.

Top missing product capabilities:

1. Source rack depth: true wavetable playback, sample/granular source options, focused FM/organ engines, second layer, per-layer mute/solo/blend, and source snippets.
2. Fast visual modulation: drag-to-modulate, right-click assignment, route range editing, route curves/slew/invert, source activity meters, and hover route overlays. First right-click assignment pass is implemented for MOD-targetable sliders, horizontal slider feedback now shows stripes/badges while preserving base tooltips, and MOD targets update the selected-control route summary on hover.
3. Serious UKG slicing: transient/beat/manual slice markers, per-slice start/end/nudge/pan/fades/probability/playback mode, slice-to-sequencer lanes, and missing-sample relink.
4. Motion lanes: step-LFO/MSEG sources and sequencer lanes for sample slice, filter cutoff, wavetable position, pump depth, delay throw, reverb throw, drive, and macro amount.
5. Club FX quality: multiband drive, mid/side routing, oversampling, gain compensation, external sidechain/pump input, drawable shapers, reverse/dub delay, and granular/time fills.
6. Preset discovery: audio previews, smart folders, custom tags, similar-preset search, construction-kit preset type, dependency warnings, and pack import/export.
7. Genre generators: UKG vocal chop templates, Dred/Reese and organ-bass starters, tech-house bassline generator, minimal Euclidean/sparse tools, techno feedback/resonator scenes, and house chord/organ stabs.
8. Ableton release polish: MIDI drag import/export, automation-name audit, save/reopen/freeze/flatten checklist, sample collection/relink checks, sidechain routing tests, signing/notarization, and installer/copy workflow.

Reference URLs used for this refresh:

- https://xferrecords.com/products/serum-2
- https://www.arturia.com/products/software-instruments/pigments/overview
- https://kilohearts.com/products/phase_plant
- https://www.minimal.audio/products/current
- https://www.roland.com/us/products/rc_zenology_pro/
- https://u-he.com/products/hive/
- https://u-he.com/products/diva/
- https://www.korg.com/us/products/software/modwave_native/
- https://www.korg.com/us/products/software/wavestate_native/
- https://www.ableton.com/en/live-manual/12/midi-tools/
- https://www.ableton.com/en/manual/live-instrument-reference/
- https://www.ableton.com/en/manual/live-audio-effect-reference/
- https://www.cableguys.com/shaperbox
- https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- https://deviousmachines.com/product/infiltrator/
- https://support.output.com/en/collections/10910224-portal-manual
- https://blog.native-instruments.com/uk-garage-music/
- https://www.musicradar.com/how-to/uk-garage-tutorial
- https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/

## Recommended Build Order

1. Componentize the editor and expand the first CTest layout bounds audit into richer overlap checks.
2. Expand the first right-click modulation assignment and visual-feedback passes with drag-from-source assignment, hover route overlays, and per-route range editing.
3. Add transient/manual sample slicing with per-slice start/end/nudge/pan/fades/probability and slice-to-sequencer lanes.
4. Add per-step sequencer ratchets, pattern slots, chord paint, and assignable modulation lanes.
5. Add a true wavetable oscillator with preview, position modulation, simple warp, and safe factory tables.
6. Add multiband drive with oversampling, gain compensation, and bass-safe routing.
7. Add full-browser mode, smart folders, user tag editing, audio previews, dependency warnings, and construction kits.
8. Add UKG source tools: organ/drawbar source, Dred/Reese source recipe, vocal-chop construction kits.
9. Add step LFO/MSEG sources and deeper route processors.
10. Add Ableton save/reopen/freeze/flatten validation and release signing.

## GitHub Issue Mapping

- UI extraction and layout checks: #61
- Control feel: #29
- Wavetable, source, and character filters: #43
- UKG bass and source modes: #41 and #33
- UKG vocal slice workflow: #34 and #55
- Sequencer lanes and pattern utilities: #50
- Modulation assignment, MSEG, and rings: #30, #40, and #56
- Club FX routing and shapers: #51 and #60
- Smart mutation and variation workflow: #49
- Tagged browser and construction kits: #42
- Audio quality and oversampling: #58
- Umbrella deep backlog: #62
