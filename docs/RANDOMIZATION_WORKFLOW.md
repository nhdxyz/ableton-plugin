# Nate VST Randomization Workflow

## Goal

The LAB panel should make new patches quickly while letting users protect the parts of a sound that already work.

## Actions

- `Generate` creates a new patch from the selected recipe.
- `Mutate` moves the current patch toward a new recipe result.
- `Vary` makes a smaller move from the current patch.
- `Wild` makes a stronger recipe-aware mutation for bigger idea jumps.
- `Undo` restores the patch state from immediately before the last Generate, Mutate, Vary, or Wild action.
- `Redo` restores the last undone Generate, Mutate, Vary, or Wild result unless a new random action has replaced the redo slot.

## Locks

Locks are applied after the selected recipe runs. This keeps the recipe engine simple while restoring protected sections from the pre-randomization snapshot.

- `Pitch`: oscillator octave/tune, oscillator 2 octave/tune, mono, and glide.
- `Env`: amp attack, decay, sustain, and release.
- `Filter`: filter mode, cutoff, resonance, and filter envelope amount.
- `Source`: oscillator waveforms, oscillator/sub/noise levels, Osc Warp, and unison settings.
- `Sample`: sample enable, slice range, reverse, playback mode, stutter, pitch, pitch ramp, gain, mix, and loaded sample path.
- `FX`: tone, distortion, pump including custom curve points, phaser, chorus, delay, reverb, Width, and Guard parameters.
- `Output`: output gain, with safety preserved.
- `Seq`: sequencer enable/rate/root/gate/swing/groove/scale/chord/voicing/strum/accent/octave/probability/random amount and step pattern, including per-step timing and length.

## Scope

The LAB and HOME `Scope` selector can limit Generate, Vary, Mutate, or Wild to a single section. `All` keeps the full recipe behavior. `Source`, `Env`, `Filter`, `Sample`, `FX`, `Seq`, and `Macros` restore every other section from the pre-randomization snapshot after the recipe runs. Scoped actions also restore modulation and output gain so focused edits do not quietly change hidden routing or level.

Recipe generation can seed visible MOD routes for synth, FX, and sample movement. Current synth destinations include Osc Warp for source-level harmonic motion. Current FX destinations cover Pump Depth, Delay Mix, Reverb Mix, Width, and FX Drive. Current sample destinations cover Start, Mix, Pitch, Ramp, and Stutter. These use LFO 1 or macro sources where the target supports them, so generated house/UKG patches can expose source grit, ducking, throw, space, drive, and chop motion directly in the matrix. Generated routes are enabled by default, while the MOD panel can bypass individual slots later without deleting their settings.

## Output Safety

Output safety remains active even when `Output` is locked. If a recipe lowers output gain for safety, the lock will not raise it back above that safer value.

## History

Global randomization now keeps one in-session undo snapshot and one redo snapshot. The status line labels the available action, for example `Undo: Mutate` after a mutation or `Redo: Wild` after undoing a wild mutation. A new Generate, Mutate, Vary, or Wild action clears redo so the history stays predictable.

## Section Random Buttons

`Rand Cut` does nothing while `Sample` is locked. `Rand Seq` and SEQ `Vary` do nothing while `Seq` is locked.

SEQ `Undo` restores the previous sequencer utility snapshot after generated, varied, template, copied, rotated, or cleared patterns.
