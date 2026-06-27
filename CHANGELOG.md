# Nate VST Changelog

## 2026-06-27

### Mod Source Route Summaries

- Expanded the MOD `SOURCES` list to include LFO 1, Mod Env 1, Velocity, and all eight performance macros.
- Added live route summaries to each source row, showing active route count and summed modulation depth/polarity.
- Highlighted active source rows with polarity-aware color so generated and hand-built matrix routing is easier to scan.

### Eight Macro Performance Bank

- Added automatable `macro_5` through `macro_8` controls named Weight, Bounce, Warp, and Throw.
- Mapped Weight to low-end reinforcement, Bounce to tempo pump movement, Warp to oscillator/filter edge, and Throw to delay/reverb push.
- Expanded MOD to show the full eight-macro bank in two grouped rows while keeping HOME focused on the first four macros and the Motion/Space XY pad.
- Added Weight, Bounce, Warp, and Throw as modulation matrix sources and seeded all factory presets with deterministic eight-macro values.
- Increased macro/random rotary drag travel for smoother knob movement after the prior fast-drag pass.

### Momentary FX Holds

- Added FX-panel hold buttons for temporary Delay, Space, Pump, and Mute Drop performance moves.
- Momentary holds snapshot the touched FX/output parameters on mouse down, apply the throw while held, and restore the previous values and selected FX module on release.
- Added editor-close cleanup so a held momentary throw is restored if the UI is closed before mouse release.

### FX Throw Performance Buttons

- Added FX-panel `Delay Throw`, `Space Throw`, `Pump Drop`, and `Throw Off` actions for fast house, UKG, tech-house, and minimal arrangement moves.
- Made the throw actions write existing Delay, Reverb, Pump, Width, and Guard parameters instead of adding new plugin state.
- Added a short-lived FX rack status override so the selected throw feedback stays visible while the rack continues refreshing.

### Library Audition And Control Feel

- Added an `Audition` action to HOME and LIBRARY preset rows so selected sounds can be loaded and briefly played without reaching for the bottom keyboard.
- Added timer-based note release and panic-button cleanup for preset auditions to avoid stuck preview notes.
- Tightened HOME/LIBRARY preset row widths and made rotary drag travel shorter again so core house-production knobs move with less mouse effort.

### Library Smart Tags

- Added a LIBRARY `Tag` filter with smart tags such as Bass, Chord, Pluck, Stab, Sequenced, Mono Safe, Pump, Wide, FX, Vocal Chop, and genre tags.
- Added saved `preset_tags` metadata to preset library scanning and user preset saving while keeping older presets valid without tags.
- Regenerated all factory presets with deterministic smart tags from the preset generator.

### Library Search

- Added a `Search presets` field to the LIBRARY panel for filtering presets by name, category, source, user/factory status, or favorite state.
- Made preset search token-based so multi-word queries like `tech bass` can match genre/category plus preset role.

### Club Factory Preset Packs

- Added repo-managed factory presets for house, tech-house, minimal, and techno workflows, expanding the factory pack from 8 to 16 presets.
- Added House, Tech House, Techno, and Minimal Library category filters so the new factory presets are browsable in the plugin.
- Made `tools/generate_factory_presets.py` preserve per-preset categories instead of hardcoding every factory preset as UKG.

### Knob Feel Pass

- Reduced rotary drag travel so common sound-design knobs respond faster while keeping frequency/time controls more controlled.
- Enabled mouse-wheel adjustment on rotary and horizontal sliders for quicker Ableton patching sessions.

### House And Tech Sequencer Templates

- Added `House Chord`, `Tech Bass`, `Minimal Pluck`, and `Techno Pulse` templates to the SEQ pattern selector for faster house, tech-house, techno, and minimal starts.
- Tuned the new templates with genre-specific gate, swing, groove, scale, accent, probability, octave, chord, voicing, and strum defaults.

### Sequencer Utility Undo

- Added a SEQ-panel `Undo` action for restoring the previous sequencer utility state after pattern generation, variation, template apply, copy, rotate, or clear.
- Kept the undo snapshot local to sequencer utilities so house and UKG groove experiments can be tried quickly without changing preset files or performance snapshots.

### Sequencer Variation

- Added a SEQ-panel `Vary` action for making small mutations to the current 16-step pattern without replacing the groove.
- Varied active steps by nudging notes, velocity, probability, and non-anchor timing, with occasional ghost-note adds/removes for house and UKG movement.
- Made SEQ variation respect the sequencer randomization lock and fall back to full sequence generation when the current pattern is empty.

### Sequencer Chord Strum

- Added a saved SEQ `Strum` amount that spreads chord note-ons for softer house chords and less rigid UKG stabs.
- Made live sequencer playback and MIDI export use the same strum timing, while preserving shared gate behavior.
- Updated chord-focused templates and UKG factory presets with subtle strum defaults while bass and vocal chop presets remain unstrummed.

### Sequencer Chord Voicing

- Added a saved SEQ `Voice` selector with Close, Inv 1, Inv 2, Open, and Drop 2 voicings for less blocky house and UKG chord stabs.
- Changed chord construction so scale mode quantizes the step/root first, then chord quality and voicing shape the notes from that root.
- Updated stab-oriented templates and UKG factory presets with explicit voicing metadata while bass and vocal chop presets stay close/mono.

### Sequencer Chord Mode

- Added a saved SEQ `Chord` mode with Off, 5th, Minor, Minor 7, Major, and Minor 9 options for quick house and UKG stab patterns.
- Made sequencer playback and MIDI export use the same chord builder, including scale quantization, per-note velocity trim, and shared gate timing.
- Updated Stab, Organ Skank, and Late Stab templates plus the UKG factory preset pack to carry chord modes while bass and vocal chop templates stay mono.

### Sequencer MIDI Export

- Added a SEQ-panel `MIDI` export action that writes the current 16-step pattern as a `.mid` clip for Ableton arrangement and editing.
- Exported clips follow the sequencer's root, octave, rate, gate, swing/groove timing, per-step timing, scale quantization, velocity, and accent settings.
- Split SEQ pattern controls and utility controls into separate rows so template selection, rotate/copy, export, randomize, and clear are easier to scan.

### Sequencer Rotate Utilities

- Added `Rot <` and `Rot >` controls to shift the whole 16-step SEQ pattern earlier or later without redrawing it.
- Preserved each step's enabled state, note offset, velocity, probability, and timing while rotating, so shuffled UKG and house grooves keep their feel.

### Sample Slice Pads

- Added eight numbered SAMPLE-panel slice pads for jumping the sample start/end window to equal phrase sections.
- Made slice pads audition the selected chop through the sampler only, without also firing the synth voice path.
- Slice pads write the existing automatable `sample_start` and `sample_end` parameters, so reverse, Gate/One Shot, stutter, pitch, ramp, gain, mix, presets, and host state all stay compatible.
- Added active-slice highlighting that follows manual start/end slider edits and preset recall.

### FX Rack Chain Order

- Added saved FX order slots so the rack can process enabled modules in a user-controlled chain order instead of a fixed internal list.
- Added FX page `Up`, `Down`, and `Reset` controls for moving the selected module while keeping Guard pinned as the final safety/output stage.
- Added numbered order badges to FX rack rows and updated the rack status text to show the selected module's chain position.
- Preserved old preset behavior by defaulting the order slots to the previous fixed chain and normalizing invalid or duplicated order data defensively.

### Control Feel and LFO Declutter

- Reworked rotary slider movement to use direct drag instead of velocity mode, with slower drag ranges for fine pitch, filter, EQ, timing, and frequency controls.
- Enlarged the MOD-panel LFO curve editor and hid the cramped point-slider grid from the visible layout while preserving the same automatable curve parameters.
- Gave the MOD macro and LFO rows slightly more vertical room so the page reads more like a focused modulation editor and less like a dense control wall.
- Made SEQ-grid drag editing deterministic: the first cell decides whether the gesture paints notes or erases matching notes, avoiding accidental toggles while drawing basslines.

### Sequencer Scale Helper

- Added a saved `Sequencer Scale` mode with Off, Major, Minor, Dorian, and Minor Pent choices.
- Quantized edited, generated, and played sequencer notes when a scale mode is active while leaving Off as the default compatibility mode.
- Added a SEQ-panel scale selector and dimmed out-of-scale grid rows so garage, house, and techno bass/stab patterns are easier to keep musical.

### Sequencer Grid Orientation

- Added beat-group headers and stronger four-beat column dividers to the SEQ grid so 16-step house and UKG patterns scan in musical bars.
- Added root-aware row labels that follow the sequencer Root and Octave controls.
- Added anchor-step dots and visible probability bars alongside the existing velocity and timing cues.

### Modulation Visibility Pass

- Added a visual MOD matrix row component that highlights active routes, shows polarity/amount as a small bar, and exposes route summaries through tooltips.
- Added a matrix status line and `SOURCE`, `DESTINATION`, and `AMOUNT` headers so random/generated modulation is easier to read.
- Added destination modulation rings to the six current matrix targets: Cutoff, Resonance, Filter Env, Drive, Osc 2 Tune, and Osc 2 Level.
- Rebalanced the MOD page so the routing matrix has taller rows while keeping the LFO curve, point sliders, and Mod Env controls visible.
- Made the LFO curve display directly draggable; edits write back to the existing automatable `lfo1_curve_*` parameters.

### Performance XY Pad

- Added a HOME-panel XY performance pad that controls the existing `Motion` and `Space` macro parameters for one-gesture rhythmic movement.
- Placed the pad inside the `MACROS` card beside the macro knobs so performance movement is grouped with the controls it drives.
- Synced the pad from APVTS macro values on the UI timer so host automation, preset recall, and snapshot recall stay reflected visually.

### Sequencer Workflow Shortcuts

- Exposed the already-wired `Bass`, `Stab`, and `UKG` quick-pattern buttons on the SEQ panel.
- Kept pattern copy, randomize, clear, and the new genre shortcuts in one row for faster house and UK garage sketching.

### Performance A/B Snapshots

- Added HOME-panel A/B performance snapshots with `A`, `Set A`, `B`, and `Set B` controls beside the macro area.
- Stored snapshot payloads inside plugin/preset state as `PerformanceSnapshot` children, while stripping them before APVTS parameter restore so existing parameter automation remains stable.
- Made snapshot recall restore the current patch, sample path, and sequencer pattern while preserving the stored A/B slots for continued comparison.
- Disabled recall buttons until their snapshot slot has content and added compact HOME status text for A/B readiness.

### Club Low-End Assistant

- Added post-FX low-end metering for sub-band RMS, low-frequency stereo side risk, and output peak using lightweight atomic values for the UI.
- Added a compact HOME-panel `CLUB` assistant showing sequencer root Hz, mono/mono-below status, sub energy, low-side risk, and Guard/headroom status.
- Reused existing Mono, Width mono-bass crossover, Guard, and sequencer-root parameters so the assistant reflects the current patch instead of creating a separate safety system.
- Updated MOD-panel background painting to match the newer compact curve layout and reduce visual grouping mismatch.

### LFO Curve Modulation

- Added a sixth LFO 1 shape, `Curve`, with eight saved and automatable curve-point parameters.
- Added a compact curve display and point controls to the MOD panel so rhythmic movement is visible without crowding HOME.
- Routed the curve shape through the voice DSP as a cyclic interpolated modulation source for filter, drive, and oscillator movement through the existing matrix.
- Tightened the MOD page layout by shortening the macro/source row, expanding the LFO editor area, and making routing rows denser to reduce overlap.
- Seeded house, tech-house, and UKG bass randomization with curve-based filter movement while leaving stepped modulation for minimal/noise recipes.
- Regenerated factory presets so UKG bass patches include useful garage-style curve movement.
- Reduced rotary full-range drag distance again so knobs feel quicker during sound-design tweaks.

### Research-Led UI Polish

- Ran a focused reference pass across Serum 2, Pigments, Phase Plant, u-he, Korg, ZENOLOGY Pro, ShaperBox, and UK garage production workflows.
- Updated the UI/effects roadmap with the next product tracks: UKG bass engine, vocal chop/stab sampler, wavetable/character filter pass, MSEG modulation, and tagged construction-kit browsing.
- Simplified HOME into a faster dashboard with Perform, Macros, Random Lab, and Library areas instead of duplicating oscillator waveform, filter-mode, mono, unison, and glide controls from SYNTH.
- Grouped the Add FX menu by production role: Tone & Drive, Movement, and Space & Utility.
- Made the FX rack switch to a compact two-column layout when many fixed modules are visible, reducing overflow risk as the effect list grows.
- Prevented Guard from being removed from the rack so output safety remains available.
- Tuned rotary controls to use velocity-aware movement with a less cramped drag feel while keeping double-click reset, value popups, snap-to-click disabled, and mouse-wheel edits disabled.

### FX Rack Row UI

- Added a dedicated `FxRackRow` JUCE component for the fixed FX rack.
- Separated selected state, enabled/bypassed state, module name, module summary, and Guard safety status instead of encoding everything into text-button labels.
- Kept the existing fixed FX parameters, DSP order, Add FX menu, and selected-module editor behavior stable.
- Preserved the one-column/two-column compact rack layout while improving scanability when many house/UKG FX modules are active.

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

### Three-Band EQ FX

- Added a bypassable EQ module to the focused FX rack.
- Added stable EQ parameters for low gain, mid gain, high gain, and trim.
- Processed EQ after Tone and before Drive/Crush so patches can be shaped before saturation.
- Added EQ to the Add FX menu, rack list, selected-module editor, FX randomization lock, recipe randomization, and factory preset defaults.
- Regenerated the UKG factory preset pack with practical EQ curves for basses, stabs, plucks, and vocal chop starters.

### Tremolo And Auto Pan FX

- Added a bypassable Tremolo module to the focused FX rack.
- Added stable Tremolo parameters for synced rate, depth, pan amount, shape, and phase.
- Processed Tremolo after Pump and before phaser/flanger/chorus for rhythmic movement before modulation effects.
- Added Tremolo to the Add FX menu, rack list, selected-module editor, FX randomization lock, recipe randomization, and factory preset defaults.
- Regenerated the UKG factory preset pack with light synced motion for stabs, bell plucks, late hits, and vocal chop starters.

### Ring Mod FX

- Added a bypassable Ring Mod module to the focused FX rack.
- Added stable Ring Mod parameters for frequency, depth, mix, and bias.
- Processed Ring Mod after Tremolo and before phaser/flanger/chorus for metallic movement before modulation effects.
- Added Ring Mod to the Add FX menu, rack list, selected-module editor, FX randomization lock, recipe randomization, and factory preset defaults.
- Regenerated the UKG factory preset pack with restrained metallic edge for bell plucks, vocal chop starters, and late stabs.

### Comb Resonator FX

- Added a bypassable Comb module to the focused FX rack.
- Added stable Comb parameters for frequency, feedback, damping, and mix.
- Processed Comb after Ring Mod and before phaser/flanger/chorus for tuned resonance before modulation effects.
- Added Comb to the Add FX menu, rack list, selected-module editor, FX randomization lock, recipe randomization, and factory preset defaults.
- Regenerated the UKG factory preset pack with tuned resonance for bell plucks, vocal chop starters, and late stabs.
