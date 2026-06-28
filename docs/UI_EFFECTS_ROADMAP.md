# Nate VST UI, Modulation, And Effects Roadmap

## Goal

Reduce clutter by showing fewer controls at once, make modulation more intentional, improve knob feel, and reshape the FX page so it can grow beyond the current fixed grid.

This is a planning document. It does not copy UI, DSP, presets, or proprietary behavior from reference products. It turns broad patterns from established synths into a Nate VST-specific implementation path.

Reference products reviewed:

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- u-he Hive: https://u-he.com/products/hive/
- u-he Diva: https://u-he.com/products/diva/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Korg wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- MusicRadar UK garage guide: https://www.musicradar.com/how-to/uk-garage-tutorial

## Framework Decision

Stay on JUCE for the production plugin UI for now.

Do not add iPlug2, HISE, Dear ImGui, React/WebView, or another UI framework in this phase. The plugin already depends on JUCE for the VST3 wrapper, editor lifecycle, APVTS automation, CMake integration, plugin copy step, and pluginval validation path. Adding a second UI framework would increase build complexity, binary/signing risk, and host validation risk without solving the immediate problem.

Instead, improve the internal UI system:

- Build better JUCE components for rack lists, selected-module editors, meters, segmented controls, and modulation indicators.
- Keep parameters in APVTS so Ableton automation and saved sets remain stable.
- Add external libraries only when they solve a specific DSP or rendering problem that JUCE does not handle well.

## Current Problems

### FX Shows Too Much

The FX page now has the right high-level pattern: an Add FX control, a fixed-parameter rack list, and a selected-module editor. The remaining risk is scale. With 15 modules available, a single vertical rack list can still become crowded when many modules are enabled, and selected state can be visually confused with enabled state.

The next version should treat the rack like a compact lane: grouped Add FX menu, compact list rows, visible enabled/bypassed state, selected detail editor, and Guard kept available as the output safety module.

### Modulation Is Too Implicit

The eight performance macros are useful, but their assignments still need clearer editing and destination feedback. A user can hear movement but cannot always quickly tell what is being modulated, what source owns the movement, or how much depth is applied.

The next modulation pass should avoid putting the whole matrix on HOME. HOME should stay fast. The deeper assignment workflow belongs in a focused MOD area.

Implemented since the first pass: MOD now includes a destination inspector and a two-bank eight-slot routing matrix, so route rows remain readable while creating or clearing house/UKG movement destinations. Remaining work is deeper route editing, macro assignment editing, and more destination types.

### Knobs Still Feel Too Small

The current rotary sliders have better drag settings than the first version, but the visual hit area still feels compact. There is no large interaction target, no clear fine-drag affordance, and too many rows depend on identical small knobs.

The issue is not only sensitivity. It is also grouping, knob size, value display, and how much precision each control actually needs.

### Grouping Needs A Stronger Information Architecture

Many panels are grouped by implementation order rather than user task. A producer usually thinks in these groups:

- Source: oscillators, noise, sample.
- Shape: filter, envelope, drive, tone.
- Motion: LFOs, macros, sequencer movement, randomization.
- Space: chorus, delay, reverb, width.
- Utility: gain, guard, mono, output safety.

The UI should use these groupings more consistently.

### Research Gaps To Fill

Modern synths and club tools are converging around a few product expectations that Nate VST only partially covers today:

- Oscillator engines: wavetable position/warp, sample/granular style sources, and stronger FM/phase movement are common in Serum 2, Pigments, Phase Plant, Current, and modwave.
- Character filters: multiple filter slopes, modeled/nonlinear filter drive, and distinct filter flavors are a major part of Pigments, Diva, Hive, and Korg instruments. The first passes are implemented as Clean, Warm, Acid, and Dirty filter-drive modes plus selectable 12 dB/24 dB slope.
- Visible modulation: drag-style routing, modulation rings, animated source feedback, curve LFO/MSEG editing, and assignment summaries are table stakes in modern synth UIs.
- Motion/groove tools: UKG, tech house, minimal, and techno benefit from per-lane swing, probability, step modulation, pump curves, delay throws, and key/scale helpers.
- Sampler depth: vocal chops need slice markers, choke behavior, pitch/formant controls, reverse/stutter variations, and better metadata than a single start/end range.
- Browser workflow: large synths make preset search, tags, categories, favorites, ratings, sort modes, folders, and per-section browsing feel central rather than secondary.
- Timing lock: implemented as a first pass. The internal sequencer follows host play-state and PPQ position so house, techno, and UKG patterns recover cleanly after Ableton loop jumps and transport repositioning. SEQ and FX now expose a compact host-sync status badge for lock/stopped/internal fallback state. Remaining work is deeper per-lane modulation sync and richer phase visualization.
- Audio quality: oscillator and drive stages need bandlimiting/oversampling attention before the synth can compete with polished commercial dance plugins at bright high-note settings. The first oscillator pass is implemented with polyBLEP saw/square generation, an integrated bandlimited triangle, and safer upper-register phase increments.

These are product directions, not framework reasons to leave JUCE. The best next framework work is still custom JUCE components for modulation rings, rack rows, curve editors, waveform/slice views, and browser lists.

## UI Direction

### 1. Show A Focused Module, Not Every Module

For areas that can grow, use this pattern:

- Left side: compact module list or lane.
- Top row: add/select controls.
- Main area: focused editor for the selected module.
- Summary rows: show enabled state, type, and one or two important values.

This should be used first for FX, then later for modulation slots.

### 2. Keep HOME Simple

HOME should not become the whole plugin. It should expose:

- Preset load and save.
- Six fast performance macros plus the full eight-macro bank in MOD.
- Performance controls: Sub, Cutoff, Drive, Output.
- Randomization entry points.
- Audition keyboard.

HOME should not show every effect, every modulation assignment, or every oscillator detail. Oscillator source choices, filter mode, unison detail, and envelope editing belong in focused panels.

### 3. Add A MOD Panel Before Adding More Modulation Controls To HOME

Suggested MOD panel layout:

- Header: active source selector and assignment count.
- Left: modulation sources such as Macro, LFO 1, Mod Env, Velocity, Seq Lane.
- Center: selected source controls.
- Right or bottom: assignment matrix rows.

Initial matrix:

| Slot | Source | Destination | Amount |
| --- | --- | --- | --- |
| 1 | Macro Tone | Filter Cutoff | positive |
| 2 | Macro Dirt | Drive | positive |
| 3 | LFO 1 | Filter Cutoff | bipolar |
| 4 | Mod Env 1 | Osc 2 Level | positive |
| 5-8 | Empty | Empty | 0 |

Do not show every assignment on HOME. HOME can show small modulation indicators on the macro knobs and important destination knobs.

## FX Rack Direction

### First UI Step: Addable Fixed Rack

Use an Add FX drop-down or menu on the FX page. The first implementation should be dynamic in the UI but stable in the host parameter model.

Important reason: Ableton automation and saved Live Sets depend on stable parameter IDs. A truly dynamic rack where users can create unlimited effect instances would require slot-based parameters and careful migration. That is too risky as the next step.

First implementation:

- Keep existing effect parameters stable.
- Implemented: add a visible rack list.
- Implemented: use Add FX to enable and reveal fixed modules.
- Implemented: show one selected effect editor at a time.
- Implemented: keep Guard available as the output safety module.
- Implemented: keep the rack as a single compact lane when many modules are visible and group Add FX by role.
- Implemented: replace text-only rack buttons with a dedicated rack-row component that separates selected state, enabled/bypassed state, module name, summary, and Guard safety status.
- Implemented: Delay now has Sync and Rate controls for 1/4, 1/8, dotted, triplet, and 1/16 club throws.
- Implemented: the rack now stays in one compact readable lane when many modules are active, and Move Up/Down skips hidden disabled modules.

Suggested FX page layout:

```text
FX

[Add FX v] [Selected: Delay] [Move <] [Move >]

Rack:
  1 Tone       On   Tilt -0.1
  2 Drive      On   Amount 0.32
  3 Delay      On   Mix 0.18
  4 Reverb     Off
  Guard        On   Ceiling -0.7 dB

Selected module:
  Delay
  Sync | Rate | Time | Feedback | Mix
```

### Later UI Step: Slot-Based Rack

Only move to true dynamic instances when the fixed rack feels good.

Slot-based design:

- `fx_slot_1_type`
- `fx_slot_1_enabled`
- `fx_slot_1_mix`
- `fx_slot_1_param_1` through `fx_slot_1_param_6`
- Repeat for 6 to 8 slots.

Tradeoff:

- Pro: duplicate modules, reorderable chain, scalable UI.
- Con: less readable automation names, migration complexity, more preset rules.

Do not do this until the fixed rack has been tested in Ableton.

## Effects To Add

### UKG-Specific Priorities

The general club roadmap should bias the next few effects and workflows toward UK garage as well as house/techno:

1. Vocal chop sampler workflow.
   Implemented as the SAMPLE-panel UKG Chop action with short cut ranges, musical pitch offsets, reverse variation, One Shot/Gate mode selection, synced stutter/retrigger controls, and sequencer-template application.
   Pitch-ramped chop movement and short delay throws are now implemented; formant-preserving pitch movement remains a later sampler-engine decision.
2. Pump/Duck.
   Implemented as the Pump module. UKG bass and chords benefit from tempo-locked movement even when the DAW is not routing a real sidechain into the instrument.
3. Width/Mono Bass.
   Implemented as the Width module. Keep sub and Reese fundamentals centered while widening stabs, delays, reverbs, and upper harmonics.
   UKG bass guardrails now force the UKG bass recipe toward mono, restrained detune/spread, protected low-cut, Width mono-bass, Guard safety, and short audible glide.
   `UKG Dred Bass` now extends this into reverse-filter/Reese-style bass movement with a centered sub and slower filter modulation.
4. Organ/Stab recipes.
   Implemented first as randomizer recipes for organ stabs, chord stabs, and bell plucks. Factory preset files and any dedicated organ oscillator mode can come later.
   The first UKG factory pack now includes organ stab, chord stab, bell pluck, late stab, vocal chop starter, and bass presets generated from repo-managed XML definitions.
5. Swing and chop sequencer templates.
   Implemented in the SEQ panel. Templates cover 2-step bass, shuffle bass, organ skank, vocal chop triggers, and late offbeat stabs.
   Selective groove is now implemented with Classic, Selective, UKG Push, and Tight modes plus per-step timing and length lanes shown directly in the grid.

### High Priority

These make the most sense for house, tech house, techno, minimal, and club sound design.

1. Flanger

Good for metallic movement, short stereo motion, and classic electronic sweeps. This can likely reuse JUCE delay/chorus-style DSP patterns with shorter delay times and more feedback.

2. Bitcrusher And Downsample

Good for minimal clicks, rough digital texture, old sampler color, and aggressive techno effects. This is a good early DSP target because it is simple, CPU-light, and easy to randomize safely.

3. Pump / Duck

Implemented as the Pump module. An internal tempo-sync pump gives sidechain-style movement without requiring external sidechain routing. The current version uses synced rate, curve type, depth, shape, phase, host PPQ alignment when available, and a BPM-based fallback. Real external sidechain can come much later.

The first rhythmic-shaper curve pass is implemented with Smooth, Tight, Garage, Stutter, and Gate curves plus a focused FX detail preview that draws the active duck envelope. This does not replace a future drawable curve editor, but it gives house and UKG patches useful production shapes now while keeping the parameter model stable.

4. Stereo Width / Mono Bass

Implemented as the Width module. Useful for club translation. Keep low frequencies centered while allowing upper layers, chorus, delay, and reverb to widen.

5. Three-Band EQ

Implemented as the EQ module. It gives fast low, mid, high, and trim controls for bass cleanup, UKG stab/vocal chop shaping, and quick patch balancing without opening another plugin.

### Medium Priority

6. Tremolo / Auto Pan

Implemented as the Tremolo module. Useful for rhythmic amplitude movement, stereo push-pull, UKG stab/chop animation, minimal blips, techno movement, and house/garage groove detail.

7. Frequency Shifter / Ring Mod

Implemented first as the Ring Mod module. Useful for metallic stabs, weird percussion, UKG chop edge, and techno FX. True frequency shifting remains a later higher-risk DSP pass if Ring Mod does not cover enough of the workflow.

8. Comb / Resonator

Implemented as the Comb module. Useful for plucks, tuned metallic sounds, UKG stab/chop resonance, and short percussive techno/minimal color.

9. Multiband Drive

Useful but higher risk. It needs crossover design, phase considerations, and output safety. Do this after simple drive and Guard feel solid.

10. Compressor / Glue

Useful for polish, but a good compressor takes careful tuning. Start with Pump/Duck and Guard before adding a full compressor.

### Lower Priority

11. Granular Freeze / Texture

Interesting, but it belongs after the sample engine and modulation system are stronger.

12. Convolution / Cabinet

Not urgent for this instrument. It adds asset management and bigger CPU/memory questions.

## Knob And Control Plan

### Interaction

- Implemented: larger drag target per knob cell, not only the knob graphic.
- Prefer vertical drag for rotary controls if horizontal/vertical drag still feels awkward.
- Keep snap-to-click disabled.
- Keep double-click reset.
- Keep value popups.
- Current pass: wheel edits stay disabled on slider controls to avoid accidental parameter jumps while scrolling or sound-designing in Ableton.
- Add clearer fine-control behavior. If JUCE modifier fine-drag is not enough, add explicit small step behavior for high-precision parameters.
- If wheel edits become a problem in longer panels, disable them only where scrolling and knob editing conflict.

### Visual

- Implemented: increased rotary knob visual diameter where space allows.
- Implemented: clearer pointer position, thicker value arc, stronger hover state, and larger value text box.
- Draw a subtle value readout or mini text field in module detail views.
- Use fewer knobs in summary views. Put only core controls in the list, and show full controls in the selected module.

### Control Type Choices

Do not use knobs for every parameter.

- Use switches for enable/bypass.
- Use segmented controls for mode/type choices.
- Use horizontal sliders for ranges where precision matters, such as sample start/end, EQ gain, and delay time.
- Use knobs for compact sound-shaping values such as drive, mix, feedback, width, depth, and macro controls.

## Grouping Plan By Panel

### HOME

Groups:

- Perform: Sub, Cutoff, Drive, Output.
- Macros: Tone, Dirt, Weight, Bounce, Warp, Throw, plus the Motion/Space XY pad.
- Random Lab: recipe, generate, mutate, variation, undo, status.
- Library: preset, favorite, and save/load workflow.

Current HOME implementation now keeps only the fast patch-building controls visible: Sub/Cutoff/Drive/Output, macros, randomization, and preset actions. Oscillator waveform selection, filter mode, mono/unison/glide, resonance, filter envelope, noise level, and full envelope editing stay in focused panels.

HOME should keep moving toward fewer permanent controls and more "jump to panel" style affordances later. The full eight-macro performance bank belongs on MOD; HOME keeps the most performance-critical macro controls plus the Motion/Space XY pad.

### SYNTH

Groups:

- Osc 1 and Osc 2.
- Sub and noise.
- Filter.
- Amp envelope.
- Unison/stereo.

Only one deep group should be expanded at a time in a later layout pass.

### LAB

Groups:

- Recipe and action.
- Locks.
- Bias controls.
- History/undo.

The LAB panel is a good candidate for two-column grouping because it is workflow-based, not signal-chain-based.

### FX

Groups:

- Add FX.
- Rack order.
- Selected module details.
- Output safety.

Do not keep a growing grid of all modules.

### MOD

Groups:

- Sources.
- Selected source controls.
- Assignment matrix.
- Destination preview.

MOD should be a new panel or a replacement for overloading the SYNTH/HOME panels.

## Implementation Phases

### Phase 1: Planning And Issues

- Capture this roadmap.
- Create separate GitHub issues for FX UI, FX DSP candidates, knob ergonomics, and modulation/grouping.

### Phase 2: FX UI Rack Without New DSP

- Implemented: add an Add FX combo/menu.
- Implemented: add a compact rack list.
- Implemented: add selected-effect detail rendering.
- Implemented: keep all existing FX parameters and DSP order stable.
- Current pass: group Add FX by Tone & Drive, Movement, and Space & Utility.
- Current pass: use a two-column compact rack when many modules are visible.
- Current pass: prevent removing Guard from the rack safety endpoint.
- Validate in pluginval and Ableton.

### Phase 3: Knob Ergonomics

- Implemented: adjusted the custom look-and-feel for stronger pointer/value feedback.
- Implemented: increased effective drag target in layout cells.
- Implemented: switched rotary knobs to direct drag, shorter full-range movement, disabled snap-to-click, kept double-click reset, kept value popups, and disabled slider mouse-wheel edits to reduce accidental changes.
- Current pass: shortened rotary drag travel again for common macro, tone, drive, output, and timing controls after the knobs still felt too hard to move.
- Still needs real Ableton testing and further tuning if specific controls feel too fast or too slow.

### Phase 4: Add Low-Risk Effects

Add in this order:

1. Bitcrusher/downsample. Implemented as the Crush module.
2. Flanger. Implemented as the Flanger module.
3. Pump/Duck. Implemented as the Pump module, now with selectable curve types for smooth ducking, tight house pump, garage hold, stutter pulses, gated minimal movement, and a saved Custom curve with eight draggable points.
4. Stereo Width/Mono Bass. Implemented as the Width module.
5. Three-Band EQ. Implemented as the EQ module.
6. Tremolo / Auto Pan. Implemented as the Tremolo module.
7. Frequency Shifter / Ring Mod. Implemented first as the Ring Mod module.
8. Comb / Resonator. Implemented as the Comb module.

Each effect should include:

- Stable parameter IDs.
- Conservative defaults.
- Randomization rules.
- Preset save/load coverage.
- Output safety behavior.
- pluginval validation.

### Phase 5: Modulation Visibility

- Add a MOD panel. Implemented first as a visible macro-routing panel.
- Add LFO 1 and Mod Env 1. Implemented for per-voice synth modulation.
- Add an 8-slot matrix. Implemented for safe synth destinations.
- First modulation-visibility pass implemented as active route rows, matrix headers/status, route tooltips, and draggable LFO curve points.
- Add small modulation indicators to macro and destination knobs. Implemented first as destination rings for the current matrix targets.
- Keep HOME focused on performance, not assignment editing.

### Phase 6: House, UKG, And Modern Synth Feature Tracks

Create and work these as separate feature tracks so the plugin grows coherently:

1. UKG/Bassline bass engine: protected sub layer, character layer, phase/wobble movement, glide, mono-below, and low-end safety readouts.
   First safety readout implemented as the HOME `CLUB` assistant: root Hz, sub-band energy, low-side stereo risk, mono/Width status, and Guard/headroom state.
   Second safety readout implemented as phase/reset and club-range guidance: phase reset status, Mono spread-lock status, low-side protection suggestions, root-range warnings, and Guard/headroom prompts.
2. Vocal chop and stab sampler: slice grid, audition pads, choke behavior, reverse/stutter, delay throws, and formant/time-stretch investigation.
3. Wavetable and character filter pass: wavetable position/warp first, then filter slopes/drive/flavors before deeper spectral or granular work.
   First oscillator-quality pass implemented as polyBLEP saw/square generation, integrated bandlimited triangle playback, and phase-delta limiting while preserving waveform parameters and presets.
   First filter-character pass implemented as a saved Filter Character selector with Clean, Warm, Acid, and Dirty modes, recipe seeds, and factory-preset defaults.
   First filter-slope pass implemented as a saved Filter Slope selector with 12 dB and cascaded 24 dB modes, recipe seeds, factory-preset defaults, and old-preset fallback to 12 dB.
4. MSEG and visual modulation: drawable synced curve, modulation rings, destination highlighting, and generated-route feedback from randomization.
5. Browser and construction-kit workflow: genre/role/BPM/key tags, favorites, ratings, sort modes, category folders, per-section presets, and factory packs for UKG, tech house, minimal, and techno.
6. House performance tools: A/B snapshots, eight consistent performance macros, XY movement, delay throws, mute drops, chord/scale helpers, and low-end metering for club translation.
   First performance slice implemented as HOME A/B snapshots that save with plugin/preset state and recall full patch/sample/sequencer state.
   Second performance slice implemented as a HOME XY pad for one-gesture Motion and Space macro control.
   Third performance slice implemented as one-click FX-panel delay, space, pump, and throw-off commands for arrangement fills and breakdowns.
   Fourth performance slice implemented as hold-to-restore Delay, Space, Pump, and Mute Drop buttons with snapshot/restore cleanup on release.
   Fifth performance slice implemented as the eight-macro bank: Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw, with MOD grouping, modulation-source access, randomizer support, and factory preset values.
   Sixth performance slice implemented as a SEQ `Memory` toggle that turns the selected chord mode and voicing into one-finger played house/UKG stabs.

### Reference-Audit Build Order

Recent reference checks against Serum 2, ZENOLOGY Pro, u-he Hive/Diva/Zebra, and Korg Collection/wavestate/modwave point to the same product direction: fast sound browsing, visible modulation, playable macros/XY, motion sequencing, serious FX, and genre-specific generators.

Low-risk additions from the latest audit: per-control modulation inspector, richer preset metadata/ratings/audition, editable macro assignment lists, FX module presets, and genre-specific init buttons. Larger tracks remain wavetable import/warp, layered partials, motion sequencing lanes, advanced sampler slicing, character filters/drives, FM/operator modes, and vector morphing.

Build the next larger slices in this order:

1. Modulation visibility: destination rings, source activity, assignment summaries, direct curve dragging, and more destinations for sample and FX parameters.
   First control-feel pass implemented as direct rotary dragging, parameter-aware drag ranges, and a larger visible LFO curve editor.
   Second control-feel pass reduced rotary drag travel and disabled wheel adjustment on slider controls to prevent accidental changes during in-session sound design.
   First macro-bank pass implemented as a two-row MOD macro area and expanded modulation source list for the eight performance macros.
   First source-visibility pass implemented as live MOD source rows for LFO 1, Mod Env 1, Velocity, and the eight macros, with active route count and summed depth/polarity.
   First panel-alignment pass realigned MOD paint grouping, compacted SAMPLE source/chop/shape regions, and reduced the FX command/performance header footprint.
   First inspector pass implemented as a MOD destination inspector with focused route summaries, summed depth, drag-to-focus for existing destinations, direct source-to-destination route creation, and a destination clear action.
   First Pump curve-edit pass implemented as an FX-panel custom curve editor with eight saved points, matching DSP interpolation, Bounce-aware preview, and recipe randomization seeds.
   First FX-destination pass implemented as MOD matrix targets for Pump Depth, Delay Mix, Reverb Mix, Width, and FX Drive, with LFO/macro DSP support and modulation-ring feedback on the matching FX controls.
   First sample-destination pass implemented as MOD matrix targets for Sample Start, Mix, Pitch, Ramp, and Stutter, with UKG Chop seeding empty slots for start and pitch/ramp motion.
   Current UI audit pass tightened the FX module-preset header and widened MOD destination boxes so Pump controls and long FX/sample destinations have more room.
   First route-edit pass implemented saved per-slot bypass and compact delete controls so users can A/B motion routes without clearing assignments.
   First curve-preset and ring-badge pass implemented MOD-panel LFO curve presets, a host-aware curve phase cursor, current-value-relative modulation rings, route-count/depth badges, and source tooltips on modulated destination knobs.
2. SEQ musical tools: key/scale quantize, chord modes, arp behavior, octave spread, latch/gate options, and UKG/house/techno templates.
   First SEQ visual pass implemented as beat-group headers, root-aware row labels, anchor-step dots, probability bars, and clearer four-beat dividers.
   First scale-helper pass implemented as saved Major, Minor, Dorian, and Minor Pent quantize modes with grid row highlighting.
   First chord/stab pass implemented as saved Off, 5th, Minor, Minor 7, Major, and Minor 9 sequencer chord modes shared by playback and MIDI export.
   Second chord/stab pass implemented as saved Close, Inv 1, Inv 2, Open, and Drop 2 voicings; scale mode quantizes the step/root before chord quality and voicing are applied.
   Third chord/stab pass implemented as a saved Strum amount that spreads chord note-ons in playback and exported MIDI clips.
   First utility pass implemented as `Rot <` / `Rot >` pattern rotation that preserves each step's note, velocity, probability, timing, and length.
   First variation pass implemented as a SEQ-panel `Vary` action that mutates notes, velocity, probability, timing, length, and ghost steps without replacing the pattern.
   First safety pass implemented as a SEQ-panel `Undo` action that restores the prior generated, varied, template, copy, rotate, or clear state.
   First broader club-template pass implemented as `House Chord`, `Tech Bass`, `Minimal Pluck`, and `Techno Pulse` templates in the SEQ pattern selector.
	   First Ableton handoff pass implemented as SEQ-panel `.mid` export that follows the current pattern's rate, root, octave, gate, swing/groove timing, scale quantization, velocity, and accent settings.
	   First chord-memory pass implemented as a saved default-off SEQ `Memory` toggle that expands live MIDI through the current chord mode and voicing while preserving matched note-offs.
	   First declutter pass moved duplicate quick-template buttons behind the pattern dropdown, grouped random/edit actions, added SEQ control/grid panel framing, and gave the grid more room.
	   First selective-transform pass implemented as SEQ `Shape` tools for Tighten, Straight Anchors, Swung Ghosts, Late Stabs, Vocal Push, and Humanize edits.
	   First transport-feedback pass implemented as a SEQ status badge showing host lock, stopped-host state, or internal fallback tempo.
3. Sample waveform slicer: visible waveform, slice markers, per-slice pitch/reverse/gain/stutter, choke behavior, and sequencer-triggered slice lanes.
   First slice-grid pass implemented as eight SAMPLE-panel pads that map the start/end window to equal phrase sections and audition the selected chop through the sampler.
   Second slice-grid pass implemented as a SAMPLE-panel waveform overview with a draggable start/end region, visible phrase markers, duration/selection readout, and host-automatable start/end writes.
   First click-safety pass implemented as internal zero-cross/low-energy boundary snapping plus adaptive fade-in/fade-out guards for chop playback and stutter restarts.
   First slice-style pass implemented as a saved SAMPLE `Slice Style` selector that lets pads apply Clean, Pitch, Reverse, Stutter, or Garage pitch/reverse/gain/stutter behaviors.
   First modulation pass implemented as global LFO/macro destinations for Sample Start, Mix, Pitch, Ramp, and Stutter.
   `UKG Vocal Chop Starter` now opens with Garage slice style selected in the factory pack.
4. Browser depth: text search, multi-tags, ratings, sort modes, category folders, pack/source filters, smart tags such as `Mono Safe`, `UKG`, `Vocal Chop`, and one-click audition.
   First broader factory-pack pass implemented as house, tech-house, minimal, and techno preset categories with matching Library filters.
   First browser-search pass implemented as token-based Library text search across preset name, category, source, user/factory status, and favorite state.
   First smart-tag pass implemented as generated `preset_tags` metadata plus a Library tag filter for role, safety, FX, sequencing, and genre tags.
   First audition pass implemented as a HOME/LIBRARY `Audition` button that loads the selected preset, plays a short root-note preview, and releases it on the UI timer.
   First rating/organization pass implemented 1-5 star ratings in `Library.xml`, sort modes, rated/star filters, recursive preset scanning, and category subfolder saves for user presets.
   First pack/key/tempo metadata pass implemented author, pack, key, and BPM preset XML fields, LIBRARY save controls, metadata search, pack/BPM filters, and sort modes for pack, BPM, key, and author.
5. Source/tone expansion: one strong wavetable lane first, followed by character filter flavors, slopes, drive, and optional quality/oversampling modes.
   First quality pass implemented in the existing oscillator lane with bandlimited saw/square/triangle playback, keeping the next bigger source work focused on wavetable/warp instead of fixing basic aliasing later.
   First source-warp pass implemented as a saved Osc Warp control with macro and MOD matrix routing for pre-filter harmonic movement on house basses, UKG Dred/Reese patches, stabs, and techno pulses.
   First character pass implemented in the existing filter lane with Clean, Warm, Acid, and Dirty drive behaviors for house basses, UKG stabs, rubber tech-house lines, and dirty techno tones.
   First slope pass implemented in the existing filter lane with 12 dB for open stabs/plucks and 24 dB for tighter bass, acid, dred, and warehouse filtering. Remaining work is more modeled filter types, richer routing, and drive oversampling.
6. FX performance workflow: tempo delay divisions, module presets, reorder/duplicate, send-style throws, and direct modulation access to FX mix/depth parameters.
   First routing pass implemented as saved FX chain order slots, rack order badges, Up/Down/Reset controls, and DSP processing through the selected order while keeping Guard last.
   First throw pass implemented as FX-panel `Delay Throw`, `Space Throw`, `Pump Drop`, and `Throw Off` actions that write Delay, Reverb, Pump, Width, and Guard settings without new parameter IDs.
   First momentary pass implemented as hold buttons that snapshot Delay, Reverb, Pump, Width, Guard, and output gain, then restore those values on release.
   First FX declutter pass combined the throw and momentary actions into a single performance row under the add/reorder strip.
	   First module-preset pass implemented as a selected-module `Module Preset` menu with focused house/UKG settings for every FX module while writing only existing automatable parameters.
	   First direct-modulation pass implemented as matrix destinations for Pump Depth, Delay Mix, Reverb Mix, Width, and FX Drive.
	   Current rack review pass restores two-column layout for high module counts, makes Guard safety state distinct from bypass state, and prevents Up/Down controls from enabling when no visible move target exists.
	   First tempo-status pass adds the same host-sync badge used by SEQ so Pump, Tremolo, and synced Delay can be checked while editing the rack.
7. Smart mutation workflow: mutation strengths, section-scoped mutation, visible undo/redo history, variation comparison, and save-as-preset handoff.
   First history pass implemented as one-slot global randomization Undo/Redo with visible `Undo:` and `Redo:` labels in HOME/LAB status.
   First mutation-strength pass implemented as `Vary`, `Mutate`, and `Wild` actions that share the recipe engine, respect locks, and participate in Undo/Redo history.
   First section-scope pass implemented as a HOME/LAB `Scope` selector for All, Source, Env, Filter, Sample, FX, Seq, and Macros, restoring unselected sections plus hidden modulation/output state after recipe generation.
   First generated-save handoff pass implemented as LAB section-roll buttons, generated preset draft naming, recipe-aware category/pack/BPM suggestions, direct LAB save controls, normalized category folder metadata, temporary-file preset writes, and a `PresetSaveAudit` CTest.
   First candidate-history pass implemented as four processor-backed LAB slots that capture generated patch/sample/sequencer snapshots, recall them with UI refresh, expose compact action/recipe/scope labels, and verify capture/recall through `RandomCandidateAudit`.
   First generated-library pass implemented generated-source preset metadata, Generated/Random Lab tags, source-recipe metadata, a Library `Generated` filter, and PresetSaveAudit coverage for generated library scanning.
   First candidate-compare pass implemented trait summaries for random candidates, LAB `To A`/`To B` promotion to performance snapshots, and RandomCandidateAudit coverage for compare and promotion recall.
   First candidate-cue pass implemented per-slot LAB `Cue` buttons that preview generated candidates, restore the current patch afterward, and verify non-destructive preview in `RandomCandidateAudit`.
   First candidate-save pass implemented LAB `Save Slot`, processor-level candidate snapshot preset writes, generated metadata/category folder preservation, and RandomCandidateAudit coverage that saving a candidate leaves the current patch unchanged.
   First candidate-diff pass implemented changed-section count badges, recall/cue tooltip section summaries, old/new value diffs for key controls, and RandomCandidateAudit coverage for the Filter/Cutoff diff path.
   First candidate-organization pass implemented LAB `Save Slot` favorite/rating controls that persist into Library filters and RandomCandidateAudit coverage for the stored candidate name.
   First focus-page pass implemented LAB `Generate`, `Mutate`, `Recipe`, `History`, and `Save` pages, with EditorLayoutAudit coverage for every Random Lab page.
   First recipe-guide pass implemented selected-recipe genre, tempo, good-for, and generator-bias summaries on the LAB Recipe page.
   First section-strength pass implemented per-section Source, Env, Filter, Sample, FX, Seq, and Macro generation strength controls on LAB Generate/Mutate, with RandomCandidateAudit coverage for zero-strength Filter protection.
   First history-detail pass implemented a visible LAB History active-candidate panel for slot label, changed sections, trait comparison, and old/new value diff summary.
   First generated-notes pass implemented searchable `preset_notes` for generated preset saves with recipe, source/category/pack, scope, locks, candidate sections, traits, diffs, use hints, and save time.

8. Modulation visibility workflow: rings, route badges, source activity meters, drag assignment, hover inspection, macro assignment, and route processors.
   First source-meter pass implemented compact MOD source activity meters for LFO 1, Mod Env, Velocity, eight macros, S&H, Smooth, Chaos, and LFO 2 without adding new host parameters.

## Acceptance Criteria For The Next Big UI Pass

- FX page no longer overflows when many effect modules are visible.
- Add FX workflow can reveal existing modules without changing parameter IDs and is grouped by production task.
- Current modules can be selected and edited in a focused detail area.
- Rack selected state, enabled state, bypass state, and Guard safety state are visually distinct in the custom rack-row component.
- Remaining UI polish: direct grid gestures now edit velocity, probability, timing, and length lanes; dense knob rows still need a larger-control design pass rather than only shorter drag travel.
- Knobs feel easier to grab and read. Current pass uses velocity-aware rotary behavior and larger full-range drag than the previous very short drag.
- HOME feels less like a control dump. Current pass limits HOME to Perform, Macros, Random Lab, and Library while keeping deeper controls on focused panels.
- HOME macro controls stay readable as a two-row performance bank beside the Motion/Space XY pad instead of a single compressed six-knob strip.
- A clear path exists for adding more FX without redesigning the page again.
