# Nate VST Synth Plugin Plan

## Current Direction

Build Nate VST, a macOS-first synth plugin for Ableton Live, focused on house, tech house, techno, minimal, and related electronic styles.

The plugin should be an instrument first: it receives MIDI, generates synth sounds, and gives fast sound-design controls for shaping, distorting, modulating, and automating those sounds inside Ableton.

The first deliverable should be a working VST3 synth that Ableton can scan, load, play from MIDI, automate, and save inside a Live Set.

## Working Product Idea

Product concept:

> A fast, gritty dance-music synth for basses, stabs, plucks, acid lines, minimal blips, drones, and distorted club textures.

This should not feel like a generic all-purpose synthesizer. It should feel designed for repeated Ableton workflows:

- Make a bassline quickly.
- Add drive without losing level control.
- Shape filter movement for groove.
- Automate parameters cleanly.
- Save and recall useful presets.
- Keep CPU low enough for multiple instances.

## Reference Synths And Inspiration

The long-term ambition is closer to a modern hybrid synth than a simple subtractive synth. The project can borrow broad workflow ideas from major instruments, but it should not clone their UI, branding, presets, wavetables, samples, DSP code, or proprietary behavior.

Reference products:

- Roland ZENOLOGY / ZEN-Core.
- Xfer Serum.
- u-he synths such as Diva, Hive, Zebra, and Repro.
- Korg instruments such as wavestate, modwave, opsix, MS-20, and Wavestation.
- Arturia Pigments and similar modern hybrid instruments for sequencer/modulation organization.
- Cableguys ShaperBox-style focused rhythmic shaping workflows.

Current research references:

- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- Xfer Serum 2: https://xferrecords.com/products/serum-2
- u-he Diva: https://u-he.com/products/diva/
- Korg Collection: https://www.korg.com/us/products/software/korg_collection/
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Cableguys ShaperBox: https://www.cableguys.com/shaperbox
- Ableton Simpler: https://www.ableton.com/en/live-manual/12/live-instrument-reference/#simpler

Current UI research takeaway:

- Large instruments tend to separate core synthesis, modulation, effects, and browsing into focused pages or panels.
- Browsers/libraries are central for recall, tags, favorites, and fast auditioning.
- Short option sets should feel immediate, so segmented controls are preferred over dropdowns for choices like waveform and synced step rate.
- Long option sets still belong in dropdowns or browser lists.
- Timing and motion tools should be direct but focused: expose a small mode selector and visible lane markers before building a full editor page.
- The first screen should function as a production dashboard, not a decorative landing page: quick patch shaping, preset recall, randomization, and deeper-panel navigation should all be close at hand.
- Latest reference review points to low-risk polish before new engines: per-control modulation inspection, deeper preset metadata/audition, tighter UKG chop trimming, selective groove transforms, editable macro assignments, FX module presets, and genre-specific init templates.
- Larger engine tracks should stay deliberate: wavetable oscillator support, layered partials, motion sequencing lanes, advanced sampler slicing, character filter/drive models, FM/operator options, and vector morphing.

### What To Learn From ZENOLOGY

Useful ideas:

- Layered/partial-based sound design.
- Each layer has its own oscillator, filter, amplifier, envelopes, LFOs, and EQ or tone shaping.
- Preset-first workflow with a large browser.
- Hybrid sound sources instead of one synthesis method only.
- Performance controls for quickly changing a sound without editing every detail.
- Polished factory sounds covering both classic and modern electronic styles.

How this plugin should adapt those ideas:

- Eventually support multiple layers, but do not start with four full layers.
- Start with one synth layer, then add a second layer when the engine is stable.
- Let each layer be independently mixed, filtered, driven, and modulated.
- Build a strong preset browser later, but keep early preset storage simple.

### What To Learn From Serum

Useful ideas:

- Clean wavetable workflow.
- Visual feedback for modulation.
- Drag or assign modulation quickly.
- Wavetable position and warp controls as performance-friendly parameters.
- Strong effects section.
- Clear parameter organization.
- Deep sound design without hiding the basics.

How this plugin should adapt those ideas:

- Add wavetable synthesis after the subtractive core is working.
- Make modulation visible and understandable.
- Use modulation rings, small indicators, or value overlays so users can see what is moving.
- Add a modulation matrix only when there are enough sources and destinations to justify it.
- Treat wavetable import/editor features as future work, not MVP work.

### What To Learn From u-he

Useful ideas:

- Sound quality matters more than a giant feature checklist.
- Filters and oscillators should have character.
- Analog-modeled behavior can be expensive, so quality modes are useful.
- Modulation should be powerful but not make the default workflow slow.
- Effects should feel musical and integrated.
- Presets should be tagged and easy to explore.

How this plugin should adapt those ideas:

- Prioritize great filters, drive, envelopes, and gain staging.
- Add oversampling where distortion or non-linear filters need it.
- Consider normal and high-quality modes later.
- Keep the first synth page immediate, then put deep modulation behind tabs or panels.
- Avoid adding advanced engines before the simple engine sounds good.

### What To Learn From Korg

Useful ideas:

- Motion sequencing.
- Vector or XY performance control.
- Randomization that generates useful starting points.
- Multi-layer performances.
- Step lanes that can control pitch, timing, shape, and timbre independently.
- Hardware-like macro controls.
- Semi-modular patching ideas from instruments such as MS-20.

How this plugin should adapt those ideas:

- Add an XY motion pad later, with recordable Ableton automation.
- Add an intelligent randomizer that can randomize a whole preset or only one section.
- Add motion sequencing as a focused club-production tool rather than a giant workstation feature.
- Keep groove and timing editable per step where it directly improves UK garage, house, and tech-house patterns.
- Consider simple patch-style routing for modulation, but do not build a full modular environment first.

## Long-Term Product Shape

The dream version is a hybrid dance-music synth with:

- Layered sound generation.
- Analog-style oscillators.
- Wavetable oscillators.
- FM/phase-modulation options.
- Sub and noise sources.
- Character filters.
- Strong distortion and saturation.
- Visual modulation.
- Motion sequencing.
- Performance macros.
- Intelligent randomization.
- A polished preset browser.
- Production-ready effects.

Practical identity:

> A club-focused hybrid synth that combines fast bass/stab programming, visual modulation, aggressive drive, and motion-based sound design.

This should sit somewhere between a bass synth, wavetable synth, performance synth, and dirty analog-inspired instrument. It does not need to become a full workstation.

## Feature Tiering

The project should be built in tiers so it stays achievable.

### Tier 1: Playable Subtractive Synth

Goal: load in Ableton and make useful basses, plucks, and stabs.

- One layer.
- One or two oscillators.
- Basic waveforms.
- ADSR.
- Filter.
- Drive.
- Output gain.
- Simple UI.

### Tier 2: Serious Dance Synth

Goal: become musically useful for actual house, tech house, techno, and minimal tracks.

- Two oscillators.
- Sub oscillator.
- Noise source.
- Mono glide.
- More filter modes.
- Filter drive.
- LFO.
- Filter envelope.
- Basic effects.
- Presets.
- Better UI.

### Tier 3: Modern Hybrid Synth

Goal: start matching the workflow depth of modern synths.

- Wavetable oscillator.
- Sample import and slicing.
- Unison.
- Modulation matrix.
- Multiple envelopes.
- Multiple LFOs.
- Macros.
- XY performance pad.
- Internal pattern/piano-roll sequencer.
- Visual modulation indicators.
- Preset browser with tags.
- More effects.

### Tier 4: Signature Instrument

Goal: develop features that make the plugin distinct.

- Motion sequencer.
- Intelligent randomizer.
- Layer system.
- Sample/granular experiments if useful.
- FM/phase-modulation engine.
- Advanced distortion modes.
- High-quality filter models.
- Oversampling quality modes.
- User wavetable import.
- User preset packs.

## Recommended Technical Foundation

- Platform: macOS first.
- DAW target: Ableton Live.
- Plugin format: VST3 first.
- Optional later format: Audio Unit.
- Framework: JUCE.
- Build system: CMake.
- Language: C++20.
- Version control: Git and GitHub.
- Local validation: pluginval.
- Local host testing: Ableton Live and JUCE AudioPluginHost.

### Why This Stack

JUCE is the most practical starting point for a custom cross-platform audio plugin. It handles plugin wrappers, parameters, host integration, GUI, MIDI, audio buffers, preset state, and VST3 export.

VST3 is the best first target for Ableton on macOS. VST2 should be avoided for a new plugin because it is legacy and not a good distribution target. CLAP can be reconsidered later, but it should not be part of the first version.

## Product Principles

- Make the default sound audible and useful.
- Prefer a small number of high-impact controls over a giant matrix of options.
- Every visible control should be automatable if it affects the sound.
- Avoid clicks, zipper noise, and sudden volume jumps.
- Drive and distortion should feel central to the instrument, not like a small effect bolted on.
- Presets should be starting points for club production, not generic keyboard sounds.
- The UI should be fast to scan while writing music.
- The plugin should work well with Ableton automation, MIDI clips, racks, and macro workflows.
- Big reference-synth features should be added only when they serve the core workflow.
- The plugin should develop its own identity instead of becoming a checklist clone.

## Genre-Focused Sound Goals

### House

- Warm basses.
- Short plucks.
- Chord stabs.
- Smooth filter automation.
- Rounded saturation.

### Tech House

- Rolling mono basses.
- Percussive midrange blips.
- Tight envelopes.
- Filtered, driven saw and square tones.
- Simple macros for movement.

### Techno

- Aggressive drive.
- Resonant filter motion.
- Acid-style lines.
- Drones and noise textures.
- Dark, industrial modulation.

### Minimal

- Short clicks, pings, and blips.
- Subtle pitch/filter movement.
- Clean mono behavior.
- Precise envelope response.

## Plugin Type

Initial version:

- Instrument plugin.
- MIDI input.
- Stereo audio output.
- No audio input.
- No standalone app required at first.

Future option:

- Separate effect plugin version for distortion, filtering, rumble, and creative processing on existing audio.

## MVP Feature Set

The first useful version should include:

- One oscillator.
- Waveforms: saw, square, sine, triangle.
- Mono and poly modes.
- Basic glide for mono lines.
- Amp ADSR envelope.
- Low-pass filter.
- Filter cutoff and resonance.
- Filter envelope amount.
- Distortion or saturation.
- Output gain.
- Basic parameter smoothing.
- A simple custom UI.
- Preset/state saving through the host.
- VST3 build that loads in Ableton.

This MVP should be intentionally small. The goal is to get from MIDI to useful sound inside Ableton before adding advanced synthesis.

## Strong V1 Feature Set

After the MVP works, the first serious version should add:

- Second oscillator.
- Oscillator detune.
- Oscillator mix.
- Sub oscillator.
- Noise source.
- More filter modes: low-pass, high-pass, band-pass.
- Filter drive.
- Post-filter saturation.
- One LFO.
- LFO destinations: pitch, filter cutoff, amp, distortion amount.
- Filter envelope with independent ADSR or simplified decay envelope.
- Velocity sensitivity.
- Pitch bend.
- Mod wheel mapping.
- Factory preset bank.
- Basic output meter.
- Undo support if practical through JUCE parameter handling.

## Future Expansion Ideas

Do not build these first, but keep the architecture open for them:

- Wavetable oscillator.
- Wavetable position, bend, fold, sync, and warp-style controls.
- User wavetable import.
- Wavetable editor.
- Multi-engine oscillator modes.
- Sample oscillator.
- Granular oscillator.
- Spectral/resynthesis experiments.
- FM or phase modulation.
- Oscillator sync.
- Ring modulation.
- Advanced unison modes and supersaw-style spread.
- Layer or partial system.
- Per-layer filters and envelopes.
- Chord memory.
- Step sequencer or modulation sequencer.
- Motion sequencer with independent lanes.
- XY motion controller.
- Randomization with musical constraints.
- Macro controls.
- Modulation matrix.
- Modulation source browser.
- Modulation amount indicators around controls.
- Built-in delay.
- Built-in chorus.
- Built-in reverb.
- Built-in phaser.
- Built-in compressor.
- Built-in EQ.
- Multi-band distortion.
- Reorderable effects rack.
- Oversampling for high-drive modes.
- Preset browser with tags.
- Favorite presets.
- Factory sound packs.
- Skin/theme options.
- MPE support.
- Separate audio effect version.

## Proposed Signal Path

MVP signal path:

```text
MIDI input
  -> voice allocation
  -> oscillator
  -> filter
  -> distortion/saturation
  -> amp envelope
  -> output gain
  -> stereo output
```

Possible V1 signal path:

```text
MIDI input
  -> voice allocation
  -> oscillator 1
  -> oscillator 2
  -> sub oscillator
  -> noise
  -> mixer
  -> pre-filter drive
  -> filter
  -> post-filter saturation
  -> amp envelope
  -> output trim
  -> stereo output
```

Open question: whether drive should be before the filter, after the filter, or both. For techno and acid sounds, pre-filter drive is very useful. For modern controlled production, post-filter saturation with gain compensation is also useful.

Recommended approach:

- MVP: one post-filter saturation stage.
- V1: add pre-filter drive and post-filter output saturation.

Possible long-term layered signal path:

```text
MIDI input
  -> performance controls
  -> internal pattern/piano-roll sequencer
  -> modulation system
  -> layer 1 source engine
       -> oscillator / wavetable / FM / sample mode
       -> layer filter
       -> layer drive
       -> layer amp
  -> layer 2 source engine
       -> oscillator / wavetable / FM / sample mode
       -> layer filter
       -> layer drive
       -> layer amp
  -> sample/audio layer
       -> sample player / slicer / granular mode
       -> sample envelope
       -> sample filter
       -> sample drive
  -> layer mixer
  -> global filter or tone stage
  -> effects rack
  -> limiter or safety clipper
  -> output
```

The architecture should not require this full path on day one, but early code should avoid assumptions that make layering impossible later.

## Synthesis Engine Brainstorm

### Oscillator Engines

Start with:

- Basic virtual analog oscillator.
- Sine, triangle, saw, square, pulse, and noise.
- Stable pitch.
- Low aliasing where practical.

Add later:

- Wavetable oscillator.
- Sub oscillator.
- FM/phase modulation.
- Sync.
- Ring modulation.
- Oscillator warp/shaping.
- Sample playback.
- Granular playback.

Recommended path:

- Build a strong virtual analog engine first.
- Add wavetable support before sample/granular support.
- Treat sample and granular engines as experiments until the core synth is solid.

### Sampler Engine

Start later with:

- One sample slot.
- WAV/AIFF loading.
- One-shot playback.
- Start/end markers.
- Reverse.
- Pitch and gain.
- Sample envelope.

Add later:

- Slice detection.
- Slice mapping.
- Looping.
- Time stretch.
- Granular playback.
- Per-slice modulation.
- Random sample and slice selection.

The sampler should share as much downstream processing as possible with the synth layers: filter, drive, modulation, effects, and output safety.

### Filters

Start with:

- Clean low-pass filter.
- Cutoff and resonance.
- Filter envelope amount.

Add later:

- High-pass, band-pass, notch.
- Ladder-style low-pass.
- Acid-style filter.
- State-variable filter.
- Comb filter.
- Drive into filter.
- Filter key tracking.
- Per-layer filters.
- High-quality mode for expensive filters.

### Distortion And Saturation

Distortion should be a core identity feature.

Possible modes:

- Soft clip.
- Hard clip.
- Tube-style saturation.
- Foldback.
- Diode-style asymmetry.
- Bit crush.
- Downsample.
- Multiband drive.
- Filtered drive.

Important details:

- Add gain compensation.
- Add dry/wet mix.
- Add oversampling for aggressive modes.
- Prevent dangerous output spikes.

### Modulation

Start with:

- Amp envelope.
- Filter envelope.
- One LFO.

Add later:

- Multiple envelopes.
- Multiple LFOs.
- Step LFO.
- Random source.
- Sample and hold.
- Key tracking.
- Velocity.
- Aftertouch.
- Mod wheel.
- Pitch bend.
- Macro knobs.
- XY pad.
- Motion sequencer lanes.
- Modulation matrix.

Visual goal:

- Users should be able to tell which parameters are being modulated.
- Modulation depth should be visible near the destination.
- Deep routing should be available, but the default workflow should stay immediate.

### Performance Controls

The plugin should eventually have performance controls that are useful in Ableton automation:

- 4 to 8 macro knobs.
- XY pad.
- Randomize button.
- Randomization panel.
- Variation button.
- Scene or snapshot morphing.
- Mono/poly toggle.
- Glide amount.
- Energy or drive macro.
- Motion amount macro.

The performance controls should be designed around recording automation into Ableton.

## Sample And Audio Lab

The plugin should eventually allow users to load audio material, cut it, reshape it, randomize it, and use it as part of a synth patch. This should feel like a creative sampler built into the instrument, not a full DAW replacement.

### Core Workflow

Desired workflow:

1. Load or drag in an audio file.
2. Detect useful slices or manually define start/end points.
3. Audition slices from MIDI notes or the internal sequencer.
4. Change pitch, envelope, filter, drive, and effects.
5. Randomize slice order, start points, pitch, or processing.
6. Save the result as part of a preset.

Supported audio goals:

- One-shots.
- Vocal chops.
- Percussion hits.
- Noise hits.
- Texture loops.
- Riser/downlifter material.
- Found-sound stabs.
- Resampled synth sounds.

### MVP Sampler Scope

Do not build this before the basic synth works, but the first sampler version should include:

- Load WAV/AIFF.
- Drag-and-drop audio into the plugin.
- Waveform display.
- Start and end markers.
- One-shot playback.
- Reverse.
- Pitch transpose and fine tune.
- Sample envelope.
- Sample gain.
- Send sample through the same filter, drive, and output path.

### Later Sampler Scope

Future features:

- Automatic transient slicing.
- Grid slicing.
- Manual slice markers.
- Slice-to-key mapping.
- Loop mode.
- Crossfade looping.
- Time-stretching.
- Granular playback.
- Random start position.
- Random slice selection.
- Per-slice pitch, envelope, filter, pan, and effects send.
- Slice export or bounce later if practical.

### Random Audio Loading

The randomization system should also work with audio:

- Pick a random sample from a user folder.
- Pick a random slice from the current sample.
- Randomize start/end points.
- Randomize reverse, pitch, and envelope.
- Randomize sample processing while preserving level safety.
- Combine sample randomization with synth-layer randomization.

This needs user-controlled scope. A user should be able to lock the loaded sample but randomize processing, or keep the processing but randomize slices.

### Sample Guardrails

- Avoid loading huge files fully into memory without limits.
- Resample or prepare audio outside the real-time audio thread.
- Never decode files on the audio thread.
- Normalize preview levels conservatively.
- Keep output safety active for unpredictable samples.
- Save relative file references when possible, but consider embedded/cached audio later for portability.

## Piano Roll And Pattern Sequencer

Ableton already has the main piano roll, so the plugin does not need to replace Ableton's MIDI editor. The useful feature is an internal pattern/piano-roll area for quickly generating riffs, basslines, acid patterns, arps, stabs, and modulation rhythms inside the plugin.

### Core Workflow

- Draw notes on a small piano-roll grid.
- Trigger the pattern from held MIDI notes or host transport.
- Choose pattern length.
- Set rate, swing, octave, and gate.
- Randomize notes, rhythm, velocity, and probability.
- Send the pattern to the synth engine and sampler engine.
- Automate pattern-related controls in Ableton.

### First Pattern Scope

- 16-step lane.
- Note offset per step.
- Gate on/off.
- Velocity per step.
- Probability per step.
- Octave range.
- Scale lock.
- Root note.
- Swing.
- Rate synced to host tempo.

### Later Piano-Roll Scope

- True piano-roll editing.
- Multiple pattern slots.
- Pattern chaining.
- Per-step modulation.
- Per-step slide/accent for acid lines.
- Random bassline generator.
- Euclidean rhythm tools.
- Export pattern as MIDI if practical.
- Drag MIDI pattern into Ableton if practical through plugin-host supported mechanisms.

### Pattern Guardrails

- The plugin should always respond to normal MIDI input even if the internal sequencer is off.
- The sequencer must follow Ableton tempo and transport when sync is enabled.
- Generated notes should avoid stuck notes.
- Scale lock and note range controls should keep random patterns musically useful.

## Effects Area

The plugin should have a dedicated effects area for shaping synth and sample sounds after generation.

### First Effects Scope

- Tone cleanup with tilt and low cut.
- Distortion/saturation.
- Phaser.
- Chorus.
- Delay.
- Reverb.
- Output trim.
- Guard output safety clipper.

### Later Effects Scope

- Reorderable effects rack.
- Per-effect dry/wet.
- Per-effect bypass.
- Flanger.
- Compressor.
- EQ.
- Bitcrush/downsample.
- Multiband distortion.
- Rumble/low-end generator for techno workflows.
- Send-style effects routing.

### Effects UI

The effects area should be a clear tab or panel:

- Each effect has an enable switch.
- Important controls are visible first.
- Advanced controls can expand.
- The signal order should be visible.
- Presets should store effect state.
- Randomization can target one effect or the whole rack.

The next FX/UI direction is tracked in [docs/UI_EFFECTS_ROADMAP.md](./docs/UI_EFFECTS_ROADMAP.md). The important shift is to move away from a fixed grid that shows every module at once and toward an addable rack list with a focused selected-effect editor.

## Randomization Lab

The plugin should have a dedicated randomization area for making new sounds quickly. This should be more useful than a single dice button. The user should be able to guide what changes, how extreme the changes are, and whether the result stays musical.

### Core Idea

Temporary feature name:

> Sound Lab

Purpose:

- Generate new patches.
- Mutate the current patch.
- Create small variations.
- Randomize only one section.
- Keep randomization musically constrained.
- Save good results as presets.

The best version should feel like a creative partner for finding basses, stabs, plucks, noise hits, drones, and weird techno textures.

### Randomization Controls

Core controls:

- `Generate`: create a new sound from the selected recipe.
- `Mutate`: change the current sound while preserving its general identity.
- `Variation`: make a subtle alternate version.
- `Undo`: return to the previous sound.
- `A/B`: compare the previous and current random result.
- `Save`: store the current result as a preset.

Intensity controls:

- Amount: how much parameters are allowed to move.
- Chaos: how unusual or extreme the result can be.
- Brightness bias: darker to brighter.
- Drive bias: clean to aggressive.
- Motion bias: static to heavily modulated.
- Bass safety: keep low-end sounds controlled.
- Level safety: prevent overly loud output.

Current section locks:

- Lock pitch/tuning.
- Lock envelopes.
- Lock filter.
- Lock source mix.
- Lock sample.
- Lock effects.
- Lock output gain.
- Lock sequencer.

Future section locks:

- Lock drive.
- Lock modulation.
- Lock macros.

Section randomize buttons:

- Randomize OSC.
- Randomize FILTER.
- Randomize DRIVE.
- Randomize ENV.
- Randomize LFO/MOD.
- Randomize FX.
- Randomize MACROS.

### Randomization Recipes

Recipes should make randomization genre-aware and goal-aware:

- Deep House Bass.
- Rolling Tech Bass.
- Acid Line.
- Minimal Blip.
- Dub Techno Chord.
- Industrial Drone.
- Percussive Tick.
- Dark Stab.
- Rave Lead.
- Noise FX.
- Evolving Motion.
- Clean Init Variation.

Each recipe should define useful ranges for oscillators, envelopes, filters, drive, modulation, and output level.

### Musical Guardrails

Randomization should avoid unusable results by default:

- Keep oscillator pitch in reasonable musical ranges.
- Avoid all source levels being zero.
- Avoid amp envelope settings that are accidentally silent.
- Avoid resonance and drive combinations that create dangerous peaks.
- Avoid random output gain spikes.
- Bias bass recipes toward mono and controlled low end.
- Bias pluck recipes toward shorter envelopes.
- Bias drone recipes toward longer envelopes and more modulation.
- Keep parameter smoothing active after random changes.

An advanced mode can allow more extreme results:

- Wider tuning offsets.
- More resonance.
- More drive.
- Experimental modulation assignments.
- Unstable motion sequencer values.
- More random effects.

### Randomization Workflow

Recommended workflow:

1. Pick a recipe.
2. Choose sections that are allowed to change.
3. Set amount and chaos.
4. Generate or mutate.
5. Use A/B or undo if the result is worse.
6. Tweak manually.
7. Save as a preset if useful.

Ableton-focused workflow:

- Record macro automation after generating a sound.
- Use the randomizer to create preset variations for a track.
- Use section locks to keep a bassline's pitch/envelope stable while changing tone.
- Save generated sounds into a project-specific preset folder later.

### UI Layout For Randomization

Possible location:

- A `PERFORM` or `LAB` tab.
- A compact randomization strip in the top bar.
- A larger drawer/panel that opens when the user clicks a dice or lab icon.

Suggested panel sections:

- Recipe selector.
- Generate/mutate/variation controls.
- Amount and chaos sliders.
- Section locks.
- Character bias controls.
- Recent results/history.
- Save preset button.

The randomization area should not take over the whole synth. It should be easy to open, generate ideas, close, and keep editing.

### Implementation Notes

The randomizer should not blindly assign every parameter a uniform random value. It should use musical ranges, weighted choices, and recipe-specific constraints.

Recommended implementation:

- Add a randomization engine that operates on parameter snapshots.
- Use recipe definitions instead of hard-coded one-off random calls.
- Keep randomization off the audio thread.
- Apply randomized values through the same parameter system used by the UI and host automation.
- Store a small history stack for undo/A/B.
- Give each generated result a seed so it can be reproduced during debugging.
- Mark randomizer-only metadata separately from automatable synth parameters.

Potential data model:

```text
RandomizationRecipe
  -> allowed parameter groups
  -> value ranges
  -> weighted choices
  -> correlation rules
  -> safety rules
  -> macro assignment rules
```

Examples of correlation rules:

- More drive usually lowers output gain.
- Short amp decay often pairs with higher filter envelope amount.
- Bass recipes avoid wide stereo spread in the sub range.
- Acid recipes favor saw/square waves, resonance, glide, and filter envelope movement.
- Drone recipes favor slow LFOs, longer envelopes, and more effects.

## UI Brainstorm

The interface should be practical and music-production focused, not a marketing page or a decorative toy.

### Overall Feel

- Dark, clean, high contrast.
- Ableton-friendly.
- Dense enough for repeated studio use.
- Clear sections with labels.
- No unnecessary animation.
- No large decorative artwork in the main editor.
- Controls should feel precise and stable.

### Layout Idea

Top bar:

- Plugin name.
- Preset selector.
- Previous/next preset buttons.
- Save preset button later.
- Mono/poly toggle.
- Output meter.
- Randomize button later.
- Settings button.

Possible first-page layout:

- Oscillator section.
- Mixer section.
- Filter section.
- Drive section.
- Output section.

Lower row:

- Amp envelope.
- Filter envelope.
- LFO.
- Macro controls.

Possible long-term tab layout:

- SYNTH: oscillators, mixer, filter, drive, amp.
- SAMPLE: audio import, slicing, sample playback, sample envelope.
- MOD: envelopes, LFOs, modulation matrix, motion sequencer.
- SEQ: piano-roll/pattern sequencer, arps, generated basslines.
- FX: effects rack and routing.
- PERFORM: macros, XY pad, snapshots.
- LAB: randomization, recipes, mutation controls, section locks.
- BROWSER: presets, tags, favorites.

Current implemented panel layout:

- HOME: engine choices, core tone shaping, unison width, performance macros, randomization, and preset load/save in one dashboard.
- SYNTH: oscillator waveform segment, mono toggle, unison controls, core filter/drive/output/amp controls.
- LAB: recipe dropdown, generate, mutate, variation, undo, section locks, status, and randomization bias controls.
- SAMPLE: sample load/clear, random cut, sample enable/reverse, cut range, pitch, gain, and mix.
- SEQ: sequencer enable, segmented rate selector, pattern presets, copy/random/clear, groove/performance controls, and the 16-step grid.
- FX: bypassable Tone, Dist, Phaser, Chorus, Delay, Reverb, and Guard modules.
- LIBRARY: preset category, preset name entry, save, filter by favorites/recent/source/category, previous/next, preset selector, favorite toggle, load, refresh, and preset folder status.
- TOP BAR: panel tabs plus a compact stereo output meter.
- BOTTOM BAR: persistent piano keyboard for mouse auditioning patches.

Suggested first UI size:

- 900 x 560 px or similar.
- Fixed size at first.
- Resizable later only if the layout remains clean.

### Control Style

- Knobs for cutoff, resonance, drive, gain, detune, mix, envelope amount.
- Sliders or compact envelope widgets for ADSR.
- Segmented controls for waveform and filter type.
- Toggle buttons for mono/poly, sync, retrigger, and tempo sync.
- Compact output metering in the top bar.
- Clear numeric readouts where exact values matter.
- Modulation rings or overlays around controls later.
- A small oscilloscope or waveform display later.
- A wavetable display once wavetable synthesis exists.
- A sample waveform display once sample import exists.
- A compact piano-roll grid once the internal sequencer exists.
- A persistent bottom piano keyboard for quick note auditioning.
- An XY pad once performance macros exist.

### Visual Sections

Possible first-pass sections:

- OSC
- MIXER
- SAMPLE
- FILTER
- DRIVE
- AMP ENV
- FILTER ENV
- LFO
- SEQ
- MOD MATRIX
- MACROS
- LAB
- FX
- OUTPUT

The UI should avoid nested cards. Use simple panels, separators, or full-width bands. Rounded corners should be subtle.

## Parameter Brainstorm

Initial parameter IDs should be stable because changing them later can break saved Ableton projects and presets.

MVP parameters:

- `osc_wave`
- `osc_octave`
- `osc_tune`
- `osc1_level`
- `osc2_wave`
- `osc2_octave`
- `osc2_tune`
- `osc2_level`
- `sub_level`
- `noise_level`
- `amp_attack`
- `amp_decay`
- `amp_sustain`
- `amp_release`
- `filter_cutoff`
- `filter_resonance`
- `filter_env_amount`
- `filter_mode`
- `drive_amount`
- `output_gain`
- `mono_mode`
- `glide_time`
- `unison_voices`
- `unison_detune`
- `unison_blend`
- `unison_spread`
- `macro_1`
- `macro_2`
- `macro_3`
- `macro_4`

Early randomization parameters and metadata:

- `random_amount`
- `random_chaos`
- `random_brightness_bias`
- `random_drive_bias`
- `random_motion_bias`
- `random_recipe`
- `random_lock_pitch`
- `random_lock_envelope`
- `random_lock_filter`
- `random_lock_source`
- `random_lock_sample`
- `random_lock_fx`
- `random_lock_output`
- `random_lock_sequencer`
- Random seed should be stored as preset metadata, not necessarily as an automatable audio parameter.

V1 parameters:

- Keep `osc_wave`, `osc_octave`, and `osc_tune` as oscillator 1 IDs for backward compatibility.
- `filter_drive`
- `lfo_rate`
- `lfo_depth`
- `lfo_shape`
- `lfo_target`
- `velocity_to_amp`
- `velocity_to_filter`
- `pitch_bend_range`

Long-term parameter groups:

- Layer enable, level, pan, transpose, tune.
- Oscillator engine type.
- Wavetable position.
- Wavetable warp amount.
- Wavetable warp mode.
- Advanced unison modes.
- FM amount.
- FM ratio.
- Sample slot enable.
- Sample start.
- Sample end.
- Sample slice index.
- Sample reverse.
- Sample transpose.
- Sample fine tune.
- Sample gain.
- Sample envelope attack/decay/sustain/release.
- Sequencer enable.
- Sequencer rate.
- Sequencer swing.
- Sequencer accent.
- Sequencer octave.
- Sequencer probability.
- Sequencer length.
- Sequencer root note.
- Sequencer scale.
- Sequencer octave range.
- Sequencer randomize amount.
- Filter model.
- Filter key tracking.
- Distortion mode.
- Distortion tone.
- Distortion mix.
- Macro 1-8.
- XY X.
- XY Y.
- Motion sequencer rate.
- Motion sequencer depth.
- Randomize amount.
- Randomize chaos.
- Randomize brightness bias.
- Randomize drive bias.
- Randomize motion bias.

Stable parameter planning becomes more important as the synth grows. Before any public release, decide which parameters are permanent and which experimental parameters are hidden from host automation.

## Preset Brainstorm

Initial presets should be simple but musically useful:

- Init
- Deep House Sub
- Rolling Tech Bass
- Acid Pluck
- Hollow Minor Stab
- Minimal Click Bass
- Dark Saw Lead
- Resonant Techno Pulse
- Noise Tick
- Warm Triangle Bass
- Driven Square Bass
- Filtered Rave Stab

Preset categories:

- Bass
- Stab
- Lead
- Pluck
- Percussive
- Drone
- FX

Expanded future categories:

- Acid.
- Chord.
- Rumble.
- Sequence.
- Motion.
- Wavetable.
- Industrial.
- Dub Techno.
- Minimal.
- Raw Analog.
- Digital.

Preset browser goals:

- Tag by category, character, and genre.
- Mark favorites.
- Search by name.
- Include macro descriptions.
- Include init templates for different synthesis modes.
- Avoid requiring the browser for basic patch creation.

Current browser slice:

- User presets remain in `~/Library/Application Support/Nate VST/Presets`.
- Factory presets are scanned separately from `~/Library/Application Support/Nate VST/Factory Presets`.
- Favorites and recent presets are stored in `~/Library/Application Support/Nate VST/Library.xml`.
- LIBRARY can filter by all presets, favorites, recent presets, user presets, factory presets, or saved category.

## Code Architecture

Proposed source organization:

```text
Source/
  PluginProcessor.h
  PluginProcessor.cpp
  PluginEditor.h
  PluginEditor.cpp
  Parameters.h
  Parameters.cpp
  Synth/
    SynthEngine.h
    SynthEngine.cpp
    Voice.h
    Voice.cpp
    Layer.h
    Layer.cpp
    Oscillator.h
    Oscillator.cpp
    WavetableOscillator.h
    WavetableOscillator.cpp
    Sampler/
      SamplePlayer.h
      SamplePlayer.cpp
      SampleBuffer.h
      SampleBuffer.cpp
      SliceMap.h
      SliceMap.cpp
    Filter.h
    Filter.cpp
    Envelope.h
    Envelope.cpp
    Distortion.h
    Distortion.cpp
    Modulation/
      ModSource.h
      ModMatrix.h
      Lfo.h
      MotionSequencer.h
    Sequencer/
      PatternSequencer.h
      PatternSequencer.cpp
      Pattern.h
      Pattern.cpp
    Effects/
      EffectsRack.h
      EffectsRack.cpp
      Delay.h
      Delay.cpp
      Chorus.h
      Chorus.cpp
      Reverb.h
      Reverb.cpp
      DistortionEffect.h
      DistortionEffect.cpp
  Presets/
    PresetManager.h
    PresetManager.cpp
  Randomization/
    Randomizer.h
    Randomizer.cpp
    RandomizationRecipe.h
    RandomizationRecipe.cpp
    RandomizationHistory.h
    RandomizationHistory.cpp
  UI/
    LookAndFeel.h
    LookAndFeel.cpp
    Knob.h
    Knob.cpp
    Section.h
    Section.cpp
Tests/
Resources/
CMakeLists.txt
README.md
PLAN.md
```

The exact structure may change depending on whether the project starts from plain JUCE or Pamplejuce.

Recommended architecture direction:

- Implement one layer first, but represent it as a `Layer` concept so a second layer can be added later without rewriting every voice.
- Keep oscillator engines interchangeable behind a small interface or variant-based engine wrapper.
- Keep modulation data separate from audio rendering code.
- Keep effects as a post-synth rack, not embedded directly inside the oscillator or voice.
- Keep preset serialization versioned from the beginning.
- Keep randomization operating on parameter snapshots, not directly on DSP objects.
- Treat UI controls as views over parameters, not as owners of synth state.
- Use `docs/MODULATION_WORKFLOW.md` as the working design for the first macro, LFO, and modulation-matrix pass.

### Engineering Rules

- Keep DSP independent from UI.
- Avoid heap allocation on the audio thread.
- Avoid locks on the audio thread.
- Smooth parameter changes that affect audio.
- Clamp and protect output levels.
- Use stable parameter IDs.
- Keep the first version simple enough to debug.
- Add tests around parameter ranges, state, and DSP helpers where useful.

## Development Milestones

### Milestone 0: Project Setup

- Initialize git.
- Add `.gitignore`.
- Add `README.md`.
- Add this `PLAN.md`.
- Choose license direction.
- Decide whether to use plain JUCE or Pamplejuce.

### Milestone 1: Empty Plugin Build

- Scaffold JUCE/CMake project.
- Build VST3 on macOS.
- Confirm the plugin binary is generated.
- Install or copy to `~/Library/Audio/Plug-Ins/VST3`.
- Confirm Ableton scans it.

### Milestone 2: First Sound

- Add MIDI note handling.
- Add one oscillator.
- Add amp envelope.
- Confirm MIDI clip in Ableton triggers sound.
- Confirm plugin state saves in an Ableton Live Set.

### Milestone 3: Basic Sound Design

- Add waveform selection.
- Add filter cutoff/resonance.
- Add drive.
- Add output gain.
- Add parameter smoothing.
- Confirm automation works in Ableton.

### Milestone 4: First UI

- Replace default editor with custom UI.
- Add controls for all MVP parameters.
- Make layout stable and readable.
- Test at common display scaling settings.

### Milestone 5: V1 Synth

- Add oscillator 2.
- Add sub/noise.
- Add LFO.
- Add mono glide.
- Add filter envelope.
- Add preset list.

### Milestone 6: Randomization Lab

- Add parameter snapshot support.
- Add first randomization recipes.
- Add amount and chaos controls.
- Add section locks.
- Add generate, mutate, variation, and undo.
- Add safety rules for output gain, resonance, drive, and silent patches.
- Add UI panel or tab for randomization.
- Save generated patches as presets.

### Milestone 7: Effects Area

- Add effects rack model.
- Add bypassable distortion, chorus, delay, and reverb.
- Add per-effect dry/wet where useful.
- Add output trim and safety clipper.
- Add FX tab or panel.
- Allow randomization to target effects.

### Milestone 8: Sample And Audio Lab

- Add audio file loading off the audio thread.
- Add one sample slot.
- Add waveform display.
- Add start/end editing.
- Add one-shot playback from MIDI.
- Add reverse, pitch, gain, and sample envelope.
- Route sample playback through filter, drive, effects, and output safety.
- Add first random sample/slice controls.

### Milestone 9: Piano Roll And Pattern Sequencer

- Add host-synced 16-step sequencer.
- Add note, gate, velocity, and probability per step.
- Add root, scale, octave range, rate, and swing.
- Add random pattern generation.
- Add sequencer tab or compact piano-roll editor.
- Confirm normal MIDI input still works when sequencer is off.

### Milestone 10: Validation and Release Prep

- Run pluginval.
- Test in Ableton.
- Test repeated load/save of Live Sets.
- Check CPU usage.
- Check output clipping behavior.
- Prepare GitHub Actions build if useful.

## GitHub Plan

Recommended early repo setup:

- `main` branch protected later.
- Feature branches for larger changes.
- Small commits with clear messages.
- GitHub Issues for larger feature ideas.
- GitHub Actions after the first local build works.

Suggested first commits:

1. `Add project plan`
2. `Scaffold JUCE VST3 project`
3. `Add initial synth voice`
4. `Add filter and drive controls`
5. `Add custom plugin editor`

## Local Tooling Needed

Likely requirements:

- Xcode.
- Xcode command line tools.
- CMake.
- Ninja or Make.
- JUCE or a JUCE-based template.
- pluginval.
- Ableton Live.

Optional:

- JUCE AudioPluginHost.
- GitHub CLI.
- clang-format.
- cppcheck or clang-tidy later.

## Open Decisions

### Project Name

Still undecided. Placeholder names:

- Nate VST
- PulseForge
- Circuit Floor
- MonoGrid
- DriveCell
- Niteform
- Voltline

Need a name that is easy to search, not already taken by another plugin, and not too genre-locked.

### Plain JUCE vs Pamplejuce

Plain JUCE:

- Smaller and easier to understand from scratch.
- Less template machinery.
- More manual setup for validation and CI.

Pamplejuce:

- More complete starting point.
- Includes modern CMake patterns and plugin validation workflows.
- More files and conventions upfront.

Recommendation:

- Use Pamplejuce if the goal is to move quickly toward a serious plugin project.
- Use plain JUCE if the goal is to learn every piece from scratch.

### UI Style

Possible directions:

- Ableton-adjacent utility synth: clean, restrained, fast.
- Dark techno instrument: higher contrast, sharper visual identity.
- Hardware-inspired: knobs and panels, but not skeuomorphic.

Recommendation:

- Start with Ableton-adjacent utility design.
- Add stronger visual identity once the sound engine is real.

### Synthesis Identity

Possible identities:

- Classic subtractive synth with strong drive.
- Wavetable synth for modern movement.
- Bass and acid-focused mono synth.
- Hybrid synth with simple oscillators plus heavy modulation.

Recommendation:

- Start as a subtractive synth with strong drive and modulation.
- Add wavetable or FM later only after the core instrument is stable.

### Commercial Intent

This matters for framework licensing and distribution.

Options:

- Personal open-source project.
- Private learning project.
- Future commercial plugin.

Recommendation:

- Treat it as a serious private project now.
- Keep license decisions explicit before public release.
- If closed-source commercial distribution becomes likely, revisit JUCE licensing early.

## Non-Goals For The First Build

- VST2 support.
- Windows support.
- CLAP support.
- Standalone app.
- Full preset browser.
- Wavetable editor.
- Complex modulation matrix.
- Skinnable UI.
- Installer package.
- Copy protection or licensing system.

These can all be reconsidered later. They are distractions before the synth makes useful sound in Ableton.

## Immediate Next Steps

1. Validate the current SEQ chord-memory and UI declutter pass in Ableton with house, UKG, tech-house, minimal, and techno preset workflows.
2. Add UKG chop tightening: zero-cross snap, tiny fade guards, and per-slice audition polish.
3. Add selective groove transforms for straight anchors, swung ghosts, late stabs, and vocal push.
4. Add a per-control modulation inspector so users can see source, depth, polarity, bypass, and delete actions without scanning the full matrix.
5. Add FX module presets and clearer per-module preset/reorder affordances.
6. Expand browser metadata and audition workflow with richer tags, ratings/favorites polish, and macro preview values.
7. Start larger engine work only after the current subtractive/sampler/SEQ/FX workflow feels strong in real sessions.
