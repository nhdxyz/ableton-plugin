# House VST Competitor Research

Research date: 2026-06-29

Purpose: compare Nate VST against current large synth, ROMpler, sampler, motion, and club-production tools, then turn the gaps into house-music-specific backlog direction. The codebase remains the source of truth; this document only records what is implemented, what is partial, and what is missing.

The goal is not to clone any product, UI, preset, sample, wavetable, DSP, or brand identity. The useful direction is to learn broad workflow patterns and build original, legal source material for house, UK garage, tech house, minimal, and techno production.

## References Reviewed

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- reFX NEXUS5: https://refx.com/nexus/
- Roland ZENOLOGY Pro: https://www.roland.com/us/products/rc_zenology_pro/
- KORG Collection: https://www.korg.com/us/products/software/korg_collection/
- KORG wavestate native: https://www.korg.com/us/products/software/wavestate_native/
- KORG modwave native: https://www.korg.com/us/products/software/modwave_native/
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Native Instruments MASSIVE X: https://www.native-instruments.com/en/products/komplete/synths/massive-x/
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- UVI Falcon: https://www.uvi.net/falcon
- FabFilter Saturn 2: https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- Cableguys ShaperBox 3: https://www.cableguys.com/shaperbox
- Ableton Live 12 instrument reference: https://www.ableton.com/en/live-manual/12/live-instrument-reference/
- Ableton Live 12 MIDI tools: https://www.ableton.com/en/live-manual/12/midi-tools/
- Serato Sample: https://serato.com/sample
- Scaler 3: https://scalermusic.com/products/scaler-3/
- Xfer Cthulhu: https://xferrecords.com/products/cthulhu
- XLN Audio XO: https://www.xlnaudio.com/products/xo
- Sonic Academy KICK 3: https://www.sonicacademy.com/products/kick-3
- FabFilter Pro-L 2: https://www.fabfilter.com/products/pro-l-2-limiter-plug-in

## Large VST Benchmark Addendum

This deeper pass focused on large current products the project explicitly cares about: Serum, NEXUS, ZENOLOGY, KORG, plus adjacent leaders that shape user expectations. The important finding is that the top products are converging around hybrid source engines, curated content ecosystems, visual modulation, and in-plugin idea generation. For Nate VST, the winning angle should stay narrower: be faster and more opinionated for house than the broad tools are.

### Serum 2 And Modern Hybrid Synths

Serum 2 raises the bar beyond classic wavetable synthesis. Current official material describes wavetable, sample, multisample, granular, and spectral oscillator modes, multiple oscillators, expanded modulation, macro control, clip sequencing, arp tools, scale/swing behavior, and MIDI output. Pigments, Phase Plant, Massive X, Vital, and Falcon reinforce the same trend: producers expect source diversity, deep modulation, and visual routing in one instrument.

Implication for Nate VST:

- The current code has sine/saw/square/triangle/generated wavetable plus procedural organ and house-piano source modes. That is a good identity pass, but it is not yet competitive source depth.
- The next source work should prioritize true wavetable import/playback, richer warp modes, focused FM/operator color, drawbar organ controls, and original/legal digital-PCM-style house chord/piano material.
- Avoid trying to out-modular Phase Plant. The better product move is a faster house source rack: Sub, Body, Character/Stab, Transient/Noise, and Chop layers with mutes, solos, blend, copy, and macro intent.

### NEXUS 5 And Preset Ecosystems

NEXUS 5 is now much more than a preset player. Official product/manual material describes a hybrid VA/sample engine, large oscillator/layer counts, sampler, wavetable, time-stretch, grain/cloud/FM-style generation, user sample and wavetable import, sample slicing/looping/crossfades, SFZ/SF2-style import, per-layer/global arp tools, MIDI import/export for patterns, macros, a modulation matrix, and a large FX suite. Its biggest advantage remains scale: a large expansion ecosystem, fast librarian, favorites, filters, wildcard search, folders, and cloud delivery.

Implication for Nate VST:

- Nate VST should not compete by raw preset count. It should compete by house usefulness per preset.
- Presets should become relationship-aware: compatible basses, chords, stabs, chops, FX tools, key/BPM, sequence intent, macro intent, and suggested Ableton clip use.
- The Library now has metadata, smart crates, ratings/favorites, rendered preview cache/playback, visible-row preview warming, preview level badges, compact-width browser rows, and 96 factory presets. Missing next: fully background preview generation, waveform preview badges, construction-kit browsing, dependency/relink warnings, pack import/export, and user crate editing.

### ZENOLOGY And Roland-Style Layered Tones

ZENOLOGY Pro is a benchmark for partial/layer architecture, PCM plus virtual analog source material, step LFOs, large effect coverage, model expansions, curated sound packs, and hardware/cloud continuity. Its strength is breadth and history, not house-specific flow.

Implication for Nate VST:

- A lightweight layer architecture matters. Even two focused layers with source/filter/amp/pan/macro blend would unlock a lot of house patches: sub plus rubber top, piano attack plus organ body, stab plus noise transient, or chop plus FX tail.
- Step-LFO and lane-based motion remain high-value because house producers use repeated motion as arrangement glue.
- Hardware portability is not a near-term requirement; consistent Ableton automation, preset state, preview state, and release validation matter more.

### KORG Collection, Gadget, wavestate, And modwave

KORG's software ecosystem shows two useful patterns. Collection is a deep instrument library built around recognizable engines and PCM character. Gadget is a production-workflow library with many focused devices, searchable programs, favorites, genre starts, chord/arp play helpers, and immediate role-based sounds. wavestate/modwave highlight lane-based motion, sample/wave sequencing, Kaoss-style performance control, and movement that is visible while editing.

Implication for Nate VST:

- Nate VST needs the "one unified house instrument" version of this: one browser, one macro standard, one expansion format, and patches organized around track-building roles.
- The sequencer already has scenes, grooves, chord colors, ratchets, first-pass odd/even/fill conditions, a first saved/exported Slide lane with live/export legato overlap, a first Bass Contour Shape transform, first-pass Chord Stab Paint, first-pass `Build 4-Bar Chain` scene generation, first-pass live scene-chain step/control playback, `Auto` / `2 Bar` / `4 Bar` chain length mode, and current/scene-chain MIDI drag-out. Missing next is deeper phrase structure: arranger-style chain editing, deeper per-scene automation/rate/follow behavior, deeper glide/portamento behavior, deeper condition vocabulary, deeper contour generation, deeper chord-stab paint variants, MIDI import, and audio/loop drag-out.
- The visual macro/motion direction is right. SEQ now has an expanded pattern focus editor alongside the macro, sample, and source focus overlays; FX and deeper MOD focus editors remain useful next targets where the UI is still dense.

### Ableton, Serato Sample, And Scaler Expectations

Ableton Live 12, Serato Sample, and Scaler-style tools set expectations outside flagship synths. Producers expect transform/generate actions, fast chord and phrase assistance, slicing/chopping with timing intelligence, key/BPM awareness, drag-out results, and previews that make sense before committing to a sound.

Implication for Nate VST:

- "Make it more house," "tighten low end," "more groove," "darker dub," "shorter stab," "delay throw variation," and "reset mix-safe" are not gimmicks if they are deterministic, audible transforms over existing code paths.
- The sampler needs transient/manual markers, per-slice playback modes beyond the first nudge/fade actions, slice-to-sequencer lanes, full relink/dependency management beyond the first missing-file stale-audio guard, and formant-safe/time-stretch direction before it feels serious for UKG and vocal chop work.
- Chord and groove generation should produce editable MIDI/steps, not hidden magic. Direct Ableton drag-out is a top workflow gap.

### Drum, Kick, And Expansion Ecosystem Expectations

XO, Atlas, Maschine, Battery, KICK 3, PunchBox, and DAW-native bounce/export workflows show a separate house-production expectation: fast drum sample discovery, kick/sub compatibility checks, kit patterns, rendered loops, and tactile drag-out. NEXUS, Serum, Roland Cloud, and KORG also show that a large instrument becomes easier to trust when content arrives as versioned packs with metadata, previews, dependencies, and compatibility rules.

Implication for Nate VST:

- A future drum/kick lane should be house-first, not a generic MPC clone: kick, clap/snare, hats, percussion, vocal/FX one-shots, groove accents, and kick/bass test loops.
- Drag-out should become a product surface, covering MIDI phrases, rendered loops, one-shots, construction-kit stems, and useful filenames/metadata.
- Expansion/content packs need a manifest early: presets, samples, MIDI clips, preview audio/waveform metadata, macro conventions, license metadata, version compatibility, and dependency/relink behavior.

## Current Nate VST Position

Implemented in code:

- VST3 synth plugin for MIDI-in instrument use in Ableton.
- Two-oscillator synth voice with sub, noise, unison, envelopes, filter character/slope, drive, FX rack, Guard, pump, width, delay/reverb throws, meters, and visual HOME analysis.
- Oscillator choices include sine, saw, square, triangle, generated wavetable, procedural organ, and procedural house piano.
- House layer rack visualizes existing Osc 1, Osc 2, Sub, Noise, and Sample Mix as Sub, Body, Character/Stab, Transient/Noise, and Chop; it is now live, clickable, and available as an expanded source-layer focus overlay.
- 16-step sequencer includes note, velocity, probability, timing, length, lock, ratchet, first-pass condition states, groove templates, chord/stab colors, first-pass Chord Stab Paint, A/B/Fill/Drop scenes, first-pass `Build 4-Bar Chain` scene generation, first-pass live scene-chain step/control playback, `Auto` / `2 Bar` / `4 Bar` scene-chain length mode, pattern tools, single-pattern MIDI file export, captured scene-chain MIDI export, and first-pass MIDI file drag-out from the `MIDI` and `Chain` buttons.
- Sampler/chop workflow includes WAV/AIFF loading, waveform display, eight slice pads, per-slice region/pitch/gain/pan/probability/reverse/stutter/choke memory, Slice Keys, and expanded chop focus overlay.
- MOD includes LFOs, Mod Env, Velocity, S&H, Smooth, Chaos, LFO 2, macro sources, right-click assignment, modulation rings/stripes, route map, matrix bypass/delete/duplicate, macro assignment pad, and macro focus overlay.
- Library includes recursive user/factory scanning, metadata, ratings, favorites, smart crates, style filters, crate map, selected-preset profile, compare/revert, safe overwrite, 96 repo-managed factory presets, construction-kit style metadata, rendered preview cache/playback, visible-row preview warming, preview level badges, and compact rows that keep sound identity readable at narrow widths.
- Test suite covers layout, theme contrast, factory preset metadata/rendering, rendered preset previews, sequencer scenes/ratchets/conditions/house patterns, source character, random candidates, Guard/Pump/stereo/spectrum telemetry, global edit history, and FX sends.

## Competitor Lessons That Matter For House

### 1. Source Depth And Identity

Serum 2, Pigments, Phase Plant, Falcon, MASSIVE X, modwave, wavestate, NEXUS5, and ZENOLOGY all win on source material depth. That can mean deep wavetable engines, sample/granular/spectral engines, partial/layer systems, huge curated PCM/multisample libraries, or genre expansions.

Nate VST has good first-pass procedural sources, but its house identity still needs more source authority:

- Original/legal house-piano and digital-PCM-style chord sources, not only additive approximations.
- Drawbar organ depth with editable drawbars, key click, leakage/noise, percussion, and bass-friendly mono behavior.
- Rubber, Reese, 303/101-style, speed-garage, and organ-bass source snippets with macro-ready tone controls.
- True wavetable import/playback and richer warp modes for modern tech-house bass, dub stabs, and metallic pings.
- Focused FM/operator source for bells, pings, metallic chord bite, and warehouse stabs.
- Layer architecture that can stack sub/body/character/transient/chop sources with true mutes, solos, copy, and macro blend. The first live/expanded visual rack is implemented; deeper DSP routing remains open.

Highest-impact source issue mapping:

- #73 House source character: organ, M1-style stabs, Reese/rubber bass, and richer warp modes.
- #59 Lightweight A/B layer architecture.
- #43 Wavetable oscillator and character filter pass.
- #41 UKG and bassline bass engine.

### 2. Browsing And Content Scale

NEXUS5 and ZENOLOGY show the value of a preset-first workflow with a large browser, expansions, custom banks, and fast audition. Pigments and Serum also make presets and previews central to discovery.

Nate VST now has metadata, smart crates, preview cache/playback, ratings, favorites, crate maps, and 96 curated factory presets. The gap is now scale, preview workflow polish, and construction-kit browsing:

- Fully background rendered preview generation so the UI does not block on first audition. A visible-row batch warming action is implemented as the first safe manual pass.
- Waveform preview badges in Library rows. Level badges are implemented from cached peak/RMS metadata.
- Construction-kit mode that groups preset, key, BPM, sequence, macro intent, and suggested Ableton clip notes.
- Larger house packs: deep house chords, piano house, jackin organ bass, disco/filter stabs, tech-house rubber hooks, dub chords, dirty tools, vocal chops, and loop starters.
- Missing-sample/wavetable dependency tracking and relink.
- Pack import/export and editable user crates.

Highest-impact library issue mapping:

- #76 Library rendered audio previews for house patches.
- #75 House preset expansion and construction-kit content.
- #83 Expansion-ready house content pack format.
- #68 Preset audio previews, smart crates, and construction-kit workflow.
- #42 Tagged browser and construction-kit preset workflow.

### 3. Groove, Motion, And Ableton Clip Flow

KORG wavestate/modwave separate timing, pitch, shape, gate, and step values into lanes. Serum 2 adds clip sequencing. MASSIVE X has drawn Performers. ShaperBox shows how visual rhythmic curves become production tools.

Nate VST has a strong 16-step foundation, ratchets, first-pass odd/even/fill conditions, a first Slide lane with live/export legato overlap, scenes, first-pass `Build 4-Bar Chain` scene generation, first-pass Chord Stab Paint, first-pass live scene-chain step/control playback, `Auto` / `2 Bar` / `4 Bar` chain length mode, chord colors, grooves, single-pattern MIDI file export, captured scene-chain MIDI export, first-pass MIDI drag-out, and a first Bass Contour Shape transform. The biggest gap is that it is still internal and not Ableton-native enough:

- Deeper arranger-style chain editing, follow/repeat behavior, and scene management beyond the first forced-length modes and generated A/B/Fill/Drop scenes.
- Per-scene control playback beyond the first live step-chain pass.
- Deeper slide/glide controls beyond the first overlap pass, accent lanes, richer condition vocabulary, ties, and step repeat probability.
- Ableton validation and richer clip handoff beyond first-pass current/scene-chain `.mid` file drag-out.
- Drag-out/render handoff for audio loops, one-shots, and construction-kit stems, not only MIDI.
- Optional MIDI output or a companion clip-export path, because the plugin currently reports no MIDI output.
- Deeper chord-stab paint variants and deeper bassline contour generation beyond the first Chord Stab Paint and Bass Contour Shape transforms.
- Sequencer lanes for filter, drive, warp, pump, sends, sample start, and macro moves.

Highest-impact groove issue mapping:

- #74 House groove tools: 2-bar/4-bar chains, ratchets, slide, conditions, and MIDI drag.
- #84 House drum and kick-bass construction lane with drag-out.
- #71 Dedicated house groove engine.
- #72 House chord and bassline designer.
- #50 Sequencer lanes, MIDI capture, and club pattern utilities.

### 4. Sampler, Chops, And Vocal Workflow

Serum 2, Pigments, Falcon, Ableton Simpler, NEXUS5, and KORG sample systems all point to deeper sample handling: slicing, loop tools, sample libraries, granular/time work, and fast audition.

Nate VST has a useful starter sampler. Missing for house and UKG:

- Transient detection, beat-grid slicing, and fully manual markers.
- Per-slice envelope, filter, send, playback mode, and deeper choke-group control beyond the first nudge/fade/choke pass.
- Slice-to-sequencer lane and pattern generation from slices.
- Time-stretch/warp and formant-safe vocal chop handling.
- Full missing-file relink workflow beyond the first stale-audio clear and missing-path UI status guard.
- Sample browser/crates with similarity and loop role tags.
- House drum/one-shot workflow: kit pads, per-pad trimming, accent/probability lanes, rendered loop export, and kick/bass compatibility checks.

Highest-impact sampler issue mapping:

- #77 Sampler and chop depth: slice markers, fades, relink, and formant-safe direction.
- #84 House drum and kick-bass construction lane with drag-out.
- #63 Sample discovery browser and similarity workflow.
- #34 UKG vocal slice grid and chop audition workflow.
- #55 UKG chop zero-cross snap and fade guards.
- #28 Formant-preserving sampler pitch and time-stretch engine.

### 5. Visual Modulation And Performance Editing

Serum, Pigments, MASSIVE X, Phase Plant, Saturn 2, and KORG instruments all make modulation visible and fast. They use drag assignment, colored routes, interactive performance views, macro controls, and visual motion editors.

Nate VST has route maps, rings, badges, macro pads, macro focus overlays, selected-control strip, MSEG-like LFO drawing, and right-click assignment. Remaining gaps:

- Direct drag-to-modulate from source chips.
- Per-route min/max ranges, curves, slew/smoothing, and processors.
- Macro snapshots/morphing.
- XY assignment editor with multiple destinations per axis.
- Expandable focus panels for Sequencer, FX rack, and modulation matrix, not only Macro and Sample.
- Implemented after the code audit: Mod Env and Velocity are now live sources for sample and FX modulation destinations, with `ModulationSourceParityAudit` covering sample mix and FX pump behavior.

Highest-impact modulation issue mapping:

- #79 Faster modulation workflow: drag routes, ranges, curves, macro snapshots, and XY assignment.
- #56 Per-control modulation inspector and macro assignment editor.
- #40 MSEG curve editor and visual modulation feedback.

### 6. Drive, Mix Safety, And Club Translation

Saturn 2, Ableton Roar, ShaperBox, and modern synth FX sections make distortion visual, multiband, oversampled, and gain-aware. House patches need dirt without losing sub control.

Nate VST has broad FX, Guard glue/punch/clip, Club Monitor, output meter, oscilloscope, spectrum, stereo field, and pump telemetry. Remaining gaps:

- Multiband drive and bass-safe saturation.
- More drive models with loudness/gain compensation.
- Oversampling modes for drive and aggressive filter behavior.
- Clearer limiter/clip metering and Guard hit history.
- External Ableton sidechain input, not only internal pump curves.
- Implemented after the code audit: the processor now reports a nonzero tail length for delay/reverb/send behavior, and `EffectsSendAudit` guards that host-facing report plus send-tail clearing.
- Role loudness targets and preset pack consistency beyond finite/non-silent/peak render smoke tests.
- Kick/bass compatibility checks for phase, note range, length, mono sub safety, and short club test loops.

Highest-impact mix issue mapping:

- #78 House drive and mix-safe color: multiband drive, oversampling, and clearer clip metering.
- #65 Club mix safety compressor clipper and transient workflow.
- #84 House drum and kick-bass construction lane with drag-out.
- #81 Ableton release validation.

## Most Important Missing Work

1. Build a stronger original house source system: drawbar organ controls, legal/original digital-PCM-style piano/chord stabs, Reese/rubber/acid bass source snippets, true wavetable import/warp, and a lightweight source-layer architecture.
2. Turn the sampler into a serious chop instrument: transient/beat/manual slicing, per-slice playback modes/envelopes/filter/sends beyond the first nudge/fade pass, slice-to-sequencer lanes, full relink/dependency management beyond the first missing-file safety guard, and formant-safe vocal handling.
3. Make the sequencer Ableton-friendly: arranger-style chain editing, deeper per-scene automation/rate/follow behavior, deeper slide/glide controls beyond first-pass overlap, richer condition vocabulary, deeper chord-paint variants, deeper bass contour tools, MIDI import, and audio/loop drag-out; captured scene-chain MIDI export, first-pass Chord Stab Paint, first-pass `Build 4-Bar Chain`, first-pass live step/control-chain playback, expanded-focus `Auto` / `2 Bar` / `4 Bar` chain length mode, first-pass Slide lane overlap, and first-pass `.mid` drag-out are the first MIDI/phrase strategies.
4. Add a house drum/kick-bass construction lane: kit pads, sample discovery, groove/accent lanes, kick/bass phase-length checks, rendered loop/one-shot drag-out, and construction-kit stems.
5. Finish fast visual modulation: drag-to-modulate, route min/max/curves/slew, macro snapshots, and XY assignment editing. Sample/FX Mod Env and Velocity source parity is now implemented.
6. Add pro drive and club safety: multiband drive, oversampling, gain compensation, bass-safe modes, clearer clip/limiter metering, external sidechain, and kick/bass translation checks. Basic host-facing tail reporting is now implemented.
7. Mature Library previews and content: background preview rendering beyond the first visible-row warmer, waveform badges, construction-kit pack views, dependency tracking, pack import/export, and larger curated house packs.
8. Define expansion-ready content packs: manifests, versioning, license metadata, previews, samples, MIDI clips, dependencies, relink, and safe user/factory separation.
9. Continue the UI overhaul with expandable focus editors for Sequencer, FX, and Mod Matrix plus screenshot/layout regression checks.
10. Add release-quality Ableton validation: scan, load, automate, save set, reopen, duplicate track, freeze/flatten, missing sample behavior, and preset loudness consistency.

## Recommended Next Build Order

1. Upgrade Library rendered previews beyond the implemented visible-row warmer and level badges into background generation, waveform badges, safe regeneration controls, and preview loudness consistency.
2. Start the source-character pass under #73: drawbar controls, rubber/Reese/acid snippets, original house-piano/chord material, and wavetable/warp foundations.
3. Add sampler transient/manual marker infrastructure under #77 before time-stretch/formant work. `SamplerChopAudit` now protects current slice reset, Slice Keys, probability, nudge/fade playback changes, missing-file stale-audio clearing, and UKG chop setup behavior.
4. Add sequencer-to-Ableton workflow under #74: validate first-pass direct MIDI drag-out and Slide overlap in Ableton, then add arranger-style chain editing, deeper per-scene automation/rate/follow behavior, deeper glide/portamento behavior, deeper condition options, MIDI import, and audio/loop handoff.
5. Start the house drum/kick-bass construction lane under #84 after the drag-out/export foundation exists, so loop/stem/one-shot handoff is designed once.
6. Define the expansion-ready pack manifest under #83 before the next large content push under #75.
7. Add modulation route ranges/curves/slew and drag-to-modulate under #79.
8. Add multiband/bass-safe drive and oversampling under #78.
9. Add deterministic house performance transforms under #80, built on the existing randomization, sequencer, FX, Guard, and macro systems.
10. Expand construction-kit packs under #75, using the new source/sampler/sequencer/drum features as soon as they exist.
11. Build expandable Sequencer, FX, and Mod focus panels as part of the UI overhaul under #61/#82.
12. Run Ableton release validation under #81 after the next source/sampler/sequencer slices, not only after UI polish.
