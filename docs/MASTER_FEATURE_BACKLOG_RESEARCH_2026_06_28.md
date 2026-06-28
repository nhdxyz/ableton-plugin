# Nate VST Master Feature Backlog Research

Research date: 2026-06-28

This is a long-form "what else should we add" backlog for Nate VST. It combines the current repo docs, two parallel research passes, and a fresh review of flagship synths, samplers, groove tools, and club effects. The product goal is not to clone another plugin. The goal is to build a fast, professional instrument for house, UK garage, tech-house, techno, and minimal inside Ableton.

Use these references as product-pattern research only. Do not copy proprietary presets, samples, wavetables, DSP code, names, branding, or exact layouts.

## References Reviewed

Primary synth and hybrid instrument references:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant and https://kilohearts.com/docs/phase_plant
- Minimal Audio Current: https://www.minimal.audio/products/current and https://manual.minimal.audio/current-manual
- u-he Hive 2: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Roland ZENOLOGY / ZEN-Core: https://www.roland.com/us/promos/zenology/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native / wavestate: https://www.korg.com/us/products/synthesizers/wavestate/
- Ableton Live 12 instruments and MIDI tools: https://www.ableton.com/en/manual/live-instrument-reference/ and https://www.ableton.com/en/live-manual/12/midi-tools/
- Ableton Roar and Meld references: https://www.ableton.com/en/live/all-new-features/ and https://www.ableton.com/en/packs/meld/

Primary FX, sampler, and motion references:

- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- Cableguys VolumeShaper / TimeShaper patterns: https://www.cableguys.com/volumeshaper and https://www.cableguys.com/timeshaper
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- FabFilter Saturn 2 help: https://www.fabfilter.com/help/saturn/using/bandcontrols
- Devious Machines Infiltrator: https://deviousmachines.com/product/infiltrator/
- Output Portal: https://output.com/products/portal
- Ableton Simpler slicing reference: https://www.ableton.com/en/manual/live-instrument-reference/

UK garage and club-production references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar / Silva Bumpa UKG workflow discussion: https://www.musicradar.com/music-tech/sometimes-swinging-all-the-elements-of-the-drums-doesnt-make-it-groove-better-breakout-producer-silva-bumpa-on-the-secret-to-creating-sub-bass-and-ukg-rhythms
- Attack Magazine UK garage beat reference: https://www.attackmagazine.com/technique/beat-dissected/uk-garage/
- NITELIFE Audio UK garage vocal cuts: https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/

## High-Level Research Findings

1. Modern flagship synths are hybrid instruments, not just subtractive synths.
   Serum 2, Pigments, Current, Phase Plant, Zenology, modwave, wavestate, and Meld all point toward layered source engines: wavetable, sample, granular, spectral/modal/physical, FM, PCM, noise, utility layers, and performance macros.

2. Visual modulation is now a core workflow, not an advanced page.
   Drag assignment, source colors, visible modulation ranges, route lists, activity meters, and editable curves are expected. Nate VST has the modulation skeleton, but still needs faster assignment, rings, source meters, range editing, and route processors.

3. Motion lanes are a major gap.
   Korg motion/wave sequencing, Hive shape sequencing, Infiltrator curve sequencing, ShaperBox curves, and Ableton MIDI tools all show that producers expect lane-based movement. Nate VST needs per-step parameter locks, MSEG, step LFOs, and pattern scenes.

4. Club FX need routing depth and visual timing.
   ShaperBox, Saturn 2, Roar, Infiltrator, and Portal show the missing layer: multiband drive, mid/side processing, oversampling, rhythmic shapers, granular fills, sends, and drawable curves.

5. UKG and house need very specific helpers.
   The strongest genre gaps are swung 2-step groove, selective swing per lane, chopped vocal tools, organ/drawbar bass, Reese/Dred bass, late stabs, short gates, mono-safe subs, and quick delay/reverb throws.

6. The browser and content system should become part of the instrument.
   Preset search is not enough. Mature plugins lean on smart folders, pack organization, audio previews, macro intent, construction kits, and fast preset-first play views.

7. The UI should show less at once.
   Nate VST has many good parts, but it should become more modular: page components, focused panels, collapsible groups, selected-control inspector, signal-flow view, and dedicated performance/browser views.

## Current Nate VST Baseline

Based on the local docs and current repo direction, Nate VST already has:

- JUCE/CMake/VST3 foundation for macOS and Ableton.
- HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, and LIBRARY panels.
- Subtractive synth core with oscillator mix, sub, noise, filter, drive, envelopes, unison, Osc Warp, and output guard.
- Genre-oriented randomization, A/B snapshots, macros, XY pad, and low-end guidance.
- Modulation matrix, LFO/curve tools, macro assignment editor, destination badges, S&H/Smooth/Chaos/LFO 2 movement sources, and a first right-click assignment pass for MOD-targetable controls.
- Sample waveform area, phrase/chop controls, slice pads, Slice Keys, per-slice pitch/gain/reverse/stutter/choke memory, and UKG chop helpers.
- Piano-roll style sequencer with scale/chord helpers, velocity/probability/timing/length lanes, groove templates, host sync, undo, rotate, variation, and MIDI export.
- Addable/reorderable FX rack with delay, reverb, drive, pump, tone/EQ, width, guard, modulation FX, throws, and module presets.
- Preset library with recursive folders, categories, favorites, ratings, metadata, search, sort, filters, macro previews, compact browser rows, and audition.
- Starter factory presets for UKG, house, tech-house, techno, and minimal.
- First editor layout audit test.

The gap is now professional depth, sound quality, focus, and genre workflow.

## Top 30 Product Gaps

1. True wavetable oscillator with table position modulation.
2. Wavetable display, playhead, browser, and safe import.
3. Second synth layer with independent source, filter, amp, pan, and routing.
4. Source rack with mute/solo/blend/copy and source snippets.
5. Drawbar/organ source for UKG and speed garage.
6. Original/legal PCM-style house organ, piano, string, and stab sources.
7. Focused FM engine for bells, pings, metallic stabs, and techno hits.
8. Better Reese/Dred bass builder with mono-safe sub and moving upper harmonics.
9. Drag-to-modulate assignment from source chips to controls.
10. Modulation rings, source activity meters, and hover route overlays.
11. Per-route range, curve, smoothing, invert, and polarity editing.
12. True MSEG/multi-point envelope source.
13. Step LFO with hold, probability, slew, and per-step shapes.
14. Pattern scenes and pattern chaining in the sequencer.
15. Per-step modulation lanes for cutoff, drive, warp, sample slice, pump, delay throw, and reverb throw.
16. Per-step ratchets, slide, accent, conditions, and lane-specific groove.
17. Direct MIDI drag export/import with Ableton-friendly behavior.
18. Drag-and-drop WAV/AIFF import onto SAMPLE.
19. Transient, beat-grid, equal-region, and manual slicing.
20. Per-slice start/end/nudge/pan/fade/probability/playback mode.
21. Time-stretch/warp and formant-safe pitch investigation for vocal chops.
22. Multiband distortion with gain compensation and oversampling.
23. Drawable volume/filter/time/noise shaper modules.
24. Internal send bus and parallel wet/dry routing for delay/reverb throws.
25. Granular FX/sampler mode for vocal fills and transitions.
26. Full browser mode with smart folders, tags, audio previews, and construction kits.
27. Find-similar preset search based on metadata and parameter distance.
28. UI scaling, compact mode, component extraction, and screenshot regression.
29. Automated sound-quality tests for aliasing, smoothing, denormals, gain staging, and stuck notes.
30. Ableton release checklist: scan, automate, save, reopen, freeze, flatten, duplicate, and export.

## Priority 0: UI, Workflow, And Foundation

These should stay near the top because every future feature becomes harder if the UI remains one large editor file.

1. Split `PluginEditor` into dedicated page components: HOME, SYNTH, LAB, MOD, SAMPLE, SEQ, FX, LIBRARY, top bar, and keyboard.
2. Split shared UI primitives into reusable components: knob group, section header, value inspector, rack row, browser row, route badge, and lane editor.
3. Add richer automated overlap checks beyond empty-bounds/editor-overflow checks.
4. Add screenshot regression captures for every panel.
5. Add a debug bounds overlay for manual layout inspection.
6. Add UI scale modes: 85%, 100%, 125%, and 150%.
7. Add compact Ableton laptop mode.
8. Add large sound-design mode.
9. Add collapsible groups for dense pages.
10. Add page sub-tabs where pages have too much content.
11. Add a selected-control inspector strip.
12. Show value, default, automation name, modulation routes, and reset action in the inspector.
13. Add value popovers while dragging.
14. Add consistent double-click reset coverage.
15. Add consistent fine-drag coverage.
16. Increase invisible hit areas on small knobs.
17. Tune rotary drag speed by control type.
18. Keep mousewheel/trackpad editing opt-in or very gentle.
19. Add hover, active, disabled, armed, bypassed, and warning states.
20. Replace standard action text buttons with icons where clear: save, favorite, audition, refresh, copy, undo, redo, reset, move up, move down.
21. Add tooltips that describe the musical effect of controls.
22. Add top-bar CPU estimate, voice count, output peak, and guard activity.
23. Add global panic/all-notes-off.
24. Add signal-flow strip: Source -> Sample -> Filter -> FX -> Guard -> Output.
25. Add routing visualization for insert, parallel, send, throw, and guard paths.
26. Add page-specific focus mode so only the active module expands.
27. Add a performance-only page with macros, XY, throws, and safe output.
28. Add a full-browser page with dense sorting/filtering.
29. Add a modern HOME redesign around preset, source, macros, randomize, and output safety.
30. Add visual domain colors for source, modulation, sequencer, sample, FX, library, and safety.

## Priority 1: Source Engine Architecture

1. Add a source rack abstraction.
2. Add Source A and Source B lanes.
3. Add per-source mute, solo, copy, paste, reset, random lock, and output trim.
4. Add per-source pan and stereo width.
5. Add per-source transpose, fine tune, drift, and phase controls.
6. Add per-source routing to Filter A, Filter B, both filters, direct FX, or sample blend.
7. Add source snippets: Sub Bass, Reese, Organ Bass, Chord Stab, Minimal Pluck, Techno Hit, Vocal Chop.
8. Add layer blend macros for Weight, Width, Dirt, Motion, and Bounce.
9. Add layer randomization locks.
10. Add voice allocation controls per layer.
11. Add mono/legato layer mode for bass.
12. Add poly layer mode for stabs and chords.
13. Add source-level preset save/load.
14. Add source compare/revert.
15. Add per-source meters.
16. Add per-source visualizer: waveform/wavetable/sample/organ partial display.
17. Add source dependency tracking for samples and wavetables.
18. Add safe source migration for future preset format changes.
19. Add CPU-aware source quality modes.
20. Add offline/high-quality render source mode.

## Priority 2: Wavetable And Hybrid Synth Engines

1. Add true wavetable oscillator playback.
2. Add wavetable position modulation.
3. Add wavetable display with current frame and playhead.
4. Add wavetable browser.
5. Add factory wavetable folder support.
6. Add safe user wavetable folder support.
7. Add WAV wavetable import after factory playback is stable.
8. Add import validation: frame length, sample rate handling, mono/stereo handling, silence check.
9. Add interpolation quality choices.
10. Add anti-aliasing tests for high notes.
11. Add wavetable warp modes: bend, fold, mirror, sync-style, phase skew, spectral tilt, formant tilt, quantize, bit shape.
12. Add dual warp slots per oscillator.
13. Add oscillator hard sync.
14. Add PWM for pulse sounds.
15. Add ring modulation.
16. Add protected FM/PM between oscillators.
17. Add audio-rate modulation for selected oscillator parameters.
18. Add focused 3-operator FM engine.
19. Add additive/partial engine for hollow bells and metallic techno hits.
20. Add drawbar organ engine for UKG, speed garage, and house organs.
21. Add original/legal sampled PCM source for house organ and piano-style stabs.
22. Add resonator/physical-style source for minimal plucks.
23. Add noise engine with white, pink, brown, air, vinyl, metallic, digital hash, attack tick, and rumble.
24. Add supersaw/unison engine improvements: detune curve, phase spread, stereo spread, mono-safe spread.
25. Add analog drift: pitch, phase, pan, filter, envelope, and voice variance.
26. Add chord stack and octave stack source modes.
27. Add MPE-ready note expression routing later.
28. Add microtuning support later if scope stays manageable.
29. Add waveform import only after licensing and safety assumptions are clean.
30. Add source engine stress tests.

## Priority 3: House, UKG, Tech-House, Techno, And Minimal Sound Builders

1. Add UKG Dred Bass builder.
2. Add Reese Bass builder with mono-safe sub and stereo upper layer.
3. Add Organ Bass builder.
4. Add Speed Garage organ wobble recipe.
5. Add House Piano Stab builder using original/legal sources.
6. Add UKG Late Stab builder.
7. Add Dub Techno Chord builder.
8. Add Tech-House Rubber Bass builder.
9. Add Minimal Click Pluck builder.
10. Add Warehouse Techno Hit builder.
11. Add Bell Pluck builder.
12. Add Rave Stab builder.
13. Add Sub Chug builder.
14. Add Skippy Bassline builder.
15. Add Wobble Bass macro recipe.
16. Add Vocal Chop Starter recipe.
17. Add Reverse Pickup Chop recipe.
18. Add Stutter Tag Chop recipe.
19. Add Call/Response Vocal recipe.
20. Add Deep House Chord Memory recipe.
21. Add Tech-House Roll pattern generator.
22. Add Minimal Sparse pattern generator.
23. Add Warehouse Straight pattern generator.
24. Add Broken Percussion pattern generator.
25. Add Dub Delay Stab FX recipe.
26. Add Pumped Organ Sidechain recipe.
27. Add Dirty Top Bass recipe.
28. Add Clean Mono Sub recipe.
29. Add Late Ghost Stab timing recipe.
30. Add one-click "make more UKG", "make more house", "make more minimal", and "make more warehouse" transforms.

## Priority 4: Filters, Drive, Tone, And Low-End

1. Add dual filters.
2. Add serial, parallel, split, and per-layer filter routing.
3. Add filter blend/morph.
4. Add notch filter.
5. Add comb filter.
6. Add formant/vowel filter.
7. Add all-pass filter.
8. Add morphing multimode filter.
9. Add clean digital filter character.
10. Add ladder-style filter character.
11. Add SEM-style state-variable character.
12. Add MS-style aggressive character.
13. Add diode/acid character.
14. Add filter keytracking with slope and center note.
15. Add protected filter FM.
16. Add pre/post filter drive switch.
17. Add per-filter saturation color.
18. Add bass-safe high-pass utility before heavy drive.
19. Add bass-safe high-pass utility after heavy drive.
20. Add transient-preserving drive mode.
21. Add soft clip, hard clip, diode, tube, tape, transformer, foldback, wavefold, fuzz, and digital degrade models.
22. Add drive gain compensation.
23. Add loudness-matched drive audition.
24. Add oversampling for filter and drive nonlinearity.
25. Add per-preset quality mode: Eco, Normal, High, Render.
26. Add club low-end monitor.
27. Warn on stereo bass below a configurable frequency.
28. Warn on DC offset.
29. Warn on repeated output guard hits.
30. Add one-knob Low-End Tighten and Stab Bite transforms.

## Priority 5: Modulation, Movement, And Performance

1. Add drag-to-modulate from source chips.
2. Expand right-click modulation menus to every automatable control.
3. Add visible modulation rings around modulated knobs.
4. Add source-colored modulation badges.
5. Add route amount indicators on badges.
6. Add hover-to-show routes for the parameter under the mouse.
7. Add selected-parameter route inspector.
8. Add global show-all-routes overlay.
9. Add source activity meters.
10. Add per-route min/max range editing.
11. Add per-route curve editing.
12. Add per-route smoothing and slew.
13. Add per-route unipolar/bipolar mode.
14. Add per-route invert.
15. Add per-route quantize.
16. Add per-route rectify.
17. Add per-route clamp.
18. Add per-route scale and offset.
19. Add per-route sample/hold.
20. Add per-route randomize.
21. Add per-route velocity scaling.
22. Add per-route keytracking scaling.
23. Add route mute, solo, duplicate, copy, and paste.
24. Add LFO 3.
25. Add true MSEG source.
26. Add multi-point curve editor with add/delete/move points.
27. Add curve snapping, tension, loop, one-shot, and tempo sync.
28. Add step LFO with hold, slew, probability, and shape per step.
29. Add envelope follower source for a future audio-effect build.
30. Add mod wheel, aftertouch, MPE pressure, and MPE slide sources.
31. Add editable macro names per preset.
32. Add macro snapshots A/B/C/D.
33. Add macro morphing between snapshots.
34. Add XY pad assignment editor.
35. Add XY gesture recording and playback.
36. Add performance macro lock states.
37. Add macro randomization that respects assigned destinations.
38. Add macro preview values in the full browser.
39. Add modulation assignment browser by destination.
40. Add automation-safe parameter naming audit before adding many new destinations.

## Priority 6: Sequencer, Piano Roll, And Groove

1. Add multiple sequencer patterns per preset.
2. Add pattern scenes: A, B, Fill, Drop, Breakdown, Transition.
3. Add pattern chaining.
4. Add scene trigger buttons.
5. Add per-step ratchet/retrigger.
6. Add per-step slide/glide.
7. Add per-step accent.
8. Tie accent to amp, filter, and drive.
9. Add per-step modulation lanes.
10. Add per-step sample slice lane.
11. Add per-step pump depth lane.
12. Add per-step delay throw lane.
13. Add per-step reverb throw lane.
14. Add per-step drive lane.
15. Add per-step wavetable position lane.
16. Add per-step filter cutoff lane.
17. Add per-step macro amount lane.
18. Add probability ranges.
19. Add step conditions: first, not first, fill, every 2, every 4, random.
20. Add scale quantize in the piano roll.
21. Add key lock in the piano roll.
22. Add chord paint tools: minor 7, minor 9, major 7, sus, garage organ, dub chord, rave stab.
23. Add voicing spread.
24. Add inversion controls.
25. Add strum direction and amount per chord.
26. Add humanize with timing, velocity, gate, and pitch.
27. Add lane-specific groove amount.
28. Add selective swing per lane.
29. Add swing microtiming visualization.
30. Add UKG 2-step groove template.
31. Add speed garage groove template.
32. Add deep house shuffle groove template.
33. Add tech-house roll groove template.
34. Add minimal offbeat groove template.
35. Add dub techno pulse groove template.
36. Add warehouse straight groove template.
37. Add broken percussion groove template.
38. Add Euclidean generator for minimal blips and percussion-like synth hits.
39. Add bassline contour generator.
40. Add recombine tool: keep rhythm, change notes.
41. Add density control for generated patterns.
42. Add range control for generated basslines.
43. Add call/response generator.
44. Add direct MIDI drag export into Ableton.
45. Add MIDI drag import from dropped MIDI files or clips.
46. Add sequencer preset snippets.
47. Add pattern undo/redo history.
48. Add host transport jump stress tests.
49. Add export validation that exported MIDI matches internal playback.
50. Add pattern preview in the library.

## Priority 7: Sampler, Chops, And UKG Vocal Tools

1. Add drag-and-drop WAV/AIFF import onto the waveform display.
2. Add sample replace by dropping from Finder or Ableton exports.
3. Add missing-file detection.
4. Add relink workflow for missing samples.
5. Add transient-based slice detection.
6. Add beat-grid slicing.
7. Add equal-region slicing with selectable count.
8. Add fully manual slice markers.
9. Add marker add, remove, drag, and snap.
10. Add zero-cross snap.
11. Add per-slice start.
12. Add per-slice end.
13. Add per-slice nudge.
14. Add per-slice pitch.
15. Add per-slice gain.
16. Add per-slice pan.
17. Add per-slice reverse.
18. Add per-slice fade in/out.
19. Add per-slice choke group.
20. Add per-slice probability.
21. Add per-slice stutter/retrigger.
22. Add per-slice FX send.
23. Add per-slice one-shot mode.
24. Add per-slice gate mode.
25. Add per-slice loop mode.
26. Add per-slice thru mode.
27. Add slice audition on hover/click.
28. Add slice keyboard mapping display.
29. Add slice-to-sequencer lane.
30. Add slice randomizer with density and repetition guardrails.
31. Add silence-aware slice randomization.
32. Add click-safe fades.
33. Add sample key detection.
34. Add sample BPM detection.
35. Add short warp/time-stretch mode.
36. Add formant-safe pitch investigation for vocal chops.
37. Add granular sampler mode for vocal clouds and fills.
38. Add tape-stop and start-rate modulation.
39. Add Akai-style 12-bit color.
40. Add sampler-rate color.
41. Add garage chop templates: call/response, late swing, reverse pickup, stutter tag, sparse hook, pitch-ramp answer.
42. Add drag audio into plugin and auto-create chop preset.
43. Add resample current synth patch into sampler.
44. Add render internal sequence to audio.
45. Add sample pack browser inside LIBRARY.
46. Add recently used sample folders.
47. Add favorite sample folders.
48. Add per-sample notes.
49. Add sample dependency tracking in preset files.
50. Add non-destructive crop/export helpers.

## Priority 8: FX, Routing, And Club Processing

1. Add true slot-based FX rack with duplicate effect instances.
2. Add parallel FX routing.
3. Add wet/dry bus routing.
4. Add internal send bus for delay/reverb throws.
5. Add pre/post source routing for synth and sample paths.
6. Add external sidechain input for real Ableton kick ducking.
7. Add compressor module.
8. Add limiter/clipper module.
9. Add transient shaper.
10. Add noise gate/expander.
11. Add multiband distortion.
12. Add movable crossovers.
13. Add per-band drive.
14. Add per-band dynamics.
15. Add per-band tone.
16. Add per-band feedback.
17. Add per-band mix.
18. Add per-band mute/solo.
19. Add mid/side processing for drive, EQ, and width.
20. Add oversampling for nonlinear FX.
21. Add gain compensation for nonlinear FX.
22. Add drawable volume shaper.
23. Add drawable filter shaper.
24. Add drawable time shaper.
25. Add drawable noise shaper.
26. Add beat repeat/glitch/stutter module.
27. Add rhythmic effect activation lanes.
28. Add frequency shifter.
29. Add tuned resonator bank.
30. Add barber-pole phaser/flanger modes.
31. Add chorus ensemble mode.
32. Add dub delay feedback tone controls.
33. Add delay diffusion mode.
34. Add tape delay color.
35. Add reverse delay.
36. Add delay ducking.
37. Add shimmer/pitch reverb only if it stays useful for club sounds.
38. Add convolution/small IR space only if CPU and licensing are clean.
39. Add granular FX for vocal fills and transitions.
40. Add pitch/formant FX.
41. Add lofi/noise module with wobble, hum, vinyl, cassette, and digital artifacts.
42. Add DJ filter macro.
43. Add one-click breakdown/transition macro routing.
44. Add per-FX module preset save/load.
45. Add whole-rack presets.
46. Add FX chain compare/revert.
47. Add drag-reorder for FX rows.
48. Add FX latency reporting if needed.
49. Add automated gain staging tests for every FX module.
50. Add CPU guardrails for stacked FX.

## Priority 9: Randomization And Sound Discovery

1. Add lock buttons on every parameter group.
2. Add randomization scopes: Source, Filter, Amp, Motion, FX, Seq, Sample, Library, Macros.
3. Add intensity per scope.
4. Add "useful only" randomization mode.
5. Add pitch guardrails.
6. Add output gain guardrails.
7. Add sub-level guardrails.
8. Add resonance guardrails.
9. Add feedback guardrails.
10. Add wet-FX guardrails.
11. Add stereo-bass guardrails.
12. Add silence detection after randomization.
13. Add safety rollback when randomization creates silence or unsafe output.
14. Add randomization history beyond one undo step.
15. Add A/B compare for generated patches.
16. Add mutate/evolve that changes 10-20% while preserving patch identity.
17. Add breed/morph between preset A and preset B.
18. Add transform buttons: darker, brighter, tighter, wider, dirtier, cleaner, bouncier.
19. Add genre transforms: more UKG, more house, more tech-house, more minimal, more warehouse.
20. Add random macro assignment generation.
21. Add random sequence generation that respects genre, key, and scale.
22. Add random slice generation that avoids silence and tiny cuts.
23. Add random FX chain generation with CPU and gain limits.
24. Add random route generation with modulation-depth limits.
25. Add recipe builder for user-defined randomizers.
26. Add favorites-aware generation from highly rated presets.
27. Add automatic preset name suggestions from detected traits.
28. Add randomization explanation text in the save metadata.
29. Add "surprise me, but keep bass safe" mode.
30. Add "make it club-ready" finalization transform.

## Priority 10: Library, Presets, Packs, And Content

1. Add larger full-browser mode.
2. Add folder tree for factory and user libraries.
3. Add smart folders: Favorites, Five Star, Recent, User, Factory, Macro Rich.
4. Add smart folders for UKG Bass, UKG Vocal Chops, House Stabs, Tech-House Bass, Minimal Plucks, Dirty Techno.
5. Add editable custom user tags.
6. Add tag editor for existing presets.
7. Add multi-select metadata editing.
8. Add preset notes.
9. Add preset compare before overwrite.
10. Add preset version history for user saves.
11. Add duplicate preset detection.
12. Add library repair/reindex command.
13. Add missing sample warnings.
14. Add missing wavetable warnings.
15. Add preset dependency list.
16. Add pack manager.
17. Add preset pack import/export.
18. Add pack validation before import.
19. Add pack artwork or pack colors if it does not clutter the UI.
20. Add per-preset audio preview rendering.
21. Add preview MIDI patterns by category.
22. Add audition auto-advance.
23. Add audition level normalization.
24. Add "find similar presets" using metadata and parameter distance.
25. Add search tokens for macro assignments.
26. Add search tokens for engine type.
27. Add search tokens for FX type.
28. Add search tokens for sequence type.
29. Add search tokens for sample type.
30. Add macro-intent metadata: Tone, Dirt, Motion, Space, Weight, Bounce, Warp, Throw.
31. Add construction-kit preset type.
32. Store preset, sequence, macro intent, key, BPM, sample dependencies, and recommended FX in construction kits.
33. Add UKG Essentials construction-kit pack.
34. Add House Essentials construction-kit pack.
35. Add Tech-House Basslines pack.
36. Add Minimal Plucks pack.
37. Add Warehouse Techno pack.
38. Add Dub Techno Chords pack.
39. Add Vocal Chop Starters pack.
40. Add user pack export for collaboration.

## Priority 11: Sound Quality, Safety, And Tests

1. Add oversampling quality modes for drive.
2. Add oversampling quality modes for aggressive filter behavior.
3. Add oversampling quality modes for clipper/limiter.
4. Add high-quality/offline render mode if host context allows it.
5. Add denormal protection audit.
6. Add automation smoothing audit.
7. Add anti-click tests for every parameter likely to move during playback.
8. Add oscillator aliasing render tests.
9. Add wavetable aliasing tests.
10. Add FM aliasing tests.
11. Add filter stability tests.
12. Add drive gain compensation tests.
13. Add loudness-matched drive tests.
14. Add randomizer safety tests.
15. Add factory preset safety validator.
16. Add output guard telemetry tests.
17. Add voice stealing audit.
18. Add stuck-note tests.
19. Add sustain-pedal tests if supported.
20. Add transport jump tests.
21. Add sample import fuzz tests for invalid files.
22. Add preset migration tests.
23. Add library scanner tests.
24. Add UI layout tests for every page.
25. Add screenshot regression tests.
26. Add pluginval run in local release script.
27. Add CI build for macOS.
28. Add CI unit tests.
29. Add CI artifact packaging.
30. Add Ableton manual checklist for every release.

## Priority 12: Ableton, Distribution, And Release

1. Add AU target after VST3 is stable.
2. Add macOS signing workflow.
3. Add notarization workflow.
4. Add installer or scripted VST3 copy workflow.
5. Add version stamping.
6. Add release notes generator.
7. Add changelog discipline by feature slice.
8. Add host automation name audit.
9. Add parameter migration strategy.
10. Add preset migration strategy.
11. Add Ableton scan checklist.
12. Add Ableton load checklist.
13. Add Ableton automation checklist.
14. Add Ableton save/reopen checklist.
15. Add Ableton freeze/flatten checklist.
16. Add Ableton duplicate-track checklist.
17. Add Ableton MIDI drag checklist.
18. Add Ableton audio drag checklist.
19. Add sample relink checklist.
20. Add user manual.
21. Add quick-start guide for UKG bass.
22. Add quick-start guide for house chord stabs.
23. Add quick-start guide for tech-house rolling bass.
24. Add quick-start guide for minimal plucks.
25. Add quick-start guide for techno stabs/drones.
26. Add factory preset authoring guide.
27. Add factory pack QA guide.
28. Add issue templates for feature slices, bugs, and Ableton validation.
29. Add release checklist issue template.
30. Add license and third-party asset audit.

## What We Should Not Prioritize Yet

1. Full spectral oscillator cloning before wavetable/source rack is solid.
2. Cloud preset marketplace before local library, tags, packs, and previews are strong.
3. Stem-aware separation before normal sample import/slicing/relink works.
4. Huge UI redesign without component extraction and screenshot regression.
5. More generic knobs before focus views and selected-control inspector.
6. Heavy AI generation before deterministic randomization is useful and safe.
7. Exact vintage instrument emulation claims unless the DSP is actually modeled and legally positioned.
8. Proprietary sample/wavetable compatibility unless licensing and format assumptions are clean.
9. AU/notarized release before VST3 preset and parameter stability improves.
10. Deep MPE/microtuning before core house/UKG workflows are fast.

## Recommended Build Order

1. Finish the current right-click modulation assignment slice and validate it.
2. Componentize the editor and expand layout/screenshot testing.
3. Add selected-control inspector with route, value, automation name, reset, and modulation summary.
4. Add drag-to-modulate plus modulation rings and hover route overlays.
5. Add per-route range, curve, slew, invert, and polarity editing.
6. Upgrade SAMPLE with drag/drop import, transient/manual slices, and per-slice start/end/nudge/pan/fades/probability.
7. Add sequencer pattern scenes, ratchets, slide/accent, and per-step modulation lanes.
8. Add full-browser mode with smart folders, tags, audio previews, construction kits, and dependency warnings.
9. Add true wavetable oscillator with preview, position modulation, safe factory tables, and simple warp.
10. Add drawbar organ, Reese/Dred builder, and original PCM-style house stab source.
11. Add multiband drive with gain compensation, oversampling, and low-end safety telemetry.
12. Add drawable shaper FX and internal send/parallel routing.
13. Add MSEG and step LFO once modulation assignment/range UX is mature.
14. Add construction-kit packs for UKG, house, tech-house, minimal, and techno.
15. Add CI, release packaging, and the Ableton save/reopen/freeze/flatten checklist.

## Strongest Next Feature Slice

The best next product slice is still the modulation workflow, but it should be tied directly to UI cleanup:

1. Selected-control inspector.
2. Drag-to-modulate source chips.
3. Modulation rings on knobs/sliders.
4. Hover route overlay.
5. Per-route min/max range editing.

Reason: this makes every existing sound source, FX module, sampler control, and sequencer lane feel more professional immediately. It also makes future wavetable, sampler, and multiband FX work easier to control without adding more clutter.

The strongest house/UKG sound slice after that is:

1. Drag/drop sample import.
2. Transient/manual slicing.
3. Per-slice start/end/nudge/pan/fade/probability/playback mode.
4. Slice-to-sequencer lane.
5. UKG chop templates.

Reason: Nate VST already has a sample page and Slice Keys foundation. Turning that into a serious UKG chop workflow creates a clear identity that generic synths do not have.
