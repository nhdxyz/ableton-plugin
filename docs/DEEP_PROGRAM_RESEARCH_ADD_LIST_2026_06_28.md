# Nate VST Deep Program Research Add List

Research date: 2026-06-28

Purpose: this is the next "what else should we add" audit for Nate VST. It compares the current project direction against current flagship synths, samplers, modulation tools, browser workflows, club FX, and UK garage production references. The goal is to create a long, prioritized feature inventory for a house, UKG, tech-house, techno, and minimal-focused instrument.

This is product-pattern research only. Do not copy proprietary layouts, presets, wavetables, samples, DSP code, names, branding, or exact workflows.

## Current Nate VST Baseline

Based on the current repo docs and recent implementation notes, Nate VST already has these foundations:

- Mac-first JUCE/CMake VST3 project for Ableton.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, top meter, and bottom keyboard panels.
- Subtractive synth with two oscillators, sub, noise, filter, drive, envelopes, unison, Osc Warp, and output guard.
- Bandlimited quality pass for bright oscillator sources.
- Randomization Lab with section locks, recipes, mutate/vary/wild, snapshots, undo/redo, and genre direction.
- Modulation matrix, LFO/curve display, source badges, right-click assignment path, macro editor, LFO 2, S&H, Smooth, and Chaos movement sources.
- Sample Lab with waveform display, phrase/chop controls, slice pads, Slice Keys, per-slice region, pitch, gain, pan, probability, reverse, stutter, and choke memory.
- Piano-roll style sequencer with scale/chord helpers, velocity, probability, timing, length lanes, groove templates, host sync, undo, rotation, variation, and MIDI export.
- Addable/reorderable FX rack with delay, reverb, drive, pump, EQ/tone, width, guard, modulation FX, throws, and module presets.
- Preset library with recursive folders, categories, favorites, 1-5 ratings, metadata, search, sort, filters, macro previews, audition, and user/factory separation.
- Factory starter presets for UKG, house, tech-house, techno, and minimal.
- First editor layout audit and plugin validation workflow.

That means the big gap is no longer "basic controls." The big gap is deeper engines, more visual movement, richer sampling, professional routing, better content discovery, and less cluttered UI.

## References Reviewed

Core synths and hybrid instruments:

- Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- Minimal Audio Current: https://www.minimal.audio/products/current
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Ableton Meld and Live 12 instrument reference: https://www.ableton.com/en/packs/meld/ and https://www.ableton.com/en/live-manual/12/live-instrument-reference/

Motion, FX, sampler, and browser references:

- Ableton Live 12 MIDI Tools: https://www.ableton.com/en/live-manual/12/midi-tools/
- Ableton Roar: https://www.ableton.com/en/live/all-new-features/
- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Serato Sample: https://serato.com/sample
- Kontakt browser and presets: https://www.native-instruments.com/ni-tech-manuals/kontakt-manual/en/browser-and-presets
- XLN XO: https://www.xlnaudio.com/products/xo and https://support.xlnaudio.com/hc/en-us/articles/16920363887645-Interface-Overview
- Algonaut Atlas: https://algonaut.audio/

Genre and production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- Attack Magazine UK garage beat breakdown: https://www.attackmagazine.com/technique/beat-dissected/uk-garage/
- NITELIFE Audio UK garage vocal cuts: https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/

## Research Findings That Change The Backlog

1. Serum 2 moved the expectation beyond classic wavetable. A current flagship hybrid synth now includes wavetable, multisample, sample, granular, spectral, flexible FX routing, arpeggiator, and clip sequencing. Nate VST should still add wavetable first, but the architecture should be ready for sample/granular/spectral-style source lanes instead of a single hardwired oscillator page.

2. Pigments and Current show that Play View matters. A modern synth needs a low-distraction performance view with macros, source status, audition, and visual feedback, plus deeper edit views only when needed.

3. Phase Plant, Hive, and Meld show that routing depth is a core differentiator. Nate VST should evolve from "many controls" to clear source lanes, effect lanes, modulation lanes, and reusable module presets.

4. Korg modwave and wavestate show that lane-based motion is a major gap. Per-step note lanes are only the start. Nate VST should add independent lanes for sample choice, pitch, duration, shape, filter, FX, probability, and skip behavior.

5. ShaperBox and Saturn show that club FX need visual timing and multiband power. Drawable time curves, per-band processing, external sidechain, envelope followers, oversampling, gain compensation, and live modulation visualization should become major FX pillars.

6. Serato Sample, Ableton Simpler, XO, Atlas, and Kontakt show that sampling is a workflow, not a file picker. The sampler should add drag/drop, key/BPM detection, colored waveform cues, cue pads, slice modes, hot-swap, missing-file handling, sample maps, similar-sound replacement, audition in context, and per-pad editing.

7. UKG references keep pointing at the same musical needs: 2-step swing, late hats, syncopated rim/clap movement, Dred/Reese bass, organ bass, chopped vocals, tiny vocal snippets, pitch/pan/filter per slice, skippy quantization, and tight low-end safety.

8. The UI should become more modular before the next large engine. The current one-file editor can keep working, but serious feature growth will be painful without child components, focused views, resizable layout, screenshot regression, and a proper control inspector.

## Highest-Value Additions

If we only pick the next 15 big work tracks, they should be:

1. True wavetable oscillator with position, interpolation, wavetable display, modulation destinations, and safe factory tables.
2. Drag-to-modulate workflow with visible modulation rings and source colors on knobs.
3. MSEG/curve editor usable as LFO, envelope, step motion, filter motion, and FX shaper source.
4. Source rack with Layer A/B, mute/solo, pan, width, routing, copy/paste, and layer snippets.
5. UKG organ/drawbar source, Reese/Dred bass builder, and mono-safe bass utility.
6. Per-step modulation lanes for cutoff, drive, pump, delay throw, reverb throw, sample slice, and wavetable position.
7. Pattern scenes, pattern chaining, fills, and Ableton-friendly MIDI drag/export/import.
8. Drag-and-drop audio import with transient/grid/manual slicing.
9. Time-stretch, key/BPM detection, hot-swap, and formant-safe vocal chop investigation.
10. Multiband distortion with oversampling, gain compensation, mid/side mode, and per-band modulation.
11. Drawable Shaper modules for volume, filter, time, pan, width, drive, noise, and pitch/formant.
12. External sidechain input for real kick ducking and sidechain-triggered rhythmic effects in Ableton.
13. Full browser mode with construction kits, audio previews, smart collections, sample packs, and macro-preview sorting.
14. UI component extraction, resizable/compact modes, and screenshot/overlap regression tests.
15. Release-quality testing: pluginval, Ableton scan/save/reopen/freeze/flatten, automation smoothing, stuck-note, denormal, aliasing, and gain-staging tests.

## Long Feature Inventory

### UI, Layout, And Interaction

1. Split `PluginEditor` into child components for HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, top bar, and keyboard.
2. Add a shared `PanelFrame`, `KnobGroup`, `SectionHeader`, `ModuleHeader`, `BrowserRow`, `RouteBadge`, and `LaneEditor` UI library.
3. Add resizable UI at 85%, 100%, 125%, and 150%.
4. Add Ableton laptop compact mode.
5. Add large sound-design mode.
6. Add full-browser mode.
7. Add performance-only mode with macros, XY pads, throws, output meter, and panic.
8. Add selected-control inspector showing value, default, automation id, modulation routes, reset, lock, and copy route actions.
9. Add value popovers while dragging.
10. Add text entry for every continuous control.
11. Add consistent double-click reset for every parameter.
12. Add consistent fine-drag for every knob and slider.
13. Increase invisible knob hit areas.
14. Keep mousewheel editing disabled or opt-in to avoid Ableton trackpad accidents.
15. Add hover states for controls, badges, routes, pads, steps, slices, FX rows, and browser rows.
16. Add disabled-state visuals when controls do nothing in the current mode.
17. Add warning states for clipping, stereo sub, missing samples, missing wavetables, CPU strain, and invalid drag/drop.
18. Add icon buttons for save, favorite, audition, refresh, copy, paste, reset, undo, redo, up, down, bypass, solo, mute, and panic.
19. Add short musical tooltips for unfamiliar controls.
20. Add debug bounds overlay.
21. Add screenshot regression captures for every panel.
22. Add layout tests for text overflow, component overlap, and offscreen controls.
23. Add a signal-flow mini-map: Sources -> Filter -> Sample -> FX -> Guard -> Output.
24. Add routing overlays for insert, send, parallel, multiband, sidechain, and direct paths.
25. Add small meters for Source A, Source B, Sampler, FX input, FX output, low band, mid band, high band, and final output.
26. Add CPU and active voice count in the top bar.
27. Add current preset dirty marker and compare/revert state.
28. Add keyboard focus handling for spacebar/host-safety so plugin shortcuts do not fight Ableton.
29. Add UI theme tokens so visual polish can happen without editing every component.
30. Add automated contrast checks for labels, badges, and selected states.

### Home And Performance Views

31. Redesign HOME around Play View: preset, engine summary, macros, XY, randomize, low-end safety, output, and recent patches.
32. Add source status cards that show Layer A, Layer B, Sampler, and FX status without opening full edit pages.
33. Add quick engine audition/solo buttons from HOME.
34. Add "make it more UKG", "make it more deep house", "make it more minimal", "make it more warehouse", and "make it more tech-house" transforms.
35. Add macro amount preview in HOME and LIBRARY.
36. Add performance snapshots A/B/C/D with morph time.
37. Add XY motion recorder for macro gestures.
38. Add safe randomize preview before committing.
39. Add randomization history with named mutations.
40. Add quick "reset to mix-safe" action for gain, low-end, width, and FX tails.

### Source Engines

41. Add source rack architecture.
42. Add Layer A and Layer B with independent source, amp, pan, width, filter routing, FX send, solo, mute, and output trim.
43. Add layer copy, paste, reset, compare, and random lock.
44. Add layer snippets: Clean Sub, Dred Bass, Reese Upper, Organ Bass, Late Stab, Chord Stab, Minimal Pluck, Techno Hit, Bell Ping, Vocal Texture.
45. Add source-level presets independent of full patch presets.
46. Add source dependency tracking for samples and wavetables.
47. Add per-source drift for pitch, phase, pan, filter, envelope, and level.
48. Add oscillator start phase controls.
49. Add voice-stack modes: classic unison, supersaw, spread-safe, chord stack, octave stack, and garage fifth.
50. Add mono sub protection per source.
51. Add per-source keytracking and velocity response.
52. Add per-source glide, legato, and retrigger mode.
53. Add per-source high quality/offline render mode.
54. Add per-source freeze/resample to sampler.
55. Add routing to Filter A, Filter B, both filters, direct FX, or sampler blend.

### Wavetable, Spectral, FM, And Hybrid Oscillators

56. Add true wavetable oscillator playback.
57. Add wavetable position parameter and modulation destinations.
58. Add wavetable display with current frame, frame path, playhead, and active modulation overlay.
59. Add factory wavetable browser.
60. Add user wavetable folder indexing.
61. Add safe WAV wavetable import.
62. Add import validation for frame size, silence, channels, sample rate, and clipping.
63. Add interpolation quality choices.
64. Add anti-aliasing stress tests for high notes.
65. Add wavetable warp modes: bend, mirror, fold, sync-style, phase skew, spectral tilt, formant tilt, quantize, bit shape, and noise smear.
66. Add dual warp slots per wavetable oscillator.
67. Add oscillator hard sync.
68. Add PWM/pulse width for square sources.
69. Add ring modulation between oscillators.
70. Add FM/PM between oscillators with protected gain.
71. Add dedicated 3-operator FM source for UKG bells, organ edges, metallic stabs, and techno pings.
72. Add additive/partial source for hollow bells and glassy minimal tones.
73. Add drawbar/organ source for UKG, speed garage, and house organ bass.
74. Add original/legal PCM-style house organ, string, piano, and stab source.
75. Add resonator/modal source for minimal plucks and percussive mallet tones.
76. Add granular source for vocal clouds, pads, and transition texture.
77. Add spectral/resynthesis investigation for advanced vocal and stab textures.
78. Add noise source types: white, pink, brown, air, vinyl, attack tick, rumble, metallic, digital hash, tape hiss, and room tone.
79. Add scale-aware oscillator modes inspired by Ableton-style pitch-quant motion.
80. Add MPE-ready pressure, slide, and per-note pitch support later.

### Filters, Drive, Low-End, And Tone

81. Add dual filters.
82. Add serial, parallel, split, and per-layer filter routing.
83. Add filter blend/morph.
84. Add notch, comb, all-pass, formant, vowel, phaser, and resonator filter modes.
85. Add clean digital, ladder-style, SEM-style, MS-style aggressive, and diode-style filter characters.
86. Add filter keytracking with center note and slope.
87. Add filter FM from oscillator, noise, or audio-rate LFO with safety limits.
88. Add filter drive pre/post switch.
89. Add per-filter saturation color.
90. Add gain compensation for every drive model.
91. Add soft clip, hard clip, diode, tube, tape, transformer, foldback, wavefolder, fuzz, downsample, and bitcrush drive models.
92. Add transient-preserving drive mode for plucks and stabs.
93. Add loudness-matched audition for drive models.
94. Add oversampling modes for drive, filter nonlinearities, clipper, and limiter.
95. Add DC blocking and denormal tests.
96. Add club low-end monitor for sub buildup, stereo bass, phase cancellation, DC offset, and excessive limiter activity.
97. Add mono-maker below a selectable crossover.
98. Add low-end sidechain preview display.
99. Add bass note range helper for UKG and house keys.
100. Add "keep sub clean" routing that drives upper harmonics without distorting sub.

### Modulation And Motion

101. Add drag-and-drop modulation from source chips to controls.
102. Add modulation rings around knobs with source-colored arcs.
103. Add hover route overlays for each parameter.
104. Add animated source activity meters.
105. Add source value preview beside badges.
106. Add per-route amount, min, max, polarity, curve, smoothing, and bypass.
107. Add route processors: scale, offset, invert, clamp, quantize, slew, sample/hold, rectify, and randomize.
108. Add macro assignment browser by destination.
109. Add macro learn mode.
110. Add macro scenes A/B/C/D.
111. Add morph pad between snapshots.
112. Add MSEG/multi-point envelope source.
113. Add function generators that can run as envelopes, gates, LFOs, slew limiters, and envelope followers.
114. Add step LFO source with hold, ramp, curve, probability, slew, and random per step.
115. Add audio envelope follower source.
116. Add external sidechain envelope follower source.
117. Add random/sample-and-hold source with clock, smoothing, and seed.
118. Add chaos/mod wander source with rate, range, and attractor style.
119. Add per-note random and per-voice spread modulation sources.
120. Add MPE sources for pressure, slide, and per-note pitch later.
121. Add modulation matrix filtering/search.
122. Add "show moving controls" mode.
123. Add modulation undo/redo independent of global preset changes.
124. Add modulation route presets.
125. Add visual modulation stress tests so no animated path allocates in audio code.

### Sequencer, Piano Roll, And Groove

126. Add multiple patterns per preset.
127. Add pattern scene buttons: A, B, fill, drop, breakdown, and transition.
128. Add pattern chaining and song slots.
129. Add fill trigger and one-shot fill mode.
130. Add per-step ratchet/retrigger.
131. Add per-step slide/glide.
132. Add per-step accent linked to amp, filter, and drive.
133. Add per-step sample slice lane.
134. Add per-step wavetable position lane.
135. Add per-step filter cutoff lane.
136. Add per-step drive lane.
137. Add per-step pump depth lane.
138. Add per-step delay throw lane.
139. Add per-step reverb throw lane.
140. Add per-step FX select/probability lane.
141. Add step conditions: first, not first, every 2, every 4, fill only, random, and previous-hit dependent.
142. Add probability ranges instead of single probability values.
143. Add lane-specific groove amount.
144. Add UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal sparse, dub techno pulse, warehouse straight, and broken percussion groove templates.
145. Add swing microtiming visualization.
146. Add groove template browser and user groove save.
147. Add Euclidean generator.
148. Add bassline contour generator.
149. Add recombine tool that keeps rhythm but changes pitches.
150. Add density control for generated patterns.
151. Add scale-aware note generation.
152. Add chord paint tools for minor 7, minor 9, major 7, sus, garage organ, dub chord, and house stab voicings.
153. Add voicing spread and inversion lock.
154. Add strum direction curves.
155. Add MIDI drag export directly from UI to Ableton.
156. Add MIDI drag import from MIDI clips/files.
157. Add MIDI capture from played notes into the sequencer.
158. Add transport-jump stress tests for host sync.
159. Add pattern preset snippets.
160. Add audio render/export of the internal sequence.

### Sampler, Vocal Chops, And Sample Discovery

161. Add drag-and-drop WAV/AIFF/FLAC import onto the waveform.
162. Add drag sample replacement while preserving slice settings.
163. Add sample missing/relink workflow.
164. Add sample key, BPM, duration, peak, loudness, and confidence metadata.
165. Add waveform color cues by frequency band.
166. Add transient-based slicing.
167. Add beat-grid slicing.
168. Add equal-region slicing.
169. Add fully manual slice markers.
170. Add slice nudge, fade in, fade out, snap to zero crossing, and crossfade.
171. Add per-slice attack, hold, decay, release.
172. Add per-slice filter, drive, tone, and FX sends.
173. Add per-slice pitch envelope and pitch ramp.
174. Add per-slice speed/rate modulation for tape-stop style moves.
175. Add one-shot, gate, loop, and thru playback modes.
176. Add mono/poly/choke voice modes.
177. Add per-slice output routing later if the host format supports the planned bus design.
178. Add slice audition on hover/click.
179. Add colored cue pads.
180. Add pad naming.
181. Add velocity mode per pad.
182. Add keyboard mode to play a sample chromatically.
183. Add formant-preserving pitch shift investigation.
184. Add high-quality time-stretch/warp mode.
185. Add beat-grid editing for unquantized vocals.
186. Add "Find slices" algorithm.
187. Add random slice assignment with repetition guardrails.
188. Add chop-to-sequencer lane.
189. Add sample hot-swap audition in context.
190. Add sample browser with folders, tags, favorites, recents, packs, key, BPM, type, length, and brightness.
191. Add sample similarity search for vocal chops, stabs, bass hits, drums, and texture one-shots.
192. Add XO/Atlas-style sample map later for large user libraries.
193. Add construction-kit presets that store sample, sequence, macros, key, BPM, and recommended FX.
194. Add audio drag-out/render current chop to Ableton.
195. Add stem-separation research as a later, optional track after core sampler quality is solid.

### UK Garage, House, Tech-House, Techno, And Minimal Builders

196. Add UKG 2-step beat template with late hats and syncopated kick.
197. Add speed garage organ bass template.
198. Add Dred bass builder with reverse-feeling filter motion.
199. Add Reese builder with clean sub and stereo upper harmonics.
200. Add organ/drawbar bass builder.
201. Add UKG chopped vocal starter with swing, pitch, pan, and delay/reverb throw assignments.
202. Add Todd Edwards-style micro vocal chop transform as a broad pattern idea, not a preset copy.
203. Add late stab generator with ghost hits and swing amount.
204. Add dub chord builder with bandpass/noise/reverb/delay motion.
205. Add house piano stab builder with legal/original sources.
206. Add tech-house rubber bass builder.
207. Add minimal click pluck builder.
208. Add techno warehouse stab builder.
209. Add acid-style filter sequence builder.
210. Add organ chord memory presets for minor 7, minor 9, sus, and garage voicings.
211. Add call/response bassline generator.
212. Add sparse vocal tag generator.
213. Add offbeat chord stab pattern.
214. Add one-knob "more swing" transform that adjusts sequencer timing and chop offsets together.
215. Add one-knob "more pump" transform that adjusts sidechain curve, envelope, and FX.
216. Add one-knob "more dirty" transform that drives upper bands while preserving sub.
217. Add "mix-safe UKG bass" action.
218. Add "garage late hats" sequencer helper.
219. Add "minimal negative space" helper that removes notes while preserving groove.
220. Add "warehouse tension" helper using pitch ramps, noise, drive, and reverb throws.

### FX, Routing, And Mixing

221. Add true slot-based FX rack with duplicate effects.
222. Add parallel FX lanes.
223. Add send bus for delay/reverb throws.
224. Add external sidechain input.
225. Add sidechain-triggered pump curve.
226. Add drawable volume shaper.
227. Add drawable filter shaper.
228. Add drawable time shaper.
229. Add drawable pan shaper.
230. Add drawable width shaper.
231. Add drawable drive shaper.
232. Add drawable noise shaper.
233. Add drawable pitch/formant shaper.
234. Add multiband distortion with up to 3 bands for Nate VST first, then optional 6-band later.
235. Add per-band mute/solo.
236. Add per-band drive, mix, tone, feedback, dynamics, and output.
237. Add mid/side processing for selected FX.
238. Add gain-compensated distortion audition.
239. Add transient shaper.
240. Add compressor.
241. Add limiter/clipper with peak and oversampling quality.
242. Add LUFS/peak guidance for preset level consistency.
243. Add tape stop/start module.
244. Add beat repeat/glitch module.
245. Add granular FX module for vocal fills.
246. Add convolution/impulse-style space later.
247. Add resonator/comb bank for stabs.
248. Add formant/vowel FX.
249. Add lo-fi/noise module with wobble, hum, vinyl, cassette, and digital artifacts.
250. Add per-FX preset save/load.
251. Add full-rack preset save/load.
252. Add FX copy/paste.
253. Add FX undo/redo.
254. Add FX routing visualization.
255. Add FX latency reporting tests if any future module adds latency.

### Browser, Presets, Packs, And Content

256. Add full browser page with dense columns.
257. Add preset audio preview rendering.
258. Add automatic preview regeneration when a preset changes.
259. Add macro-preview columns for Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw.
260. Add key, BPM, pack, category, subcategory, author, rating, favorite, date, source type, sample dependency, and wavetable dependency columns.
261. Add smart collections.
262. Add user crates/playlists.
263. Add editable tag manager.
264. Add batch retagging.
265. Add duplicate preset detection.
266. Add find-similar preset by metadata and parameter distance.
267. Add favorites-aware random preset suggestions.
268. Add preset compare/revert.
269. Add preset mutation history.
270. Add section presets: source, filter, modulation, sequence, sampler, FX, and macros.
271. Add construction-kit browser packs.
272. Add pack installer/import/export.
273. Add dependency report for missing samples and wavetables.
274. Add "collect and save" pack export.
275. Add factory packs: UKG Basslines, Garage Chops, House Chords, Tech-House Loops, Minimal Plucks, Dub Techno Stabs, Warehouse Hits, Vocal Tags, Reese/Dred Bass, Organ Stabs.
276. Add sample pack browser integrated with SAMPLE.
277. Add sample preview in context with the internal sequencer.
278. Add browsing by musical role: Sub, Top Bass, Organ, Chord, Stab, Pluck, Vocal Chop, FX Throw, Texture, Sequence.
279. Add browser keyboard shortcuts.
280. Add browser pinned-open mode.

### Quality, Testing, Distribution, And Ableton Workflow

281. Add CI for macOS build and tests.
282. Add pluginval CI or release-gate script.
283. Add automated preset load smoke test for all factory presets.
284. Add parameter migration tests for old presets.
285. Add automation smoothing tests.
286. Add stuck-note/all-notes-off tests.
287. Add host transport jump tests.
288. Add sample missing/relink tests.
289. Add denormal and silence CPU tests.
290. Add aliasing render tests for oscillators.
291. Add oversampling CPU/latency tests.
292. Add output gain ceiling tests.
293. Add modulation stress tests.
294. Add UI screenshot regression tests.
295. Add Ableton manual checklist: scan, load, automate, save, reopen, duplicate, freeze, flatten, export, and reload a missing-sample set.
296. Add VST3 install/copy script.
297. Add codesigning and notarization workflow.
298. Add versioned release notes.
299. Add crash log collection notes for local testing.
300. Add beta preset/content compatibility policy.

## Recommended Next Implementation Slices

1. UI foundation slice: extract panels/components enough to keep future features manageable, then add screenshot/overlap regression.
2. Wavetable slice: add internal wavetable oscillator, position parameter, display, and modulation destinations.
3. Modulation workflow slice: add MSEG plus visible modulation rings and route inspector.
4. UKG sampler slice: add drag/drop audio, transient/grid/manual slicing, slice nudge/fades, and sample metadata.
5. Motion lane slice: add assignable per-step modulation lanes and pattern scenes.
6. Club FX slice: add drawable volume/filter/time shaper and external sidechain.
7. Drive quality slice: add multiband drive, oversampling, and gain compensation.
8. Browser/content slice: add rendered audio previews, construction kits, section presets, and dependency reports.

## Issue Mapping

Existing open issues cover most of this research:

- #62 tracks the deep research backlog.
- #43 covers wavetable oscillator and character filter work.
- #40 covers MSEG and visual modulation.
- #59 covers layer architecture.
- #34 covers UKG vocal slice workflow.
- #50 covers sequencer lanes and MIDI capture.
- #60 covers rhythmic FX shaping.
- #63 covers sample discovery and similarity.
- #61 covers UI extraction and layout checks.

Avoid creating duplicate issues unless a feature becomes concrete enough for a focused implementation slice.

Implemented update:

- First internal wavetable oscillator slice is now implemented with generated table frames, Osc 1/Osc 2 WT position parameters, a compact SYNTH display, modulation destinations, migration defaults, and factory preset examples. Wavetable import, browser, drag/edit display, bend/sync/FM/warp modes, and user table folders remain open.
