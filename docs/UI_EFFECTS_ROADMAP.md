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

The four macros are useful, but their assignments are mostly invisible. A user can hear movement but cannot quickly tell what is being modulated, what source owns the movement, or how much depth is applied.

The next modulation pass should avoid putting the whole matrix on HOME. HOME should stay fast. The deeper assignment workflow belongs in a focused MOD area.

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
- Character filters: multiple filter slopes, modeled/nonlinear filter drive, and distinct filter flavors are a major part of Pigments, Diva, Hive, and Korg instruments.
- Visible modulation: drag-style routing, modulation rings, animated source feedback, curve LFO/MSEG editing, and assignment summaries are table stakes in modern synth UIs.
- Motion/groove tools: UKG, tech house, minimal, and techno benefit from per-lane swing, probability, step modulation, pump curves, delay throws, and key/scale helpers.
- Sampler depth: vocal chops need slice markers, choke behavior, pitch/formant controls, reverse/stutter variations, and better metadata than a single start/end range.
- Browser workflow: large synths make preset search, tags, categories, favorites, and per-section browsing feel central rather than secondary.

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
- Four macros.
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
- Implemented: compact the list into two columns when many modules are visible and group Add FX by role.
- Implemented: replace text-only rack buttons with a dedicated rack-row component that separates selected state, enabled/bypassed state, module name, summary, and Guard safety status.

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
  Time | Feedback | Mix | Sync later
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
   Selective groove is now implemented with Classic, Selective, UKG Push, and Tight modes plus a per-step timing lane shown directly in the grid.

### High Priority

These make the most sense for house, tech house, techno, minimal, and club sound design.

1. Flanger

Good for metallic movement, short stereo motion, and classic electronic sweeps. This can likely reuse JUCE delay/chorus-style DSP patterns with shorter delay times and more feedback.

2. Bitcrusher And Downsample

Good for minimal clicks, rough digital texture, old sampler color, and aggressive techno effects. This is a good early DSP target because it is simple, CPU-light, and easy to randomize safely.

3. Pump / Duck

Implemented as the Pump module. An internal tempo-sync pump gives sidechain-style movement without requiring external sidechain routing. The first version uses synced rate, depth, shape, phase, host PPQ alignment when available, and a BPM-based fallback. Real external sidechain can come much later.

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
- Add clearer fine-control behavior. If JUCE modifier fine-drag is not enough, add explicit small step behavior for high-precision parameters.
- Consider disabling mouse-wheel changes on knobs to avoid accidental changes while scrolling future panels.

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
- Macros: Tone, Dirt, Motion, Space.
- Random Lab: recipe, generate, mutate, variation, undo, status.
- Library: preset, favorite, and save/load workflow.

Current HOME implementation now keeps only the fast patch-building controls visible: Sub/Cutoff/Drive/Output, macros, randomization, and preset actions. Oscillator waveform selection, filter mode, mono/unison/glide, resonance, filter envelope, noise level, and full envelope editing stay in focused panels.

HOME should keep moving toward fewer permanent controls and more "jump to panel" style affordances later.

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
- Implemented: switched rotary knobs to direct drag, shorter full-range movement, disabled snap-to-click, kept double-click reset, kept value popups, and disabled mouse-wheel edits.
- Still needs real Ableton testing and further tuning if specific controls feel too fast or too slow.

### Phase 4: Add Low-Risk Effects

Add in this order:

1. Bitcrusher/downsample. Implemented as the Crush module.
2. Flanger. Implemented as the Flanger module.
3. Pump/Duck. Implemented as the Pump module.
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
2. Vocal chop and stab sampler: slice grid, audition pads, choke behavior, reverse/stutter, delay throws, and formant/time-stretch investigation.
3. Wavetable and character filter pass: wavetable position/warp first, then filter slopes/drive/flavors before deeper spectral or granular work.
4. MSEG and visual modulation: drawable synced curve, modulation rings, destination highlighting, and generated-route feedback from randomization.
5. Browser and construction-kit workflow: genre/role/BPM/key tags, favorites, per-section presets, and factory packs for UKG, tech house, minimal, and techno.
6. House performance tools: A/B snapshots, eight consistent performance macros, XY movement, delay throws, mute drops, chord/scale helpers, and low-end metering for club translation.
   First performance slice implemented as HOME A/B snapshots that save with plugin/preset state and recall full patch/sample/sequencer state.
   Second performance slice implemented as a HOME XY pad for one-gesture Motion and Space macro control.

### Reference-Audit Build Order

Recent reference checks against Serum 2, ZENOLOGY Pro, u-he Hive/Diva/Zebra, and Korg Collection/wavestate/modwave point to the same product direction: fast sound browsing, visible modulation, playable macros/XY, motion sequencing, serious FX, and genre-specific generators.

Build the next larger slices in this order:

1. Modulation visibility: destination rings, source activity, assignment summaries, direct curve dragging, and more destinations for sample and FX parameters.
2. SEQ musical tools: key/scale quantize, chord modes, arp behavior, octave spread, latch/gate options, and UKG/house/techno templates.
   First SEQ visual pass implemented as beat-group headers, root-aware row labels, anchor-step dots, probability bars, and clearer four-beat dividers.
   First scale-helper pass implemented as saved Major, Minor, Dorian, and Minor Pent quantize modes with grid row highlighting.
3. Sample waveform slicer: visible waveform, slice markers, per-slice pitch/reverse/gain/stutter, choke behavior, and sequencer-triggered slice lanes.
4. Browser depth: text search, multi-tags, pack/source filters, smart tags such as `Mono Safe`, `UKG`, `Vocal Chop`, and one-click audition.
5. Source/tone expansion: one strong wavetable lane first, followed by character filter flavors, slopes, drive, and optional quality/oversampling modes.
6. FX performance workflow: tempo delay divisions, module presets, reorder/duplicate, send-style throws, and direct modulation access to FX mix/depth parameters.

## Acceptance Criteria For The Next Big UI Pass

- FX page no longer overflows when many effect modules are visible.
- Add FX workflow can reveal existing modules without changing parameter IDs and is grouped by production task.
- Current modules can be selected and edited in a focused detail area.
- Rack selected state, enabled state, bypass state, and Guard safety state are visually distinct in the custom rack-row component.
- Knobs feel easier to grab and read. Current pass uses velocity-aware rotary behavior and larger full-range drag than the previous very short drag.
- HOME feels less like a control dump. Current pass limits HOME to Perform, Macros, Random Lab, and Library while keeping deeper controls on focused panels.
- A clear path exists for adding more FX without redesigning the page again.
