# Nate VST Next Feature Research

Research date: 2026-06-28

Current house-focused refresh: 2026-06-29, see `docs/HOUSE_VST_COMPETITOR_RESEARCH_2026_06_29.md`.

This is the current "what else should we add" list for Nate VST. It compares the repo against current reference products and narrows the next backlog toward the actual product identity: a fast house, UK garage, tech-house, minimal, and techno instrument for Ableton.

The goal is not to clone any product. These notes translate broad product patterns into Nate VST-specific features.

## References Reviewed

Synth and hybrid instrument references:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- reFX NEXUS5: https://refx.com/nexus/
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
- FabFilter Pro-L 2: https://www.fabfilter.com/products/pro-l-2-limiter-plug-in
- Output Portal: https://output.com/products/portal
- XLN Audio XO: https://www.xlnaudio.com/products/xo
- Sonic Academy KICK 3: https://www.sonicacademy.com/products/kick-3
- Xfer Cthulhu: https://xferrecords.com/products/cthulhu
- Scaler 3: https://scalermusic.com/products/scaler-3/
- Serato Sample: https://serato.com/sample

UK garage and club-production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage tutorial: https://www.musicradar.com/how-to/uk-garage-tutorial

## Current Repo Position

Nate VST already has a real foundation:

- JUCE/VST3/CMake project with Ableton-focused validation.
- Subtractive synth core with oscillator 1, oscillator 2, sub, noise, unison, filter character/slope, drive, envelopes, and output guard.
- A first bandlimited oscillator quality pass.
- A live/clickable SYNTH house source rack with expanded source-layer focus overlay for Sub, Body, Character/Stab, Transient/Noise, and Chop roles.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, and INFO panels.
- Randomization recipes, section scope, locks, undo/redo, and genre seeds.
- Eight performance macros plus a HOME/MOD macro shape map, Motion/Space XY macro pad, visual MOD macro assignment pad, and expanded macro focus overlay.
- HOME Patch Snapshot for selected-preset metadata, patch role, source type, output safety, performance meters, compare readiness, active random-candidate state, recall, and audition.
- HOME signal-flow strip for Source, Filter, Motion, FX, Guard, and Output active-state visualization.
- Modulation matrix, LFO curve editing, pump curve editing, visual macro assignment editing, route bypass/delete, and S&H/Smooth/Chaos sources.
- Sample waveform display, chop window, phrase markers, visual selected/custom slice overlays, adaptive expanded chop focus overlay, slice pads, reverse, pitch ramp, stutter, and UKG chop helpers.
- 16-step piano-roll style sequencer with chord memory, expanded house/dub chord colors, templates, appended house/tech/minimal/drive groove modes, velocity/probability/timing/length/lock/ratchet/condition/slide lanes, A/B/Fill/Drop pattern scenes, MIDI export with groove/ratchet/condition/slide timing, and host sync.
- Addable/reorderable FX rack with delay, reverb, drive, pump, tone/EQ, width, guard, modulation FX, throws, and module presets.
- Live pump phase/reduction feedback, top-bar output safety status, and Guard reduction telemetry for checking ducking and level risk while sound-designing.
- Preset library with recursive folders, categories, search, sort, favorites, 1-5 rating, metadata, macro previews, filters, compare/revert, smart crates, 96 factory presets, rendered preview cache/playback, visible-row preview warming, and preview level badges.

The missing work is not "add one more page of knobs." The missing work is depth, visual feedback, source quality, library maturity, and genre-specific generation.

For the latest house-focused comparison against Serum 2, NEXUS5, ZENOLOGY Pro, KORG Collection/wavestate/modwave, Pigments, MASSIVE X, Phase Plant, Falcon, ShaperBox, Saturn, and Ableton, see `docs/HOUSE_VST_COMPETITOR_RESEARCH_2026_06_29.md`. `docs/PROGRAM_RESEARCH_GAP_LIST.md` remains the older broad program matrix; this file remains the shorter prioritized add list.

## Highest-Level Gaps

1. The synth source engine is still shallow compared with modern hybrid synths.
2. Modulation is functional, but assignment, route ranges, and source parity are not yet complete.
3. The sampler is now a stronger visual chop starter with eight-pad slice overlays and focus editing, but not yet a serious UKG vocal and phrase instrument.
4. The sequencer is useful and now has first-pass genre groove templates, saved/exported ratchets, odd/even/fill step conditions, a saved/exported Slide lane with live/export legato overlap, scene-chain export, first-pass `Build 4-Bar Chain` scene generation, first-pass live scene-chain step/control playback, adaptive expanded-focus `Auto` / `2 Bar` / `4 Bar` chain length mode, Bass Contour, Chord Stab Paint, and current/chain MIDI drag-out, but not yet arranger-style chain editing, deeper per-scene automation/rate/follow behavior, deeper glide/portamento controls, deeper condition vocabulary, multi-lane locks, MIDI import, audio drag-out, or full arrangement tools.
5. The FX rack is broad, but still needs pro-level drive, shaper, and routing depth.
6. Randomization needs stronger "useful musical result" guardrails.
7. The preset browser has first smart crates, rendered previews, visible-row warming, level badges, and compact-width rows that keep sound metadata readable, but needs fully background preview generation, waveform badges, construction-kit workflows, expansion pack manifests, and pack tooling.
8. There is no first-class house drum/kick-bass construction lane yet; XO/KICK/Maschine-style expectations point toward kit pads, groove/accent lanes, kick/bass checks, rendered loop/one-shot drag-out, and construction-kit stems.
9. The UI has first layout/theme audits, compact Library row checks, compact MOD matrix/source-meter checks, adaptive macro/chop/source/SEQ focus overlays, and tall-focus growth checks, but still needs component extraction, screenshot checks, and stronger FX/Mod focus views before adding large engines.
10. Audio quality needs oversampling, smoothing, gain-staging, and render tests.
11. Ableton release workflow needs scan/load/automate/save/reopen/duplicate/freeze/flatten/missing-sample/loudness testing and macOS signing/notarization.

## Long Add List

### A. UI, Layout, And Interaction

1. Split `PluginEditor` into one component per page: HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, INFO, top bar, and keyboard.
2. Add an automated component-overlap test for every panel at the current plugin size. First editor bounds audit implemented through CTest, with added bottom-keyboard width/coverage checks; component extraction and richer overlap/text-fit rules remain open.
3. Centralize UI theme tokens before exposing selectable themes. First shared-control/editor-chrome token pass plus `ThemeAudit` contrast validation is implemented; custom display components and screenshot review remain open.
4. Add screenshot regression captures for every panel after each significant UI change.
5. Add a debug bounds overlay toggle for manual layout reviews.
5. Add UI scale choices: 85%, 100%, 125%, and 150%.
6. Add a compact Ableton-laptop layout that shows fewer controls per page.
7. Add a larger sound-design layout once component extraction is complete.
8. Replace text-heavy buttons with icon buttons where the action is standard: save, reload, favorite, audition, undo, redo, copy, clear, move up, move down. First-pass global `Undo Edit`/`Redo Edit` buttons are implemented in the persistent selected-control strip; icon conversion remains open.
9. Add a global selected-control inspector that shows value, automation name, modulation routes, and reset action.
10. Add value popovers for every draggable control.
11. Add consistent double-click reset behavior across all sliders, buttons, and curve points.
12. Add per-control drag sensitivity classes: coarse, normal, fine, and bipolar.
13. Add larger invisible hit areas around small rotary controls.
14. Add page-level grouping around user tasks: Source, Shape, Motion, Space, Utility.
15. Add collapsible groups for dense pages instead of always showing every section.
16. Continue the professional HOME redesign beyond the implemented overview/Patch Snapshot/signal-flow pass with selected-control focus and larger performance/browser modes.
17. Extend the implemented signal-flow strip into deeper split/source, send, parallel, and Guard routing visualization.
18. Add voice count, CPU estimate, and guard activity in the top bar.
19. Add stronger disabled-state styling for inactive modules and mode-dependent controls.
20. Add color-coded domain language: source, filter, modulation, sequence, effects, library.

### B. Source Engines

21. Add a true wavetable oscillator with position modulation. First internal generated-table playback and Osc 1/Osc 2 WT position pass implemented; import, browser, and warp modes remain open.
22. Add a wavetable preview display with current frame and animated position. Osc 1/Osc 2 WT position controls are implemented, but the compact source-graph slot is currently used by the house layer rack; a dedicated drag/edit/import wavetable view remains open.
23. Add factory wavetables for UKG organ/bell, house chord, tech-house rubber bass, dub stab, and techno noise.
24. Add safe WAV wavetable import after factory playback is stable.
25. Add wavetable interpolation quality modes.
26. Add wavetable warp modes: bend, fold, mirror, sync-style, phase skew, spectral tilt, and formant-like tilt.
27. Add oscillator hard sync.
28. Add PWM for square/pulse sounds.
29. Add ring modulation between oscillators.
30. Add protected FM/PM between oscillators.
31. Add a focused 3-operator FM engine for UKG bells, pings, metallic stabs, and techno hits.
32. Add an additive/drawbar-style organ source for speed garage and UKG bass. First procedural Organ waveform pass is implemented as an appended source mode; deeper drawbar controls and source snippets remain open.
33. Add legal/original sampled-PCM-style starter sources for house piano/organ-style stabs. First procedural House Piano waveform pass is implemented for original house-piano chord sources; bundled sampled/PCM-style content and richer source controls remain open.
34. Add noise source colors: white, pink, brown, metallic, vinyl, air, digital hash. First pass implemented as White, Pink, Brown, Air, Tick, Vinyl, and Digital with Noise Decay; metallic/riser shaping and a dedicated lofi/noise FX module remain open.
35. Add supersaw/unison improvements: detune curve, phase spread, stereo spread, and mono-safe mode.
36. Add analog drift for pitch, phase, filter, envelope, and pan.
37. Add a second synth layer with layer mute, solo, copy, blend, and routing. First live/expanded source-rack visualization and click-to-focus behavior is implemented over the existing source controls; true second-layer DSP remains open.
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
52. Add transient-preserving drive for plucks and stabs. First Guard-stage Punch pass implemented for club-safe transient lift; drive-specific transient preservation remains open.
53. Extend club low-end telemetry beyond the current HOME assistant/analyzer with DC checks, long-window buildup warnings, phase-cancellation history, and guard-hit rate.
54. Add a one-knob "low-end tighten" transform for house and UKG bass patches.
55. Add a one-knob "stab bite" transform for chord and organ stabs.

### D. Modulation And Performance

56. Add drag assignment from modulation source chips to controls.
57. Add right-click "modulate with" menus on automatable controls. First pass implemented for MOD-targetable sliders.
58. Add visible modulation rings around modulated knobs and stripes/badges on modulated horizontal controls. Rotary rings, horizontal slider feedback, compact source badges, and source-colored accents are implemented.
59. Add source-colored modulation badges with route amount. First source-color pass implemented for macro, LFO, envelope, S&H, Smooth, Chaos, and LFO 2 routes.
60. Add hover-to-show routes for the parameter under the mouse. First pass implemented through the selected-control inspector; graphical hover overlays remain open.
61. Add route min/max range editing. Visual macro assignment now shows macro route counts, assigned destination chips, draggable bipolar depth, and a larger focus overlay; explicit range handles remain open.
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
77. Add full missing-file relink handling for saved presets beyond the first stale-audio clear and missing-path UI status guard.
78. Add transient-based slice detection.
79. Add beat-grid slicing.
80. Add equal-region slicing with user-selectable count.
81. Add fully manual slice marker creation and editing.
82. Add per-slice start, end, nudge, gain, pan, pitch, reverse, fade, and choke. First stored start/end region plus pitch/gain/pan/reverse/stutter/choke/nudge/fade memory, visual slice badges, and Slice Keys playback pass implemented for eight pads; manual markers and deeper playback modes remain open.
83. Add per-slice probability. First Slice Keys trigger-probability pass implemented with the SAMPLE `Ghost` action; deeper probability lanes and conditions remain open.
84. Add per-slice stutter and retrigger.
85. Add slice audition on hover/click. First waveform slice-lane click selection/audition is implemented; hover audition remains open.
86. Add keyboard mapping display for slices. First selected-slice status can call out C3-G3 Slice Keys mode; a dedicated visual keyboard map remains open.
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
98. Add house piano/organ stab starter recipes using original/legal sources. First procedural Organ/House Piano source pass is implemented for selected factory presets and organ/chord random recipes; larger recipe/content packs remain open.
99. Add a sample pack browser inside LIBRARY.
100. Add construction-kit presets that store sample, sequence, macros, key, BPM, and recommended FX.

### F. Sequencer, Piano Roll, And Groove

101. Add multiple sequencer patterns per preset. First four scene slots are implemented as A/B/Fill/Drop; chaining and more named sections remain open.
102. Add pattern scenes: A, B, fill, drop, breakdown, transition. First A/B/Fill/Drop capture/recall pass implemented.
103. Add deeper pattern chaining beyond the first live scene-chain playback, Build 4-Bar Chain, and forced chain-length passes.
104. Add per-step note length. First saved lane/playback/export pass implemented.
105. Add per-step ratchet/retrigger. First saved lane/playback/export pass implemented; repeat curves, per-ratchet probability, and deeper velocity falloff remain open.
106. Add deeper per-step slide/glide behavior beyond the first saved Slide lane/live-export overlap pass.
107. Add per-step accent tied to amp, filter, and drive.
108. Add per-step modulation lanes assignable to safe destinations. First single-destination Lock lane implemented for cutoff, drive, warp, pump, delay, reverb, and wavetable-position movement; multi-lane/sample-accurate locks remain open.
109. Add per-step sample slice lane.
110. Add per-step pump depth lane.
111. Add per-step delay throw lane.
112. Add per-step reverb throw lane.
113. Add probability ranges and deeper step conditions such as first, not first, every 4, random, follow, and per-repeat probability. First Always/Odd/Even/Fill condition lane is implemented for playback, scenes, state, and MIDI export.
114. Add scale quantize and key lock in the piano roll.
115. Add chord paint tools for minor 7, minor 9, major 7, sus, garage organ stabs, and dub chords. First Maj 7, Dom 7, Sus, House 9, and Dub chord colors plus house/dub chord pattern starters and Chord Stab Paint are implemented; deeper paint variants remain open.
116. Add voicing spread and inversion tools.
117. Add humanize with timing, velocity, gate, and pitch amount.
118. Add groove template library: UKG 2-step, speed garage, deep house shuffle, tech-house roll, minimal offbeat, dub techno pulse, warehouse straight, and broken percussion. First pass implemented for House Shuffle, UKG 2-Step Push, Tech House Tight, Minimal Skip, and Techno Drive; speed-garage, dub techno, warehouse, and broken-percussion variants remain open.
119. Add groove amount per lane.
120. Add swing microtiming visualization.
121. Add Euclidean generator for minimal blips, hats, and percussion-style synth hits.
122. Add bassline contour generator for rolling tech-house and UKG movement. First Off Bass and Rolling Bass pattern starters plus a Bass Contour Shape transform are implemented; deeper multi-phrase contour generation remains open.
123. Add recombine tool: keep rhythm, change notes.
124. Add MIDI drag export directly from the plugin UI into Ableton. First-pass current-pattern and scene-chain `.mid` drag-out is implemented; Ableton host validation and richer clip handoff remain open.
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
150. Add internal send bus for delay/reverb throws. First pass implemented with separate send amounts, dedicated send delay/reverb DSP state, throw-button integration, MOD destinations, and one-shot tail kill; deeper Dub Send/Short Room/Wash preset modes remain open.

### H. Randomization And Sound Discovery

151. Add lock buttons directly on every parameter group.
152. Add per-scope intensity: Source, Filter, Amp, Motion, FX, Seq, Sample, Library, Macros. First pass implemented for Source, Env, Filter, Sample, FX, Seq, and Macro section strength controls.
153. Add "useful only" guardrails for pitch, output gain, sub level, resonance, feedback, wet FX, and stereo bass. First validation, short-render check, auto-retry, and fallback pass is implemented; deeper musical-role validation remains open.
154. Add a user recipe builder for randomization.
155. Add randomization history beyond the current one-step undo/redo. First global sound-design edit history is implemented separately for manual UI edits, preset loads, snapshots, modulation routes, sample/slice edits, MSEG tools, FX rack changes, and slider gestures.
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
168. Expand the implemented smart-crate filter pass, centered browser actions, and folder/pack/style crate rail into full smart folders: New This Week, editable user crates, UKG Vocal Chops, House Stabs, Dirty Techno, Macro Rich, pack crates, and full-browser sidebar navigation.
169. Add tag editor for user presets.
170. Add custom user tags.
171. Add multi-select metadata editing.
172. Expand implemented preset notes into structured production tips, source-recipe context, and batch-editable metadata.
173. Add pack manager with import/export.
174. Expand duplicate preset handling beyond same-folder overwrite detection with rename-as-copy, version history, and near-duplicate detection.
175. Extend the implemented preset-load compare/revert, Save Target preview, and two-click overwrite guard into full edited-vs-saved safe-overwrite compare.
176. Add preset version history for user saves.
177. Add missing sample and missing wavetable warnings in browser rows.
178. Expand the implemented rendered per-preset preview cache/playback, visible-row warmer, and level badges into fully background generation, waveform row badges, manual regeneration, and preview loudness consistency.
179. Add audition auto-advance.
180. Expand the implemented role-aware audition phrases into editable preview MIDI patterns per category.
181. Add "find similar presets" using metadata and parameter distance.
182. Add preset dependency tracking for samples and wavetables.
183. Add pack artwork or pack color markers only if it does not clutter the UI.
184. Add search tokens for macro assignments, engine type, FX type, sequence type, and sample type.
185. Add construction-kit browser category.

### J. Genre-Specific Factory Content

186. Expand the current UKG Essentials/Basslines presets into a larger pack: more organ bass, Dred/Reese variants, late stabs, bell plucks, vocal chops, and shuffle basses.
187. Expand the first speed-garage factory additions beyond the implemented organ-bass starter with Reese bass, swung bass, offbeat stab, chopped vocal, and noise throw variants.
188. Expand the current Deep House/House Chords presets into a larger pack: dub chord, warm sub, original-source M1-style stab, soft pluck, and pad stab variants.
189. Expand the current Tech House Tools presets into a larger pack: more rubber bass, rolling bass, percussive pluck, stab hit, and noise-riser variants.
190. Expand the current Minimal Tools presets into a larger pack: more click plucks, sine sub pulses, resonant blips, tuned percussion hits, and tiny noise accents.
191. Expand the current Warehouse Techno presets into a larger pack: more drive stabs, FM pings, pulse basses, metallic hits, and distorted drones.
192. Add preset construction kits with suggested BPM, key, sequence, macro intent, and recommended Ableton MIDI clip usage.
193. Add "starter from genre" buttons on HOME or LAB.
194. Add factory macro names and descriptions that explain performance intent.
195. Expand factory safety validation beyond the first finite/non-silent/peak render smoke gate into role loudness targets, stuck notes, mono bass, and guard activity.

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
209. Expand factory preset loudness validation beyond `FactoryPresetRenderAudit` into role targets and pack consistency.
210. Add randomizer loudness validation.
211. Add sample import failure tests.
212. Add file-permission handling for user library saves.
213. Add crash-safe preset writing through temp file and atomic replace. First user/generated preset save path uses `juce::TemporaryFile` before replacing the target preset file.
214. Add CI for CMake build and non-GUI checks.
215. Add signed/notarized macOS release workflow.
216. Add installer or reliable VST3 copy script.
217. Add AU target after VST3 remains stable.
218. Add macOS Intel and Apple Silicon release checks.
219. Add plugin metadata audit: manufacturer, plugin name, category, version, automation names.
220. Add user manual pages for the main workflows.

## Multi-Agent Research Refresh

Two focused research passes were run on 2026-06-28: one for modern synth/source engines and one for club samplers, FX, groove tools, and Ableton workflow. A deeper large-VST refresh was added on 2026-06-29 for Serum 2, NEXUS 5, ZENOLOGY Pro, KORG Collection/Gadget/wavestate/modwave, and adjacent Ableton/Serato/Scaler-style production workflows. The result does not change the whole roadmap, but it clarifies what should happen next.

Top missing product capabilities:

1. House source rack depth: true wavetable playback/import, richer warp modes, focused FM/operator color, deeper drawbar organ controls, original/legal digital-PCM-style chord and piano material, second layer, per-layer mute/solo/blend, and source snippets.
2. Fast visual modulation: drag-to-modulate, route range editing, route curves/slew/invert, source activity meters, and hover route overlays. First right-click assignment pass is implemented for MOD-targetable sliders, rotary/linear modulation feedback now shows source-colored rings/badges while preserving base tooltips, MOD targets update the selected-control route summary on hover, and the macro assignment workflow now has a draggable visual pad backed by matrix slots.
3. Serious UKG slicing: transient/beat/manual slice markers, per-slice playback modes beyond the first stored start/end/pan/probability/nudge/fade, visual overlay, and missing-file stale-audio guard passes, slice-to-sequencer lanes, and full missing-sample relink.
4. Motion lanes: step-LFO/MSEG sources and sequencer lanes for sample slice, filter cutoff, wavetable position, pump depth, delay throw, reverb throw, drive, and macro amount.
5. Club FX quality: multiband drive, mid/side routing, oversampling, gain compensation, external sidechain/pump input, drawable shapers, reverse/dub delay, and granular/time fills. First pump telemetry pass is implemented for live phase/reduction display; external sidechain routing remains open.
6. Preset discovery: background audio previews beyond the first visible-row warmer, waveform badges, smart folders, custom tags, similar-preset search, construction-kit preset type, relationship-aware compatible preset groups, dependency warnings, and pack import/export.
7. Genre generators: UKG vocal chop templates, Dred/Reese and organ-bass starters, tech-house bassline generator, minimal Euclidean/sparse tools, techno feedback/resonator scenes, and house chord/organ stabs.
8. Ableton release polish: validate first-pass MIDI drag export, add MIDI drag import, optional MIDI-output strategy, automation-name audit, save/reopen/freeze/flatten checklist, sample collection/relink checks, sidechain routing tests, signing/notarization, and installer/copy workflow.
9. Deterministic production transforms: "make it more house," "tighten low end," "more groove," "darker dub," "shorter stab," "delay throw variation," and "reset mix-safe" actions built from existing patch, macro, sequencer, FX, and Guard state.

Reference URLs used for this refresh:

- https://xferrecords.com/products/serum-2
- https://static.xferrecords.com/Serum%202%20What%27s%20New.pdf
- https://refx.com/nexus/
- https://refx.com/nexus/manual/
- https://www.arturia.com/products/software-instruments/pigments/overview
- https://kilohearts.com/products/phase_plant
- https://www.minimal.audio/products/current
- https://www.roland.com/us/products/rc_zenology_pro/
- https://u-he.com/products/hive/
- https://u-he.com/products/diva/
- https://www.korg.com/us/products/software/modwave_native/
- https://www.korg.com/us/products/software/wavestate_native/
- https://www.korg.com/us/products/software/korg_collection/
- https://www.korg.com/us/products/software/korg_gadget/
- https://www.ableton.com/en/live-manual/12/midi-tools/
- https://www.ableton.com/en/manual/live-instrument-reference/
- https://www.ableton.com/en/manual/live-audio-effect-reference/
- https://serato.com/sample
- https://scalermusic.com/products/scaler-3/
- https://www.cableguys.com/shaperbox
- https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- https://deviousmachines.com/product/infiltrator/
- https://support.output.com/en/collections/10910224-portal-manual
- https://blog.native-instruments.com/uk-garage-music/
- https://www.musicradar.com/how-to/uk-garage-tutorial
- https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/

## Recommended Build Order

1. Expand rendered preset previews beyond the implemented visible-row warmer and level badges into background generation, waveform row badges, safe regeneration controls, and preview loudness checks.
2. Move house source character deeper: drawbar controls, original/legal digital-PCM-style chord/piano material, rubber/Reese/acid snippets, true wavetable import/playback, richer warp modes, and focused FM/operator color.
3. Add transient/manual sample slicing with per-slice playback modes beyond the first stored start/end/pan/probability/nudge/fade and visual overlay pass, plus slice-to-sequencer lanes.
4. Add deeper sequencer workflow: arranger-style chain editing beyond first-pass `Build 4-Bar Chain` and `Auto` / `2 Bar` / `4 Bar` modes, deeper per-scene automation/rate/follow behavior beyond first-pass live step/control chaining, richer audio/loop drag-out or export staging beyond first-pass MIDI drag-out, deeper slide/glide controls beyond first-pass overlap, deeper condition vocabulary, deeper chord-paint variants beyond the first Chord Stab Paint transform, MIDI import, deeper bassline contour generation beyond the first Shape transform, and richer assignable modulation lanes.
5. Add the house drum/kick-bass construction lane from #84 once the export/drag-out foundation is designed.
6. Define the expansion-ready content pack manifest from #83 before the next large curated factory/content push.
7. Add deterministic house performance transforms for "more house," "tighten low end," "more groove," "darker dub," "shorter stab," "delay throw variation," and "reset mix-safe."
8. Expand the first right-click modulation assignment and visual-feedback passes with drag-from-source assignment, hover route overlays, per-route range editing, curves, and slew.
9. Add multiband drive with oversampling, gain compensation, clearer limiter/clip metering, and bass-safe routing.
10. Add full-browser mode, smart folders, user tag editing, dependency warnings, construction kits, relationship-aware preset groups, and pack import/export.
11. Build expandable FX and Mod focus panels after the Macro, Sample, Source, and SEQ focus overlays, then continue component extraction and screenshot/layout regression.
12. Add Ableton save/reopen/freeze/flatten validation, missing-sample behavior checks, MIDI/audio drag validation, signing/notarization, and installer/copy workflow.

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
- Expansion-ready content packs: #83
- House drum and kick-bass construction lane: #84
- Audio quality and oversampling: #58
- Umbrella deep backlog: #62
