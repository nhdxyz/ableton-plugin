# Nate VST Randomization Workflow

## Goal

The LAB panel should make new patches quickly while letting users protect the parts of a sound that already work.

## Actions

- `Generate` creates a new patch from the selected recipe.
- `Mutate` moves the current patch toward a new recipe result.
- `Variation` makes a smaller move from the current patch.
- `Undo` restores the patch state from immediately before the last Generate, Mutate, or Variation action.

## Locks

Locks are applied after the selected recipe runs. This keeps the recipe engine simple while restoring protected sections from the pre-randomization snapshot.

- `Pitch`: oscillator octave/tune, oscillator 2 octave/tune, mono, and glide.
- `Env`: amp attack, decay, sustain, and release.
- `Filter`: filter mode, cutoff, resonance, and filter envelope amount.
- `Source`: oscillator waveforms, oscillator/sub/noise levels, and unison settings.
- `Sample`: sample enable, slice range, reverse, playback mode, stutter, pitch, pitch ramp, gain, mix, and loaded sample path.
- `FX`: tone, distortion, phaser, chorus, delay, reverb, and Guard parameters.
- `Output`: output gain, with safety preserved.
- `Seq`: sequencer enable/rate/root/gate/swing/groove/accent/octave/probability/random amount and step pattern, including per-step timing.

## Output Safety

Output safety remains active even when `Output` is locked. If a recipe lowers output gain for safety, the lock will not raise it back above that safer value.

## Section Random Buttons

`Rand Cut` does nothing while `Sample` is locked. `Rand Seq` does nothing while `Seq` is locked.
