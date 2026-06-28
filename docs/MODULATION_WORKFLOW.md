# Nate VST Modulation And Performance Workflow

## Goal

Nate VST should build toward a modulation workflow that is fast enough for Ableton production, visible enough to understand while designing sounds, and small enough to validate safely in pluginval and Ableton.

The direction is inspired by broad patterns from larger synths:

- Serum-style visible assignments and immediate sound-design feedback.
- Pigments-style separation between sound engines, modulation, sequencing, and effects.
- u-he-style depth through a matrix without slowing down the first-page workflow.
- KORG-style motion and performance control for rhythmic sound changes.
- ZENOLOGY-style performance controls that make patches playable without deep editing.

This document is a planning artifact. It does not copy UI, presets, wavetables, DSP, or proprietary behavior from those products.

Reference pages checked during the editable-matrix pass:

- Serum 2: https://xferrecords.com/products/serum-2
- Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- u-he Hive 2: https://u-he.com/products/hive/
- Korg modwave native: https://www.korg.com/us/products/software/modwave_native/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/

## Principles

- Keep HOME focused on playability: macros, source mix, randomization, and preset recall.
- Keep SYNTH focused on oscillator, filter, amp, and core tone shaping.
- Put deeper assignment work in a future MOD panel instead of crowding HOME.
- Make the first modulation implementation small, automatable, and easy to validate.
- Prefer visible movement indicators over hidden behavior.
- Keep stable parameter IDs; changing them later can break Ableton sets and user presets.

## First Modulation Sources

Start with a small set:

- `lfo1`: tempo-syncable low-frequency oscillator.
- `mod_env1`: assignable envelope separate from amp envelope.
- `velocity`: note velocity as a performance source.
- `macro_1` through `macro_8`: Ableton-friendly performance controls.
- `seq_lane_1`: a later per-step modulation lane tied to the existing sequencer.

Avoid adding every possible source at once. More sources can follow after the matrix and UI prove stable.

## First Destinations

Good first destinations:

- `filter_cutoff`
- `filter_resonance`
- `filter_env_amount`
- `drive_amount`
- `osc2_tune`
- `osc2_level`
- `sub_level`
- `noise_level`
- `osc_warp`
- `sample_start`
- `sample_mix`
- Sample Pitch, Ramp, and Stutter
- FX Pump Depth, Delay Mix, Reverb Mix, Width, and Drive

Avoid modulating parameters that can easily create unstable or silent patches until safety rules exist.

## Macro Controls

The first macro pass started with four HOME controls; the current implementation uses an eight-macro performance bank across HOME and MOD:

- `Macro 1: Tone`
- `Macro 2: Dirt`
- `Macro 3: Motion`
- `Macro 4: Space`
- `Macro 5: Weight`
- `Macro 6: Bounce`
- `Macro 7: Warp`
- `Macro 8: Throw`

Initial macro behavior should be explicit and simple:

- Macro values are automatable parameters.
- Each macro can have up to four internal assignments.
- Each assignment stores source, destination, bipolar amount, and optional curve later.
- Randomization may move macro assignments, but should preserve output safety.
- Warp now affects oscillator bend and Osc Warp source harmonics, so it can push bass and stab character before filter/FX processing.
- Osc Warp is a per-voice synth destination, so LFO 1, Mod Env 1, Velocity, and macro sources can all target it. The FX and sample destinations still use global LFO/macro sources until a proper global Mod Env/Velocity strategy exists.

Possible parameter IDs:

- `macro_1`
- `macro_2`
- `macro_3`
- `macro_4`
- `macro_5`
- `macro_6`
- `macro_7`
- `macro_8`
- `macro_1_name`
- `macro_2_name`
- `macro_3_name`
- `macro_4_name`

Macro names may be preset metadata rather than automatable parameters if host support becomes awkward.

## LFO 1

Suggested first LFO parameters:

- `lfo1_rate`
- `lfo1_sync`
- `lfo1_shape`
- `lfo1_depth`
- `lfo1_phase`
- `lfo1_retrigger`

Initial shapes:

- sine
- triangle
- saw
- square
- random stepped
- editable 8-point curve for pump, wobble, and off-grid filter movement

Start with one LFO before building LFO 2.

## Modulation Matrix

Suggested first matrix shape:

| Slot | Source | Destination | Amount |
| --- | --- | --- | --- |
| 1 | Macro 1 | Filter Cutoff | + |
| 2 | LFO 1 | Filter Cutoff | +/- |
| 3 | Mod Env 1 | Osc 2 Level | + |
| 4 | Velocity | Amp or Filter | + |
| 5-8 | Empty | Empty | 0 |

Start with 8 slots. This is enough to be useful without becoming hard to validate.

## UI Placement

HOME:

- Show the performance-critical macro controls and the Motion/Space XY pad without turning HOME into a full modulation matrix.
- Keep fast patch-shaping and preset recall visible.
- Show a small moving indicator when macro/LFO modulation is active.
- Keep deeper source, routing, and assignment editing in focused panels.

SYNTH:

- Show modulation amount rings around key tone controls. Current implementation covers synth destinations, the first sample chop destinations, and the first FX movement destinations.
- Keep detailed oscillator, unison, and filter controls here.

MOD:

- LFO controls.
- Mod envelope controls.
- Source rows for LFO 1, Mod Env 1, Velocity, and the eight macro sources.
- Matrix rows.
- Assignment amount controls.
- Current implementation includes active/bypassed route rows, per-slot route bypass/delete, route tooltips, source route counts/depth summaries, matrix status text, destination rings, and direct dragging on LFO curve points.

SEQ:

- Keep pitch/gate/probability in the sequencer.
- Add a modulation lane later only after the matrix supports it.

## Safety Rules

- Clamp modulation depth so cutoff, resonance, and drive stay inside safe ranges.
- Smooth destination changes that affect audio.
- Do not allocate or lock on the audio thread.
- Keep output gain safety active after randomization.
- Keep sub content centered, and force spread-safe behavior when mono mode is active.
- Avoid random assignments that produce silent patches unless the user explicitly asks for extreme chaos.

## First Implementation Slice

Implemented first slice:

1. Add the first macro parameters and macro knobs on HOME.
2. Add fixed internal macro assignments for useful first behavior:
   - Tone -> filter cutoff and resonance.
   - Dirt -> drive amount and output compensation.
   - Motion -> filter envelope amount and oscillator 2 detune.
   - Space -> delay/reverb mix.
3. Save macro values in host state and `.natevstpreset` files through APVTS.
4. Validate with build, codesign, pluginval, and Ableton scan.

Visible workflow slice:

1. Added a dedicated MOD tab.
2. Show the current fixed macro sources without crowding HOME.
3. Show the current real macro routing:
   - Tone -> filter cutoff and resonance.
   - Dirt -> drive and output compensation.
   - Motion -> filter envelope amount and oscillator 2 detune.
   - Space -> delay and reverb sends.
   - Weight -> sub and low-end support.
   - Bounce -> pump depth and groove movement.
   - Warp -> oscillator 2 bend and filter edge.
   - Throw -> delay and reverb push.
4. Reuse the same APVTS macro parameters, so values remain saved in Ableton state and `.natevstpreset` files.
5. Keep HOME focused on the first four fast controls and show all eight macro sources in the deeper MOD workflow.

Editable engine slice:

1. Added saved APVTS parameters for LFO 1, Mod Env 1, and eight matrix slots.
2. Added per-voice LFO 1 with free/synced rates, five shapes, phase, depth, and retrigger.
3. Added per-voice Mod Env 1 with ADSR and depth.
4. Added editable matrix rows on the MOD panel.
5. Implemented conservative synth destinations:
   - Filter cutoff.
   - Filter resonance.
   - Filter envelope amount.
   - Drive.
   - Oscillator 2 tune.
   - Oscillator 2 level.
6. Made randomization seed safe modulation movement for bass, UKG stabs, plucks, minimal blips, and noise FX.
7. Added the first MSEG-style slice: `Curve` as an LFO 1 shape, eight saved curve-point parameters, a compact MOD-panel curve display, and randomization/factory seeds for house and UKG bass movement.
8. Added the first FX curve-shaper slice: Pump Custom curve points with a draggable FX-panel editor, matching DSP interpolation, and recipe randomization support.
9. Added the first global FX modulation destinations: Pump Depth, Delay Mix, Reverb Mix, Width, and FX Drive. LFO 1 and macro sources drive these in the effects rack; Mod Env and Velocity remain per-voice synth-only sources.
10. Added the first global sample modulation destinations: Sample Start, Mix, Pitch, Ramp, and Stutter. UKG Chop seeds empty routes for start and pitch/ramp movement when a sample workflow is active.
11. Added saved per-slot route enable flags plus compact bypass/delete controls. Disabled routes keep their source/destination/amount for comparison but no longer affect synth, sample, FX, rings, or depth summaries.
12. Added reusable LFO curve presets, a phase cursor on the curve display, current-value-relative destination rings, and compact route-count/depth badges on modulated knobs.

Remaining modulation work:

- Add a real global Mod Env/Velocity aggregation strategy before allowing those sources to drive sample or FX targets.
- Expand draggable curve editing into per-slot curve modulation and bipolar/unipolar curve modes after the current LFO and Pump editors have been tested in Ableton.
