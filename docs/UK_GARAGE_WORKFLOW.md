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
- MusicRadar Silva Bumpa production interview: https://www.musicradar.com/music-tech/sometimes-swinging-all-the-elements-of-the-drums-doesnt-make-it-groove-better-breakout-producer-silva-bumpa-on-the-secret-to-creating-sub-bass-and-ukg-rhythms
- NITELIFE UK garage vocal cuts guide: https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/
- Splice speed garage sound notes: https://splice.com/sounds/genres/speed-garage/samples

## Researched Sound Map

### Timing And Groove

- Default UKG range should sit around 125 to 140 BPM, with 130 to 134 BPM as the practical center for 2-step and modern garage.
- The sequencer should favor late/offbeat placement, strong swing, short gates, and controlled ghost probability.
- Swing now has a selective groove layer: Classic preserves old offbeat swing, Selective uses per-step timing, UKG Push keeps anchors tighter while pushing ghost/offbeat steps, and Tight applies only conservative per-step delay.
- Drum-adjacent synth patterns should leave gaps for rim shots, shakers, and chopped hats instead of filling every 16th.

### Bass Character

- Core bass targets: mono sub, Reese movement, Dred/reverse-filter movement, and short rubbery plucks.
- Useful controls: filter envelope amount, filter envelope attack, glide, mono/legato, drive, Crush mix, and output Guard.
- Bass randomization should include tuned-sub guardrails, with practical club notes called out when preset authors build sub-focused patches.
- Sub-focused preset authoring should keep the fundamental in a useful club range, roughly 40 to 80 Hz, and avoid relying on stereo detune for the weight of the bass.
- UKG randomization should keep low-end stable first, then add motion with filter and oscillator detune.
- Dred/reverse-filter bass should use a centered sub, a restrained Reese layer, a slower modulation envelope opening the filter, and short glide so it feels like a garage bass phrase without making the amp envelope sluggish.

### Stabs, Keys, And Leads

- Common colors to support: organ stabs, M1-ish house chords, short bell/pluck phrases, bright minor chord stabs, and simple repeating lead hooks.
- These need short envelopes, cleaner transients than techno stabs, optional chorus/width, and a quick low-cut/tone path.
- A future organ/stab source should start as recipes and presets before adding a dedicated drawbar-style oscillator mode.

### Vocal And Sample Behavior

- Chopped vocal hooks are central enough that the sampler needs a dedicated UKG chop workflow, not just generic sample playback.
- Required controls: one-shot vs gated, quick slice/cut ranges, pitch offsets, pitch ramps, reverse, stutter/retrigger, delay throws, and sequencer-lane triggering.
- Randomized chops should prefer short, rhythmic fragments and avoid huge unpredictable pitch sweeps unless chaos is high.
- True formant-preserving pitch movement should remain future work because it needs a dedicated time-stretch/formant engine; the current lightweight performance pass uses pitch-ramped playback instead.

### FX Flavor

- Useful UKG FX are tight and rhythmic: three-band EQ for low-mid cleanup and presence, short room/plate, short delay throws, pump/duck, tremolo/auto-pan movement, restrained ring modulation and comb resonance for chop/pluck edge, stereo width above the bass, subtle chorus/phaser/flanger, bitcrush/downsample for sampler color, and reverse/rewind-style transitions.
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
- Selective groove so anchors can stay tight while offbeats, vocal triggers, and stab hits sit later.
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
- Added the `Width` FX module for mono-safe bass and controlled stereo upper content.
- Added a compact sequencer template selector with UKG 2-step, shuffle bass, organ skank, vocal chop, and late stab patterns.
- Added sequencer Groove modes and a per-step timing lane so UKG templates can push selected offbeats without moving every step equally.
- Added UKG organ stab, chord stab, and bell pluck randomization recipes.
- Added a SAMPLE-panel `UKG Chop` action for short musical vocal/sample cuts, One Shot playback, and synced stutter/retrigger.
- Added UKG bass guardrails: audible mono glide, restrained sub/octave choices, limited detune/spread, Width mono-bass protection, and Guard safety.
- Added a first UKG factory preset pack with bass, stab, bell, vocal chop starter, and sequence-focused patches.
- Added a sampler Pitch Ramp control and made `UKG Chop` create pitch-ramped vocal cuts with delay throws when FX locking allows it.
- Added `UKG Dred Bass` as a reverse-filter/Reese-style randomization recipe and factory preset.
- Added a focused EQ module and seeded the UKG factory presets with subtle bass, stab, pluck, and vocal chop EQ curves.
- Added a focused Tremolo/Auto Pan module and seeded UKG movement presets with light synced motion.
- Added a focused Ring Mod module and seeded select UKG pluck, chop, and late-stab presets with restrained metallic edge.
- Added a focused Comb module and seeded select UKG pluck, chop, and late-stab presets with tuned resonance.

## Next UKG Features

1. Vocal chop mode for the sampler.

- Added a `UKG Chop` action for short chop windows, musical pitch choices, reverse variation, One Shot playback, synced stutter/retrigger, and automatic Vocal Chop sequencer template application.
- Added one-shot/gated playback option.
- Tighter random cut ranges for short phrases are implemented in `UKG Chop`.
- Quick pitch choices such as -12, -7, 0, +7, +12 are implemented in `UKG Chop`.
- Retrigger/stutter timing is implemented as synced sample stutter controls with 1/8, 1/16, and 1/32 rates plus repeat count.
- Pitch-ramped chop movement is implemented with the SAMPLE-panel `Ramp` control.
- UKG Chop now sets a short delay throw when FX locking is off.
- The SAMPLE panel now includes eight numbered slice pads for jumping and auditioning a loaded phrase in equal chop windows before applying stutter, reverse, pitch, ramp, or sequencer triggering.

2. Pump/Duck FX.

- Implemented as the `Pump` FX module.
- Tempo-synced volume shaper.
- Depth, rate, shape, and phase controls.
- Useful for UKG, house, and garage-adjacent bass movement.
- Bass presets now use the MOD-panel curve LFO for garage-style push/pull cutoff movement where a basic sine felt too even.
- HOME now includes a compact club low-end assistant so UKG bass patches expose root Hz, sub energy, low-side stereo risk, Width mono-below status, and Guard/headroom state while designing.
- Available as an FX rack module. Simple macro/mod assignment should come through the future MOD workflow.

3. Organ/Stab source.

- Implemented first as curated randomization recipes: `UKG Organ Stab`, `UKG Chord Stab`, and `UKG Bell Pluck`.
- Later add organ-style oscillator mix or drawbar-inspired mode.
- Factory preset files should be added in a later preset-pack pass.

4. Swing templates.

- Implemented as a compact SEQ-panel template selector.
- Templates include `UKG 2-Step`, `Shuffle Bass`, `Organ Skank`, `Vocal Chop`, and `Late Stab`.
- Per-template rate, gate, swing, groove mode, scale mode, per-step timing, accent, probability, octave, and random amount defaults are set when applied.
- The grid now shows timing markers on pushed steps, probability bars, anchor-step dots, beat-group dividers, and root-aware note rows, with mouse-wheel nudging and right-click/command-click cycling for direct per-step groove edits.
- The SEQ panel includes Major, Minor, Dorian, and Minor Pent scale modes for keeping garage basslines, stabs, and generated patterns in key.
- The SEQ panel includes chord modes for fast 5th, minor, minor 7, major, and minor 9 stabs; Organ Skank, UKG Chord Stab, and Late Stab templates now use those modes by default.
- The SEQ panel includes Close, Inv 1, Inv 2, Open, and Drop 2 voicings so short organ, chord, and late-stab patterns can sit in different registers without redrawing the grid.
- The SEQ panel includes a Strum control for subtle flam/spread on chord stabs; chord-focused factory presets use it lightly while bass and vocal chop patterns stay tight.
- The SEQ panel includes `Vary` for small groove-preserving mutations to basslines, chord stabs, and vocal-trigger patterns.
- The SEQ panel includes `Undo` for stepping back from generated, varied, template, copied, rotated, or cleared patterns while searching for a garage groove.
- The SEQ panel includes `Rot <` and `Rot >` controls for shifting 2-step basslines, late stabs, and vocal-trigger patterns around the bar while keeping per-step groove details intact.
- The SEQ panel includes MIDI export so generated UKG basslines, skanks, and vocal trigger patterns can be saved as `.mid` clips and edited directly in Ableton.

5. UKG factory presets.

- Implemented as repo-managed `.natevstpreset` files generated by `tools/generate_factory_presets.py`.
- Current pack: `UKG 2-Step Bass`, `UKG Shuffle Bass`, `UKG Dred Bass`, `UKG Organ Stab`, `UKG Chord Stab`, `UKG Bell Pluck`, `UKG Vocal Chop Starter`, and `UKG Late Stab`.
- Bass presets use the UKG guardrails as defaults: mono enabled, Glide audible but short, sub stable, upper-bass movement separated from the centered low end, and Width/Guard active.
- The SEQ template selector now has first-pass house, tech-house, techno, and minimal pattern starters, and the factory generator now includes matching House, Tech House, Techno, and Minimal patch categories.

6. Width and mono-bass utility.

- Implemented as the `Width` FX module.
- Keep sub frequencies centered.
- Add width for stabs, shakers, delays, and reverbs without weakening the bass.

7. Transition and rewind FX.

- Add short reverse, downer, delay throw, and rewind-style one-shot workflows after the sampler is stronger.
