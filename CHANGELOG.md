# Nate VST Changelog

## 2026-06-26

### Rename To Nate VST

- Renamed the JUCE/CMake target from `Floorform` to `NateVST`.
- Changed the plugin product name shown to hosts from `Floorform` to `Nate VST`.
- Updated the VST3 bundle path to `Nate VST.vst3`.
- Moved new user presets to `~/Library/Application Support/Nate VST/Presets`.
- Changed user preset extension from `.floorformpreset` to `.natevstpreset`.
- Renamed the main processor/editor C++ classes to match the new product direction.

### Home Dashboard And Control Feel

- Added a `HOME` panel as the default first view.
- Reworked the first screen into a practical dashboard with engine, shape, lab, and library areas.
- Added quick access to waveform, filter mode, mono, tone-shaping knobs, recipe randomization, and preset load/save from the home panel.
- Reduced rotary knob drag distance and enabled velocity mode so knobs respond with less physical mouse travel.
- Added an automatable `filter_mode` parameter with low-pass, band-pass, and high-pass modes.
- Wired randomization to use filter modes where it adds useful variation, especially minimal blips and noise FX.

### Expanded Synth Source Mixer

- Added oscillator 2 with independent waveform, octave, tune, and level controls.
- Added oscillator 1, oscillator 2, sub, and noise source levels to the synth engine.
- Added a sine sub oscillator one octave below the played note.
- Added a white-noise source for stabs, blips, and noise FX recipes.
- Added source-level compensation so layered patches stay closer to a safe output range.
- Updated HOME and SYNTH panels with the new source mix controls.
- Updated randomization recipes so basses can use sub reinforcement, stabs can use detuned oscillator 2, and noise FX can lean into the noise source.

### Control Ergonomics Pass

- Reduced rotary drag distance again so knobs respond with less mouse travel.
- Added drag/hover value popups for rotary and horizontal sliders.
- Disabled snap-to-click behavior so controls do not jump unexpectedly.
- Added double-click reset for every slider using the parameter default value.
- Added clearer hover, pressed, and selected states for text buttons and segmented selectors.
- Added hover emphasis to rotary knob rings.

### Preset Browser Workflow

- Added preset category selection for saved `.natevstpreset` files.
- Saved category metadata into preset XML using the `preset_category` property.
- Added previous and next preset buttons on HOME and LIBRARY panels.
- Left deeper filtering, favorites, recent presets, and factory/user separation for the later library pass below.

### Modulation Planning

- Added `docs/MODULATION_WORKFLOW.md` as the first modulation and performance workflow plan.
- Defined initial modulation sources, destinations, macro controls, LFO 1 shape, matrix scope, and safety rules.
- Recommended the first implementation slice as four HOME macros before a full modulation matrix.

### HOME Performance Macros

- Added automatable `macro_1` through `macro_4` parameters.
- Added HOME macro knobs for Tone, Dirt, Motion, and Space.
- Mapped Tone to filter cutoff and resonance offsets.
- Mapped Dirt to synth drive with output compensation.
- Mapped Motion to filter envelope amount and oscillator 2 detune.
- Mapped Space to delay and reverb mix, even when the dedicated FX toggles are off.
- Made randomization pull macros back toward neutral so generated patches stay predictable.

### Unison And Stereo Spread

- Added automatable `unison_voices`, `unison_detune`, `unison_blend`, and `unison_spread` parameters.
- Added Voices and Spread controls to HOME, with the full unison control set on SYNTH.
- Added per-note oscillator stacking with level compensation for safer wide stabs, leads, drones, and FX.
- Kept the sub oscillator centered and forced unison spread to mono while `Mono` is enabled.
- Updated randomization recipes so bass and acid patches stay conservative while stabs, blips, and FX can get controlled width.

### Randomization Locks And Undo

- Added LAB lock toggles for Pitch, Env, Filter, Source, Sample, FX, Output, and Seq.
- Added one-step Undo for Generate, Mutate, and Variation actions.
- Restored locked parameter groups after recipe generation so protected sections keep their previous values.
- Made sample cut and sequencer randomization respect their section locks.
- Added LAB status text that reports the last randomization action and active locks.
- Kept output safety active by allowing the output lock to preserve or lower gain, but not raise it above the randomized safe value.
- Documented lock behavior in `docs/RANDOMIZATION_WORKFLOW.md`.

### Sequencer Groove And Performance

- Added automatable `sequencer_swing`, `sequencer_accent`, `sequencer_octave`, and `sequencer_probability` parameters.
- Added SEQ controls for Root, Gate, Swing, Accent, Oct, Prob, and Rand in a compact knob row.
- Added Bass and Stab pattern preset buttons plus a Copy action that duplicates steps 1-8 into 9-16.
- Made sequencer swing alternate long and short step durations while keeping host tempo sync.
- Added accent behavior for anchor steps and global probability scaling over per-step probability.
- Updated `Rand Seq` to generate conservative swing, accent, octave, and probability settings with the pattern.

### Visual Feedback And Metering

- Added a compact stereo output meter to the top bar so output level stays visible across panels.
- Measured output peak and RMS after the FX rack using relaxed atomics for UI reads.
- Kept meter smoothing and repainting on the editor timer instead of the audio thread.

### FX Rack Expansion

- Added a bypassable Tone module with tilt and low-cut controls for quick mix cleanup.
- Added a bypassable Phaser module with rate, depth, and mix controls.
- Added a bypassable Guard module with push and ceiling controls for controlled output clipping.
- Reworked the FX panel into a two-row module grid that keeps Tone, Dist, Phaser, Chorus, Delay, Reverb, and Guard visible.
- Made randomization set conservative Tone, Phaser, and Guard values per recipe while preserving output safety.

### Deeper Library Workflow

- Added LIBRARY filtering for All, Favorites, Recent, User, Factory, and saved categories.
- Added a `Fav` preset toggle and persistent favorite storage in `~/Library/Application Support/Nate VST/Library.xml`.
- Added recent preset tracking when presets load.
- Added factory preset scanning from `~/Library/Application Support/Nate VST/Factory Presets` while keeping user presets in the existing `Presets` folder.
- Saved user preset metadata for category, author, and source.

### Bottom Piano Keyboard

- Added a persistent bottom piano keyboard for mouse auditioning patches across editor panels.
- Wired the keyboard through `MidiKeyboardState` so clicked notes enter the same MIDI path as host input and sequencer notes.
- Increased the fixed editor height to keep the new keyboard from crowding existing controls.

### UI And FX Roadmap

- Added `docs/UI_EFFECTS_ROADMAP.md` to plan the next round of grouping, modulation visibility, knob ergonomics, and FX expansion.
- Defined a scalable FX direction using an Add FX menu, rack list, and focused selected-effect editor instead of a fixed grid that shows every module at once.
- Identified the next best effect candidates for house, tech house, techno, and minimal workflows: bitcrusher/downsample, flanger, pump/duck, stereo width/mono bass, and three-band EQ.
- Captured the parameter-stability constraint for Ableton: the first Add FX pass should hide, reveal, and bypass fixed modules before attempting true dynamic effect instances.

### Focused FX Rack UI And Knob Polish

- Replaced the FX page's full module grid with an Add FX menu, compact rack list, and focused selected-module editor.
- Kept existing FX parameter IDs stable so Ableton automation and saved sets remain compatible.
- Made Remove behave as a safe bypass/hide action instead of deleting host parameters.
- Kept Guard pinned as the final output safety module.
- Improved rotary knob rendering with a clearer panel, rail, value arc, pointer, and center cap.
- Lowered rotary drag distance and disabled mouse-wheel edits on sliders to reduce accidental changes.
- Documented the framework decision to stay on JUCE instead of adding a second GUI framework for this phase.

### UK Garage Direction

- Added `docs/UK_GARAGE_WORKFLOW.md` to capture UK garage sound targets and next feature priorities.
- Added `UKG 2-Step Bass` as a randomization recipe while preserving existing recipe indexes.
- Added `UKG` as a preset category and library filter option.
- Added a `UKG` sequencer preset button with a skippy 2-step pattern, strong swing, short gates, and ghost-note probability.
- Tuned the UKG recipe toward mono-safe sub/reese bass, short envelopes, controlled filter motion, light movement FX, and Guard output safety.

### Bitcrush FX

- Added a bypassable Crush FX module with bit depth, downsample, and mix controls.
- Routed Crush after distortion and before modulation/time FX for old-sampler grit, minimal click texture, UKG vocal/bass color, and techno edge.
- Added Crush to the Add FX menu, rack list, selected-module editor, FX randomization lock, and recipe randomization.
- Biased Minimal Blip, Noise FX, Rolling Tech Bass, and UKG recipes toward conservative-to-aggressive Crush settings where useful.

### UK Garage Planning

- Expanded `docs/UK_GARAGE_WORKFLOW.md` with researched UKG sound targets for groove, bass, stabs, vocal chops, and FX flavor.
- Added UKG-specific priorities to `docs/UI_EFFECTS_ROADMAP.md` so upcoming FX and UI work favors vocal chops, pump/duck, mono-safe width, organ/stab recipes, and swing templates.

### Pump/Duck FX

- Added a tempo-synced `Pump` FX module inspired by common sidechain/volume-shaper workflows for house and UK garage.
- Added stable Pump parameters for bypass, synced rate, depth, shape, and phase.
- Aligned Pump to host PPQ position when available, with BPM-based free-run fallback when a host does not expose grid position.
- Added Pump to the FX Add menu, rack list, selected-module editor, FX randomization lock, and recipe randomization.
- Biased UKG, deep house, rolling tech, stab, minimal, acid, and noise recipes toward conservative-to-aggressive Pump settings where useful.

### Width/Mono Bass FX

- Added a late-chain `Width` FX utility for mono-safe low end and controllable stereo upper content.
- Added stable Width parameters for bypass, width amount, and mono-bass crossover.
- Processed lows as mono below the crossover while applying mid/side width only to upper content.
- Added Width to the FX Add menu, rack list, selected-module editor, FX randomization lock, and recipe randomization.
- Biased bass recipes toward subtle width/mono-bass protection and stab/noise recipes toward wider upper content.

### UKG Sequencer Templates

- Replaced the fixed Bass/Stab/UKG button row in the SEQ panel with a compact template selector and Apply button.
- Added UKG-focused templates for 2-step bass, shuffle bass, organ skank, vocal chop triggering, and late offbeat stabs.
- Made sequencer templates set rate, gate, swing, accent, octave, probability, and random amount so each pattern starts with a distinct feel.

### UKG Stab Recipes

- Added `UKG Organ Stab`, `UKG Chord Stab`, and `UKG Bell Pluck` randomization recipes.
- Appended the new recipes after existing choices so current saved recipe indexes remain stable.
- Tuned the recipes for short garage-style envelopes, bright but controlled filtering, sparse movement FX, Pump, Width, and Guard safety.

### UKG Vocal Chop Workflow

- Added a `UKG Chop` action to the SAMPLE panel for loaded vocal phrases and short rhythmic samples.
- Biased UKG chops toward short cut windows, musical pitch offsets `-12`, `-7`, `0`, `+7`, and `+12`, optional reverse, and hotter sample mix levels.
- Made `UKG Chop` apply the Vocal Chop sequencer template when sequencer locking is not active.
- Added a Gate/One Shot sample playback mode selector, preserving One Shot as the default and leaving Gate available for tight note-off chopping.
- Made the sampler process MIDI note events in block order so gated chops respond to sequencer note-offs.
- Added tempo-synced sample stutter/retrigger with on/off, rate, and repeat controls.
- Made `UKG Chop` use One Shot with stutter at musical 1/16 or 1/32 rates so repeats can complete after short sequencer gates.

### Selective Groove Sequencer

- Added a sequencer Groove mode selector with Classic, Selective, UKG Push, and Tight timing behavior.
- Added a saved per-step timing lane, visible as amber markers in the sequencer grid.
- Made UKG sequencer templates set groove mode and per-step timing so anchors stay tighter while offbeats, vocal triggers, and stab hits can sit later.
- Split the SEQ panel controls into timing and pattern rows to reduce crowding.

### UKG Bass Guardrails

- Made the existing Glide parameter audible in mono bass patches with sample-rate-aware pitch smoothing.
- Added a visible Glide knob to the HOME and SYNTH panels.
- Tightened the `UKG 2-Step Bass` recipe around a mono-safe tuned sub layer, restrained oscillator detune, limited stereo spread, protected low-cut, Width mono-bass, and Guard output safety.
- Applied UKG bass guardrails after mutation/variation blending so locked-in bass patches do not drift into unstable low-end ranges.

### UKG Factory Presets

- Added a repo-managed UKG factory preset pack under `Resources/Factory Presets`.
- Added factory presets for 2-step bass, shuffle bass, organ stab, chord stab, bell pluck, vocal chop starter, and late stab workflows.
- Added `tools/generate_factory_presets.py` to regenerate `.natevstpreset` XML files from documented preset definitions.
- Made local macOS builds copy factory presets into `~/Library/Application Support/Nate VST/Factory Presets`.

### UKG Vocal Chop Performance

- Added a SAMPLE-panel `Ramp` control for pitch-ramped sample playback across a chop window.
- Made sample stutter retriggers reset the pitch-ramp movement naturally by restarting the source position.
- Made `UKG Chop` randomize musical pitch-ramp movements and set a short delay throw when FX locking is off.
- Regenerated the factory preset pack so `UKG Vocal Chop Starter` demonstrates the new pitch-ramp control.

### Knob Ergonomics And Home Grouping

- Switched rotary knobs to direct drag with shorter full-range movement so small sound-design moves feel less awkward.
- Enlarged the rotary visual target and value box, strengthened the pointer/value arc, and added clearer hover emphasis.
- Reduced HOME clutter by renaming the main areas to Source, Shape, Motion, and Library.
- Kept HOME Shape focused on Sub, Cutoff, Drive, and Output while leaving oscillator levels, noise, resonance, and filter-envelope depth in the SYNTH panel.
- Preserved all existing parameter IDs and preset state.

### Visible MOD Workflow

- Added a dedicated `MOD` tab between LAB and SAMPLE.
- Added a source summary for the current Tone, Dirt, Motion, and Space macro behavior.
- Added a focused macro area using the same saved APVTS macro parameters as HOME.
- Added a routing table that shows the real fixed macro destinations and depths currently implemented in the synth and effects engines.
- Deferred editable LFO, mod-envelope, velocity, and matrix-slot DSP to a later modulation-engine slice.

### Editable Modulation Engine Slice

- Added saved APVTS parameters for LFO 1, Mod Env 1, and eight modulation matrix slots.
- Added per-voice LFO 1 with free or host-synced rates, sine/triangle/saw/square/step shapes, phase, depth, and retrigger behavior.
- Added per-voice Mod Env 1 with ADSR and depth controls.
- Added editable MOD-panel matrix rows with source, synth destination, and bipolar amount controls.
- Implemented safe synth destinations for filter cutoff, resonance, filter envelope amount, drive, oscillator 2 tune, and oscillator 2 level.
- Made randomization seed conservative LFO/envelope assignments for bass, UKG stabs, plucks, minimal blips, and noise FX.
- Regenerated the UKG factory preset pack with modulation defaults.

### Flanger FX

- Added a bypassable Flanger module to the focused FX rack.
- Added stable Flanger parameters for rate, depth, feedback, and mix.
- Processed Flanger after Phaser and before Chorus for short-delay metallic movement.
- Added Flanger to the Add FX menu, rack list, selected-module editor, FX randomization lock, recipe randomization, and factory preset defaults.

### UKG Dred Bass

- Added `UKG Dred Bass` as a new randomization recipe without changing existing recipe indexes.
- Tuned the recipe for mono-safe sub, restrained Reese detune, slower filter-opening modulation, short glide, Pump, Width, subtle movement FX, and Guard safety.
- Added a matching repo-managed factory preset and regenerated the UKG preset pack.
- Updated UK garage workflow docs and factory preset notes.
