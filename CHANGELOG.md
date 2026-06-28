# Nate VST Changelog

## 2026-06-28

### LFO 2 Modulation Source

- Added an appended `LFO 2` source after `Chaos`, preserving all existing modulation source indices for saved projects and Ableton automation.
- Added independent LFO 2 rate, sync, synced rate, shape, depth, phase, and retrigger parameters with a compact MOD-panel editor.
- Routed LFO 2 through synth, sample, and FX modulation paths so it can drive filter, Osc Warp, sample start/pitch/ramp/stutter, pump depth, delay/reverb mix, width, and FX drive destinations.
- Expanded the MOD source summary from 14 to 15 rows so LFO 2 routes show active count and summed depth like the existing sources.

### Slice Keys Playback Mode

- Added a third sample playback mode, `Slice Keys`, while preserving the existing `Gate` and `One Shot` indices for saved projects.
- In Slice Keys mode, MIDI notes C3-G3 map across the eight stored slice pads and repeat every octave for drum-rack-style UKG chop playing.
- Slice-key voices now use each pad's stored pitch, gain, reverse, stutter enable, and stutter repeat settings directly at trigger time.
- SAMPLE pad selection no longer forces the playback mode out of Slice Keys, and the slice status row calls out the key-map mode when active.

### Per-Slice Sampler Edit Memory

- Added persisted per-slice edit memory for the eight SAMPLE slice pads, covering custom/default state, pitch, gain, reverse, stutter enable, and stutter repeat count.
- Added a compact slice edit row with Store, Recall, Dice, and Rev actions for UKG chop work without leaving the SAMPLE tab.
- Slice pads now mark custom slices with `*` and expose stored pitch/gain/reverse/stutter details in their tooltips.
- Selecting a custom slice recalls its stored edit values; untouched slices still follow the selected Clean, Pitch, Reverse, Stutter, or Garage style defaults.

### MSEG Curve Editor Pass

- Upgraded the LFO custom curve display into a denser MSEG-style editor with segment lanes, stronger value guides, focused point readouts, and interpolated drag drawing across multiple points.
- Added double-click point reset and Shift/Cmd quantized editing while keeping the existing eight saved `lfo1_curve_*` parameters intact for session compatibility.
- Added new club-focused curve presets for UKG swing, minimal pulse, techno ramp, and house chug movement.
- Curve edits now update the selected-control inspector so point values and automation IDs are visible while drawing modulation.

### Selected Control Inspector

- Added a compact global `CONTROL` strip that follows the last changed knob or slider across Home, Synth, Lab, Mod, Sample, Seq, FX, and Library views.
- The strip now shows formatted value text, the automation parameter ID, and whether the parameter has active modulation routes.
- Reused the existing modulation destination metadata so modulated targets show source names, per-route depth, and summed route amount while unassigned targets stay visually quiet.
- Wired the inspector through the shared slider setup so new APVTS-backed controls inherit the same feedback behavior automatically.
- Added global `MOD+` and `MOD` actions to the selected-control strip so touched modulatable controls can quickly receive the current MOD source or open the detailed MOD editor.

### Deep Research Expansion

- Expanded `docs/PROGRAM_RESEARCH_GAP_LIST.md` with a second research pass covering Vital, Korg opsix native, XO, Sononym, Ableton Live 12 MIDI/browser workflows, and updated lessons from Serum 2, Pigments, Phase Plant, Korg modwave, ZENOLOGY Pro, ShaperBox, Saturn 2, Infiltrator, and UK garage production references.
- Added 160 additional feature ideas, extending the backlog from 232 to 392 items across hybrid source racks, wavetable/spectral depth, FM/organ/partial sources, sampler discovery, sequencer/MIDI tools, modulation visualization, rhythmic FX, preset discovery, and house/UKG content packs.
- Added a refreshed next-build order that puts editor componentization, layout tests, modulation rings, pattern lanes, UKG slicing, wavetable playback, multiband drive, MSEG movement, full-browser discovery, and Ableton workflow validation in priority order.

### Chaos Modulation Source

- Added an appended `Chaos` source after `Smooth` so existing modulation source indices stay stable while adding random-walk motion.
- Routed Chaos through synth, sample, and FX modulation paths with a bounded, mean-reverting walk that follows the existing LFO timing rate.
- Expanded the MOD source summary area from 13 to 14 rows; the existing adaptive two-column source layout now balances seven rows per column.
- Seeded `Techno Warehouse Stab` with a subtle Chaos-to-FX-Drive route for unstable warehouse edge without changing core pitch or cutoff.

### Smooth Random Modulation Source

- Added an appended `Smooth` source after `S&H` so existing modulation source indices and saved presets stay compatible.
- Routed Smooth random drift through synth, sample, and FX modulation paths, using the existing LFO timing as the random target clock while slewing between targets.
- Expanded the MOD source summary area from 12 to 13 rows and made the two-column source layout adapt so the new row fits without overlapping the macro controls.
- Seeded selected factory presets with subtle Smooth routes for UKG Dred warp drift, tech-house oscillator detune, and vocal-chop sample-start variation.

### Program Research Gap List

- Added `docs/PROGRAM_RESEARCH_GAP_LIST.md`, a deeper program-by-program research matrix covering Serum 2, Pigments, Phase Plant, Current, ZENOLOGY Pro, u-he, Korg, Ableton, ShaperBox, Saturn, Portal, Infiltrator, and UK garage production patterns.
- Expanded the next-add backlog into 232 concrete feature ideas across UI, source engines, drive/filter tone, modulation, sampler chops, sequencer/groove, FX routing, randomization, library, factory content, and release quality.
- Linked the new research artifact from the README, plan, next-feature research, and deep backlog so future feature work has one current comparison point.

### Sequencer Step Length Lane

- Added a saved per-step `length` value to the internal sequencer, with old presets and sessions defaulting to full length so existing patterns keep their previous playback.
- Added a fourth SEQ grid lane, `Len`, for drawing or mouse-wheel nudging step gate length alongside Velocity, Probability, and Late timing.
- Made playback and MIDI export multiply the existing global Gate by each step's length, so short UKG stabs, vocal triggers, and minimal plucks export the same way they sound.
- Included step length in sequencer undo snapshots, preset/session state, random patterns, mutations, groove transforms, and factory preset generation.

### Sample And Hold Modulation Source

- Added an appended `S&H` source to the MOD matrix for stepped random movement without shifting existing modulation source indices.
- Routed `S&H` through synth, FX, and sample modulation paths, using the existing LFO timing as the hold clock while staying independent from the selected LFO shape and depth.
- Expanded MOD source summaries to include the new source and kept the macro assignment editor limited to Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw.
- Seeded global FX/sample held values on reset/prepare so S&H routes produce movement immediately.

### Control Feel Pass

- Reduced rotary drag distance for the main knob groups so common house/UKG shaping controls respond with less dragging.
- Added explicit Shift/Cmd fine-adjust behavior to rotary, horizontal, compact curve, and macro-assignment amount sliders using JUCE velocity mode.
- Kept scroll-wheel changes disabled to avoid accidental Ableton trackpad edits, while preserving drag popups, typed values, and double-click reset.
- Added control tooltips that call out drag, fine-adjust, reset, and value typing behavior.

### Library Browser Table

- Added a compact multi-column LIBRARY preset browser list under the existing save/search/load controls.
- Browser rows now show preset name, category/folder, pack, key, BPM, rating, favorite marker, and saved macro summary before loading.
- Kept the existing preset combo box workflow intact while syncing combo and browser-row selection both ways.
- Added double-click row loading for faster audition/load flow when browsing house, UKG, tech-house, minimal, and techno presets.

### Competitor Gap Analysis Refresh

- Added `docs/COMPETITOR_GAP_ANALYSIS.md`, a larger feature-gap review across Serum, Pigments, Phase Plant, Vital, u-he, Korg, Roland, Ableton, ShaperBox, Saturn, Portal-style granular FX, and UKG production references.
- Expanded the future feature list across UI quality, source engines, filters, sampling, sequencing, modulation, FX routing, randomization, library, test coverage, and release/distribution.
- Refreshed the shorter deep-research backlog so already-implemented basic FX work does not stay listed as a next action.
- Linked the new gap-analysis document from the README.

## 2026-06-27

### Preset Browser Macro Preview

- Added saved-preset macro preview metadata for Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw by parsing existing preset XML.
- Added a `Macro Rich` Library filter and `Macros` sort mode for finding presets with more performance-macro movement.
- Extended preset browser search, status text, and tooltips with macro summaries so house/UKG presets can be judged before loading.
- Kept preset files and APVTS parameter IDs unchanged; old presets fall back to a flat macro preview.

### Sequencer Velocity Probability And Timing Lanes

- Added three editable lane rows under the SEQ piano-roll grid for per-step velocity, probability, and late timing.
- Lane bars now show each step's playback strength, ghost-note chance, and pushed-groove amount without relying only on small note-cell markers.
- Clicking or dragging a lane row edits the matching per-step value, and mouse-wheel nudging works directly over lane rows.
- Reused the existing saved sequencer step state, so presets, undo snapshots, MIDI export, and host/project recall keep the same persistence path.

### Macro Assignment Editor

- Added a compact MOD-panel macro assignment lane for Tone, Dirt, Motion, Space, Weight, Bounce, Warp, and Throw.
- The new editor can add/update a macro route, replace all routes for the selected macro with one destination, or clear that macro's routes while reusing the existing eight saved MOD matrix slots.
- Added a live macro summary so the selected macro shows its current destinations and depths without scanning every matrix row.
- Kept the HOME performance macro layout unchanged and kept the deeper editing workflow inside the focused MOD panel.

### Deep Research Backlog

- Added `docs/DEEP_RESEARCH_BACKLOG.md`, a long feature inventory based on current synth, FX, groove, and UKG production references.
- Grouped missing work into UI/workflow, modulation, source engines, sampler/chops, sequencer/groove, FX, randomization, library, quality, and Ableton/distribution tracks.
- Linked the main plan to the new backlog so future implementation slices can be selected from one durable roadmap.

### LFO Curve Presets And Modulation Ring Badges

- Added a MOD-panel `Curve Preset` selector for saved LFO 1 curve-point parameters, covering garage push, ducking, offbeat, riser/fall, stepped, wobble, and flat movement shapes.
- Added a moving phase cursor to the LFO curve editor when Curve mode is active, using host PPQ when available and a UI fallback phase otherwise.
- Upgraded modulation-ring drawing so destination knobs show the modulated range around the current value instead of only from the minimum.
- Added compact modulation badges and source tooltips on destination knobs, showing active route count and summed modulation depth.

### Library Pack Key And Tempo Metadata

- Added preset metadata fields for author, pack, musical key, and target BPM, with user saves writing those values into `.natevstpreset` files.
- Added LIBRARY-panel metadata controls plus search, filter, and sort support for pack, BPM range, key, and author.
- Updated factory preset generation so the bundled UKG, house, tech-house, minimal, and techno presets carry pack/key/BPM metadata.
- Kept old presets compatible by falling back to `User Pack`, `Factory Pack`, `Any Key`, and no BPM when metadata is missing.

### Library Ratings And Category Folders

- Added persistent 0-5 star preset ratings in `Library.xml`, so user and factory presets can be ranked without modifying factory preset files.
- Added LIBRARY-panel rating and sort controls with filters for rated, five-star, and four-star-plus presets, plus sorting by name, rating, newest file, category, or source.
- Made the save category box editable and made new user preset saves write into category subfolders, including slash paths such as `UKG/Bass` or `House/Chords`.
- Changed user/factory preset scanning and loading to recurse through subfolders while preserving older flat preset files.

### Host Sync Status And Home Macro Layout

- Added an audio-thread host-sync snapshot for the editor, exposing host BPM, play state, and PPQ availability without querying the host playhead from the UI.
- Added compact `LOCK/HOST/INT` status badges to the SEQ and FX panels so house, UKG, techno, and minimal patterns can be checked against Ableton transport and tempo-sync state while editing.
- Split the HOME macro bank into two three-knob rows and gave the HOME top band more space so Tone, Dirt, Weight, Bounce, Warp, and Throw stay readable next to the Motion/Space XY pad.
- Re-reviewed the current feature set against Serum-style visual modulation, ZENOLOGY-style layers/browsing, u-he-style quality/character, Korg-style motion lanes, ShaperBox-style curve editing, and UKG chop workflows; the remaining gaps stay focused on source depth, visual modulation, slice lanes, construction-kit presets, and drive/filter quality modes.

### Mod Route Bypass And Delete

- Added saved per-slot MOD route enable flags so individual matrix routes can be bypassed without losing their source, destination, or amount.
- Added compact `On` and `X` controls to each MOD routing row for fast route bypass/delete while keeping the two-bank matrix layout intact.
- Made synth, sample, and FX modulation engines ignore bypassed routes, and made destination rings, source summaries, and inspector summed depth only count enabled routes.
- Updated preset migration, randomization, UKG Chop route seeding, and factory presets so existing routes remain enabled by default.

### Oscillator Warp Pass

- Added an automatable Osc Warp control that shapes oscillator harmonics before the filter, giving basses, stabs, plucks, and UKG Dred-style patches more source movement without needing another effect.
- Routed the existing Warp macro and the MOD matrix into Osc Warp, including destination-ring feedback and drag-to-focus support from the SYNTH page.
- Seeded recipe randomization and factory presets with conservative Osc Warp values, keeping basses and techno/warehouse stabs more animated while leaving bells, vocal chop starters, and cleaner chords restrained.
- Moved Osc Warp into the wider SYNTH voice group after the UI review so the source-mix row stays readable and the new knob has enough room.

### Filter Slope Pass

- Added an automatable Filter Slope selector with 12 dB and 24 dB options so house/UKG patches can choose open movement or tighter club filtering.
- Implemented 24 dB filtering as a second TPT filter stage with restrained resonance, preserving the old single-stage 12 dB behavior as the default for existing presets.
- Added the slope selector to the SYNTH page, made old presets migrate to 12 dB, and included slope in filter-scoped randomization locks.
- Seeded recipe randomization and factory presets with 24 dB for bass, acid, dred, and warehouse tones while keeping cleaner stabs, bells, and plucks mostly at 12 dB.

### Filter Character Pass

- Added an automatable Filter Character selector with Clean, Warm, Acid, and Dirty modes for faster house bass, tech-house rubber, techno stab, and UKG organ/chord coloration.
- Wired character modes into the synth filter DSP with per-mode drive behavior while keeping Clean as the default for older presets and sessions.
- Added the character selector to the SYNTH page, made old presets migrate to Clean, and included the new parameter in filter-scoped randomization locks.
- Seeded randomization recipes and all factory presets with practical character defaults such as Warm for UKG/house weight, Acid for rubber/pulse basses, and Dirty for warehouse/dred tones.

### Oscillator Quality Pass

- Reworked saw and square oscillators with polyBLEP discontinuity correction so bright house stabs, tech-house basses, techno pulses, and UKG chord tones alias less at higher notes.
- Rebuilt triangle from an integrated bandlimited square core while preserving the existing waveform enum, parameter IDs, factory presets, and Ableton automation mappings.
- Clamped oscillator phase increments below Nyquist to avoid unstable upper-register playback on 44.1 kHz sessions.
- Tightened the FX detail header and widened MOD destination selectors after the UI audit flagged crowded Pump controls and long sample/FX destination names.

### Sample Modulation Destinations

- Added MOD matrix destinations for Sample Start, Sample Mix, Sample Pitch, Sample Ramp, and Sample Stutter so UKG vocal chops can move from the same routing workflow as synth and FX motion.
- Wired sample modulation into `SamplePlayer` using global LFO 1 and macro sources, with host-PPQ sync when Ableton transport position is available.
- Added modulation-ring feedback to the matching SAMPLE controls and made `UKG Chop` seed empty matrix slots with useful sample-start and pitch/ramp movement.

### FX Modulation Destinations

- Added MOD matrix destinations for FX Pump Depth, FX Delay Mix, FX Reverb Mix, FX Width, and FX Drive so house/UKG movement can reach the effects rack directly.
- Wired FX matrix modulation through the audio DSP using LFO 1 and the eight macros as global sources; per-voice Mod Env and Velocity remain synth-only until a proper global modulation bus exists.
- Added modulation-ring feedback to the matching FX sliders and made the MOD inspector avoid dead Mod Env/Velocity routes when adding global FX targets.
- Seeded recipe randomization with practical FX routes such as Bounce to Pump Depth, LFO 1 to FX Drive, and Throw to Delay/Reverb Mix.

### Rhythmic Pump Shaper

- Added an automatable Pump Curve selector with Smooth, Tight, Garage, Stutter, and Gate shapes for more precise house, tech-house, minimal, techno, and UKG ducking.
- Updated Pump DSP so each curve changes the gain envelope while preserving the old Smooth behavior as the default.
- Added a focused Pump curve preview to the FX detail view so the selected duck shape, rate, depth, and phase are visible while designing rhythmic movement.
- Wired Pump Curve into the FX detail UI, rack summaries, momentary Pump Drop restore path, module presets, randomization recipes, and factory-preset generation.
- Added a saved Custom Pump curve with eight automatable points, draggable FX-panel handles, and matching DSP interpolation for VolumeShaper-style house/UKG duck editing.
- Made Pump preview reflect Bounce macro-driven pumping, even when the Pump module itself is bypassed.
- Let recipe randomization occasionally generate editable Custom Pump curves for stronger house, tech-house, minimal, techno, and UKG rhythmic variation.

### FX Rack Review Fixes

- Restored the compact two-column FX rack layout when the visible module count is high, so full effect chains do not overflow the rack lane.
- Changed the Guard rack badge to show `SAFE` only when Guard is enabled; bypassed Guard now reads as `OFF`.
- Made FX Move Up/Down enablement check for an actual visible move target, avoiding no-op controls when hidden disabled modules sit in the order list.

### Tempo-Synced Delay And Host Phase

- Added Delay Sync and Delay Rate controls so FX throws, Space macros, and UKG vocal-chop presets can lock delay timing to musical divisions instead of only milliseconds.
- Made the internal SEQ engine read host play state and PPQ position so house, techno, and UKG patterns recover cleanly after Ableton transport starts, loop jumps, and repositioning.
- Stopped the internal SEQ engine from free-running while a host transport is stopped, flushed active sequencer notes on disable/stop, and aligned host jumps against per-step groove delay.
- Expanded the SEQ piano-roll grid to the full two-octave sequencer note range and routed direct grid edits through the sequencer undo stack.
- Kept stopped-host FX auditioning smoother by only applying PPQ phase resets to Pump/Tremolo while the host is playing.
- Made the FX rack stay as a readable single compact lane and made Move Up/Down skip hidden disabled modules.
- Updated FX rack summaries, momentary throws, module presets, UKG randomization, and factory-preset generation to prefer synced delay divisions where rhythmic delay is expected.

### Synth Page Grouping Pass

- Reworked the SYNTH panel into task-focused Source Mix, Pitch + Voice, Filter Drive, and Amp + Output groups so house basses, stabs, plucks, and UKG patches are faster to scan.
- Removed the old 8-knob and 9-knob rows from the SYNTH page, reducing visual clutter without changing any parameter IDs, presets, or Ableton automation mappings.
- Shortened rotary drag travel again and made the shared knob-row helper use tighter padding in narrow rows so controls feel less resistant and crowding is less likely as pages evolve.
- Reclaimed MOD-panel space for routing by compressing the source/macro and generator groups, then split the eight matrix slots into two readable four-slot banks with duplicated headers.
- Tightened the LAB action row so Generate, Vary, Mutate, Wild, Undo, and Redo no longer overflow the available width.

### Sampler And Preset Correctness

- Fixed reverse sample interpolation so pitched/reversed UKG chop styles interpolate from the continuous source position instead of smearing toward the previous sample.
- Added `sequencer_chord_memory` and all 15 `fx_order` slots to the factory-preset generator defaults.
- Regenerated factory presets so chord-memory and FX rack ordering state are explicit in every factory patch.

### House/UKG Review Delta

- Ran another reference pass across Serum 2, Pigments, Hive 2, ZENOLOGY Pro, Korg-style motion instruments, and UK garage production references.
- Confirmed Nate VST now covers the first layer of house/UKG workflow: fast macros, low-end guidance, randomization scopes, pattern tools, slice auditioning, FX throws, module presets, and modulation inspection.
- Added the next missing product gaps to the roadmap: deeper oscillator/source engines, more visible modulation feedback, per-slice sampler lanes, FX modulation destinations, and a stronger preset/construction-kit browser.

### Modulation Inspector

- Added a MOD routing inspector that focuses one modulation destination at a time and summarizes every active source, slot, depth, and summed depth for that destination.
- Added destination/source selectors, an `Add` action that fills the first available matrix slot with a sensible default depth, and a `Clear` action for deleting all routes targeting the inspected destination.
- Made Cutoff, Resonance, Filter Env, Drive, Osc 2 Tune, and Osc 2 Level focus the inspector when dragged, while preserving the existing matrix and modulation-ring behavior.

### FX Module Presets

- Added a selected-module `Module Preset` menu and `Load` action to the FX detail header.
- Added focused house/UKG/tech-house presets for all 15 FX modules, including garage delay throws, short rooms, mono-safe width, controlled drive, light movement, resonator, and Guard safety settings.
- Kept the pass host-safe by writing existing automatable FX parameters instead of adding new parameter IDs.

### Sample Slice Styles

- Added a saved SAMPLE `Slice Style` selector with Clean, Pitch, Reverse, Stutter, and Garage modes for the numbered slice pads.
- Made slice-pad selection apply style-aware pitch, reverse, pitch-ramp, gain, and stutter settings using the existing automatable sample controls.
- Set `UKG Vocal Chop Starter` to Garage slice style so the factory preset opens ready for pitched/reversed/stuttered phrase auditioning.
- Kept the first pass preset-compatible by adding only one style choice parameter while leaving deeper stored per-slice lanes for a later sampler pass.

### UI Grouping And Control Feel

- Made rotary and horizontal controls easier to move with shorter drag travel, while disabling mouse-wheel edits to avoid accidental parameter jumps inside Ableton.
- Realigned MOD panel background grouping to the actual source, macro, LFO, envelope, and routing layout.
- Added SAMPLE source/chop/shape grouping and compacted its rows so waveform, slice pads, cut controls, and shape knobs no longer compete for vertical space.
- Condensed the FX performance controls into one row under the add/order strip, leaving more room for the focused rack and selected module editor.

### Selective Groove Transforms

- Added a SEQ groove transform dropdown with Tighten, Straight Anchors, Swung Ghosts, Late Stabs, Vocal Push, and Humanize options.
- Added a compact `Shape` action that applies the selected transform to the current pattern without replacing notes, chord settings, scale mode, or template identity.
- Made transforms use the existing sequencer undo path and randomization lock, while reshaping per-step timing/probability/velocity plus focused gate/swing/groove settings.

### Sample Chop Click Guards

- Added non-destructive zero-cross/low-energy snapping for internal sample chop playback boundaries so UKG vocal cuts and slice pads are less click-prone.
- Added adaptive fade-in and fade-out guards to sample voice playback, including stutter restarts, while keeping short chops punchy.
- Kept `sample_start` and `sample_end` automation/preset values unchanged; the sampler only nudges effective playback boundaries internally.

### Chord Memory And Sequencer UI Pass

- Added a saved SEQ `Memory` toggle that expands live played MIDI notes through the selected SEQ chord mode and voicing for one-finger house and UKG stabs.
- Matched note-offs against the chord notes that were actually triggered, so changing or disabling Memory while notes are held does not leave hanging chord voices.
- Rebalanced the SEQ panel around the pattern dropdown, Apply, random/variation/undo/clear actions, and a secondary copy/rotate/MIDI strip so the grid gets more visual priority.
- Added subtle SEQ panel grouping, lighter step timing/probability markers, and removed redundant quick-pattern buttons now covered by the template dropdown.

### Home Performance And Control Feel

- Made rotary controls easier to move by shortening drag travel again while keeping frequency and time parameters slower for precision.
- Shifted HOME macro knobs toward Tone, Dirt, Weight, Bounce, Warp, and Throw while leaving Motion/Space on the XY pad.
- Increased the HOME club low-end assistant area slightly and fixed XY pad handle clipping at the edges.

### Feature Research Backlog

- Prioritized low-risk next additions from reference research: per-control modulation inspection, richer preset metadata/audition, tighter UKG chop editing, editable macro assignments, and FX module presets.
- Kept larger researched items on the roadmap: wavetable oscillator lane, layered/partial architecture, motion sequencing lanes, deeper sampler/chopper behavior, character filter/drive bank, FM/operator options, and XY/vector morphing.

### Low-End Assistant Guidance

- Expanded the HOME `CLUB` assistant with a phase/status chip and soft guidance line for bass patch decisions.
- Surfaced when oscillator phases reset cleanly, when Mono is actively collapsing unison spread, when low-side energy needs mono-bass protection, and when the sequencer root is outside the practical club sub range.
- Kept the pass UI-only by reading existing root, Mono, Width, unison, Guard, and low-end meter values without adding parameters or audio-thread work.

### Randomization Redo History

- Added a LAB/HOME `Redo` action for restoring the last undone Generate, Mutate, or Variation result.
- Added randomization history summaries such as `Undo: Mutate` and `Redo: Variation` to the random status label.
- Kept the history transient and parameter-compatible by storing in-session plugin-state snapshots only, without adding host parameters or preset data.

### Wild Mutation Strength

- Added a LAB/HOME `Wild` mutation action for stronger recipe-aware sound changes while preserving active randomization locks.
- Kept the mutation ladder explicit: `Vary` for subtle changes, `Mutate` for medium movement, and `Wild` for bigger house/UKG idea jumps.
- Integrated Wild with the existing Undo/Redo randomization history so status labels and restore behavior remain consistent.

### Section-Scoped Randomization

- Added a LAB/HOME `Scope` selector for limiting Generate, Vary, Mutate, and Wild to All, Source, Env, Filter, Sample, FX, Seq, or Macros.
- Made scoped randomization restore every unselected section from the pre-randomization snapshot while still applying explicit randomization locks afterward.
- Restored modulation and output state during scoped runs so focused edits do not quietly change hidden LFO, matrix, or output-gain behavior.

### Sample Waveform Display

- Added a SAMPLE-panel waveform overview for loaded audio so vocal chops and phrase cuts can be edited visually instead of only by start/end numbers.
- Drew the active chop window, start/end handles, duration/selection readout, and eight phrase markers directly over the waveform.
- Made the waveform draggable: pull the start/end handles for fine trims or drag across the display to set a new chop region that writes the existing automatable sample start/end parameters.

### Sample Panel Grouping

- Split the SAMPLE panel into Source, Chop, and Shape groups so load/actions, waveform/slice editing, and pitch/gain/stutter controls scan separately.
- Rebalanced the SAMPLE panel spacing after adding the waveform so house and UKG chop work keeps a clear top-to-bottom flow.

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
