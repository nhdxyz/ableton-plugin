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
- `macro_1` through `macro_4`: Ableton-friendly performance controls.
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
- `sample_start`
- `sample_mix`
- FX mix parameters such as delay, reverb, and chorus mix

Avoid modulating parameters that can easily create unstable or silent patches until safety rules exist.

## Macro Controls

Start with four macros on HOME:

- `Macro 1: Tone`
- `Macro 2: Dirt`
- `Macro 3: Motion`
- `Macro 4: Space`

Initial macro behavior should be explicit and simple:

- Macro values are automatable parameters.
- Each macro can have up to four internal assignments.
- Each assignment stores source, destination, bipolar amount, and optional curve later.
- Randomization may move macro assignments, but should preserve output safety.

Possible parameter IDs:

- `macro_1`
- `macro_2`
- `macro_3`
- `macro_4`
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

- Show four macro knobs.
- Keep fast unison width controls available without hiding source mix or preset recall.
- Show a small moving indicator when macro/LFO modulation is active.
- Keep source mix and preset controls visible.

SYNTH:

- Show modulation amount rings around key tone controls later.
- Keep detailed oscillator, unison, and filter controls here.

MOD:

- LFO controls.
- Mod envelope controls.
- Matrix rows.
- Assignment amount controls.

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

1. Add four macro parameters and macro knobs on HOME.
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
4. Reuse the same APVTS macro parameters, so values remain saved in Ableton state and `.natevstpreset` files.

Do not build the editable LFO/mod-envelope matrix until the macro workflow is tested in Ableton and the modulation engine has saved slot parameters, smoothing, and destination safety rules.
