# Nate VST UK Garage Direction

## Goal

Nate VST should support UK garage alongside house, tech house, techno, and minimal. The focus is not only tempo or preset names. UK garage needs a different feel: shuffled 2-step rhythm, elastic bass, short chord/organ stabs, vocal chops, and tight space effects.

Reference patterns reviewed:

- Skippy 2-step drums with shuffled/offbeat placement.
- Sub and reese-style basslines.
- Organ, chord, and stab sounds.
- Chopped vocal hooks and short sample phrases.
- Short delay, plate/room reverb, chorus/width, and sidechain-style pump.

Research references:

- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- Attack Magazine UK garage beat breakdown: https://www.attackmagazine.com/technique/beat-dissected/uk-garage/
- Attack Magazine modern garage/Overmono-style breakdown: https://www.attackmagazine.com/technique/tutorials/how-to-make-banging-modern-garage-in-style-of-overmono-so-u-kno/
- Splice speed garage sound notes: https://splice.com/sounds/genres/speed-garage/samples

## Researched Sound Map

### Timing And Groove

- Default UKG range should sit around 125 to 140 BPM, with 130 to 134 BPM as the practical center for 2-step and modern garage.
- The sequencer should favor late/offbeat placement, strong swing, short gates, and controlled ghost probability.
- Drum-adjacent synth patterns should leave gaps for rim shots, shakers, and chopped hats instead of filling every 16th.

### Bass Character

- Core bass targets: mono sub, Reese movement, Dred/reverse-filter movement, and short rubbery plucks.
- Useful controls: filter envelope amount, filter envelope attack, glide, mono/legato, drive, Crush mix, and output Guard.
- UKG randomization should keep low-end stable first, then add motion with filter and oscillator detune.

### Stabs, Keys, And Leads

- Common colors to support: organ stabs, M1-ish house chords, short bell/pluck phrases, bright minor chord stabs, and simple repeating lead hooks.
- These need short envelopes, cleaner transients than techno stabs, optional chorus/width, and a quick low-cut/tone path.
- A future organ/stab source should start as recipes and presets before adding a dedicated drawbar-style oscillator mode.

### Vocal And Sample Behavior

- Chopped vocal hooks are central enough that the sampler needs a dedicated UKG chop workflow, not just generic sample playback.
- Required controls: one-shot vs gated, quick slice/cut ranges, pitch offsets, reverse, stutter/retrigger, and sequencer-lane triggering.
- Randomized chops should prefer short, rhythmic fragments and avoid huge unpredictable pitch sweeps unless chaos is high.

### FX Flavor

- Useful UKG FX are tight and rhythmic: short room/plate, short delay throws, pump/duck, stereo width above the bass, subtle chorus/phaser, bitcrush/downsample for sampler color, and reverse/rewind-style transitions.
- The newly added Crush module fits this by giving old-sampler grit without requiring a full sample editor yet.

## Sound Targets

### Bass

- Mono-safe sub layer.
- Slightly detuned second oscillator for reese-style motion.
- Short-to-medium envelopes so notes bounce with the groove.
- Moderate filter envelope and drive.
- Guard enabled by default for level safety.

### Chords And Organ Stabs

- Short attack.
- Tight decay and release.
- Brighter filtering than techno stabs, but not harsh.
- Light chorus or phaser only when it helps movement.
- Later: dedicated organ-style source or additive/drawbar-inspired mode.

### Vocal Chops

- The sampler is the right foundation.
- UKG needs quick random cuts, pitch offsets, reverse as an option, and rhythmic triggering.
- Later: add slice markers, transient-ish slicing, one-shot/gated modes, and per-slice pitch.

### Groove

- 1/16 sequencing with strong swing.
- Offbeat and late-step note placement.
- Short gates.
- Some probability for ghost-note variation.

## First Implemented Slice

- Added `UKG 2-Step Bass` to the randomization recipe list.
- Appended the recipe to preserve existing saved recipe indexes.
- Added UKG as a preset category and filter option.
- Added a `UKG` sequencer preset button.
- Added a skippy 2-step sequencer preset with high swing, short gates, and ghost-note probability.
- Added the `Crush` FX module and biased the UKG recipe toward light bitcrush/downsample color when useful.
- Added the `Pump` FX module and biased the UKG recipe toward tempo-synced ducking/pump movement.

## Next UKG Features

1. Vocal chop mode for the sampler.

- Add one-shot/gated playback option.
- Add tighter random cut ranges for short phrases.
- Add quick pitch choices such as -12, -7, 0, +7, +12.
- Add a `UKG Chop` randomize action.
- Add retrigger/stutter timing tied to the sequencer rate.

2. Pump/Duck FX.

- Implemented as the `Pump` FX module.
- Tempo-synced volume shaper.
- Depth, rate, shape, and phase controls.
- Useful for UKG, house, and garage-adjacent bass movement.
- Available as an FX rack module. Simple macro/mod assignment should come through the future MOD workflow.

3. Organ/Stab source.

- Start with a curated synth patch recipe.
- Later add organ-style oscillator mix or drawbar-inspired mode.
- Add recipe/preset targets for organ stab, M1-ish chord, bell pluck, and bright minor stab.

4. Swing templates.

- Add more sequencer presets: `2-Step`, `Shuffle Bass`, `Organ Skank`, and `Vocal Chop`.
- Add per-template gate and probability defaults.

5. UKG factory presets.

- Bass: sub/reese.
- Chords: organ stab, bright stab.
- Sample: vocal chop starter.
- Sequence: 2-step bass and offbeat stab patterns.

6. Width and mono-bass utility.

- Keep sub frequencies centered.
- Add width for stabs, shakers, delays, and reverbs without weakening the bass.

7. Transition and rewind FX.

- Add short reverse, downer, delay throw, and rewind-style one-shot workflows after the sampler is stronger.
