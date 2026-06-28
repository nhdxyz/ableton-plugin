# Nate VST Randomization And Library Research

Research date: 2026-06-28

This pass focuses on the user request to make randomization one of Nate VST's strongest features and to keep preset saving/category folders reliable. The product goal is a controlled generator for house, UK garage, tech-house, techno, and minimal sounds, not a dice button that produces unusable patches.

## References Reviewed

- Xfer Serum 2: https://xferrecords.com/products/serum-2
- Arturia Pigments: https://www.arturia.com/products/software-instruments/pigments/overview
- Kilohearts Phase Plant: https://kilohearts.com/products/phase_plant
- Minimal Audio Current: https://www.minimal.audio/products/current
- Native Instruments UK garage guide: https://blog.native-instruments.com/uk-garage-music/
- Attack Magazine UK garage beat reference: https://www.attackmagazine.com/technique/beat-dissected/uk-garage/
- NITELIFE Audio UK garage vocal cuts: https://nitelifeaudio.com/classic-techniques-uk-garage-vocal-cuts/

Use these as product-pattern references only. Do not copy proprietary layouts, presets, samples, DSP, or branding.

## What The References Suggest

1. Randomization should be scoped.
   Phase Plant and Pigments emphasize modular sources, lanes, and drag/drop modulation rather than one global destructive random button. Nate VST should keep randomization section-aware: Source, Env, Filter, Sample, FX, Seq, Macros, and future Mod routes.

2. Randomization needs feedback.
   Pigments and Current lean on Play View, color-coded modulation, visible engine state, and preset-first feedback. Nate VST should always show what changed, what is locked, what can be undone, and where the generated idea will save.

3. Random generation should connect directly to the browser.
   Current's content workflow points toward preview, favorite, tag, and pack discovery. Nate VST should let a generated idea become a properly categorized preset without jumping through Library first.

4. House and UKG need guardrails.
   UKG references repeatedly emphasize 124-132+ BPM, 2-step swing, chopped vocals, Dred/Reese bass, sub weight, organ/chord stabs, pitched snippets, and delay/reverb motion. Random recipes should bias toward those musical results instead of generic EDM patches.

5. Preset saving must be boringly reliable.
   Once random generation is good, users will save a lot of small variations. Category folders, normalized metadata, duplicate handling, and crash-safe writes matter more than they did for a small preset browser.

## Implemented In This Slice

- LAB now has one-click section rolls for Source, Env, Filter, Sample, FX, Seq, and Macros.
- The main Generate, Mutate, Vary, Wild, and section-roll actions refresh the visible selector, waveform, wavetable, and sequencer feedback after changing the patch.
- Random actions create a save draft when the preset name is blank or still matches the loaded preset, reducing accidental overwrite risk.
- Save drafts suggest category, pack, and tempo from the selected recipe:
  - UKG bass and Dred recipes -> `UKG/Bass`, `UKG Basslines`, 132-134 BPM.
  - UKG organ/chord/bell recipes -> `UKG/Organ`, `UKG/Stabs`, or `UKG/Bells`.
  - House/tech-house/minimal/techno recipes -> matching genre folders and packs.
- LAB now exposes category, preset name, pack, BPM, and Save directly under the randomizer.
- Preset category paths are normalized before storing metadata, so `UKG\Bass` saves and reports as `UKG/Bass`.
- Presets are written through a temporary file before replacing the final file.
- `PresetSaveAudit` verifies nested category save, metadata normalization, and load-by-name.
- Four Random Lab candidate slots now capture full patch/sample/sequencer snapshots after Generate, Mutate, Vary, Wild, and section rolls.
- Candidate buttons show compact action/recipe/scope labels, highlight the active slot, recall saved generated states, and are covered by `RandomCandidateAudit`.
- Generated Random Lab saves now store `Generated` source metadata, `Generated`/`Random Lab` tags, and the source recipe, and Library can filter/tag-search generated presets.
- Candidate tooltips now summarize whether each slot is brighter/darker, dirtier/cleaner, wider/narrower, more/less motion, more/less space, and hotter/safer than the current patch.
- Active candidates can be promoted directly to performance snapshot A or B from LAB, and `RandomCandidateAudit` verifies promotion and recall.
- Per-slot candidate `Cue` buttons now preview generated ideas with a short note and restore the current patch afterward; `RandomCandidateAudit` verifies that preview state is non-destructive.
- LAB `Save Slot` now writes the active generated candidate into the recursive preset library with generated-source metadata, category folder, pack, key, BPM, and source-recipe tags without recalling the candidate first.
- Candidate buttons now show changed-section count badges, and recall/cue tooltips include changed sections plus old/new value diffs for the most important sound-shaping controls.
- LAB `Save Slot` now has candidate-specific favorite and 1-5 star rating controls, and the saved generated preset is immediately available to Library favorite/rating filters.
- LAB now has focused `Generate`, `Mutate`, `Recipe`, `History`, and `Save` pages so randomization controls are grouped by task instead of shown as one dense control surface.
- LAB Recipe now shows selected-recipe genre, tempo range, good-for intent, and generator-bias guidance for all current house, UKG, tech-house, minimal, techno, and FX recipes.
- LAB Generate and Mutate now expose per-section Source, Env, Filter, Sample, FX, Seq, and Macro strength controls that scale generated changes before locks and candidate capture.
- LAB History now has a visible active-candidate detail panel for slot label, changed sections, trait comparison, and old/new value diff summary.
- Generated preset saves now write searchable `preset_notes` with recipe, source/category/pack, scope, locks, candidate sections, traits, diffs, use hints, and save time.
- LAB Save now includes an editable notes field that auto-fills from generated context but preserves user notes before Save or Save Slot.
- LAB Save now includes reusable note templates for macro intent, Ableton use, UKG variation notes, mix safety, and pack notes.
- Random generation now runs a useful-only validation pass that can restore silent sources, clamp unsafe output gain, correct bass/stab sequencer range, contain bass width, widen tiny sample windows, and surface the validation summary in LAB status, History, and generated preset notes.
- Useful-only validation now renders a short internal audition through the synth/sample/FX chain to detect quiet, clipped, non-finite, or heavy-tail candidates before candidate capture, then appends render feedback to LAB status, History, and generated preset notes.
- Random Lab now retries rejected render candidates before filling a candidate slot, restoring the original pre-random state between attempts and reporting retry exhaustion when locks make a bad result impossible to fix.
- Random Lab now applies recipe-aware safe fallbacks after retry exhaustion, recovering from the original pre-random state with bass/mid/noise/general init shapes, relaxed Output/FX safety locks when needed, and explicit blocked-source-lock reporting.
- Library scan results now keep all eight macro values per preset, and the Browser renders them as a compact Tone/Dirt/Motion/Space/Weight/Bounce/Warp/Throw strip for faster performance-preset triage.
- LIBRARY now separates Find, Browser, Save Patch, and Inspect areas, shows selected-preset notes and macro shape in a dedicated summary component, and keeps HOME preset access to recall/audition instead of save metadata.
- The repo-managed factory pack now covers 32 UKG, house, tech-house, minimal, and techno starter presets, with newer presets carrying curated `preset_notes` for the Library inspector.
- The LIBRARY panel now has a preset-vault polish pass with quick genre/source filter buttons, a denser central crate, and a clearer right-side save metadata flow while preserving existing preset files and metadata.
- HOME now includes stereo field/correlation visualization beside the spectrum display, exposing full-band width, balance, correlation, and low-end side risk for mono-safe house and club mix decisions.

## Highest-Value Randomization Backlog

1. Add route-level locks for modulation matrix slots.
2. Add target locks for specific controls by right-clicking a knob and choosing "Lock from Random".
3. Expand the History detail panel into a scrollable all-parameter diff table with section grouping, changed parameters, old value, new value, and notes.
4. Expand randomization history beyond the first four candidate slots into a scrollable list with search and notes.
5. Add "promote variation" and "discard variation" actions.
6. Add per-candidate row save/favorite/rating shortcuts so each generated slot can be organized without using the shared save strip.
7. Add favorite-aware randomization seeded from 4-5 star presets.
8. Add "more like selected preset" using parameter-distance mutation.
9. Add genre transforms: More UKG, More Deep House, More Minimal, More Warehouse, More Tech House.
10. Add safety meters during randomization: output peak, sub stereo risk, drive risk, FX tail risk.
11. Add a loudness-normalized audition note after randomization.
12. Expand fallback strategies into smarter per-recipe fallback chains with multiple alternate shapes and user-configurable lock relaxation rules.
13. Expand musical note-range validation from root/octave guards into per-step bass, stab, pluck, and FX role checks.
14. Add recipe-specific chord/scale seeding.
15. Add macro naming and macro range generation per recipe.
16. Add favorite/recent note templates and let users edit their template library.
17. Add one-click "save generated pack" for a set of variations.
18. Add a full recipe browser/table with search, genre filters, favorites, and custom user recipes.

## House And UKG-Specific Generator Ideas

1. UKG Dred/Reese bass generator with mono sub, moving upper harmonics, lowpass opening, restrained width, and glide.
2. UKG organ stab generator with drawbar-like harmonic levels, short gate, minor 7/minor 9 chord memory, and delay throw macro.
3. UKG vocal chop generator with slice choice, per-slice pitch, per-slice pan, choke, reverse pickup, and swing.
4. Speed garage bass generator with clipped upper harmonics, offbeat pump, and tight low-end guard.
5. House chord generator with filtered minor/major 7 stabs, slow filter env, width, and reverb throw.
6. Tech-house rubber bass generator with short envelopes, filter snap, controlled drive, and 1/16 or 1/8 groove.
7. Minimal click/pluck generator with short decay, random step gaps, comb/phaser movement, and low FX mix.
8. Warehouse techno stab generator with darker filter, drive, reverb size, and guarded output.
9. Dub techno pulse generator with chord stab, filtered delay, noise layer, and macro-controlled space.
10. FX riser/drop generator using noise, pump, delay/reverb throws, and guard.

## Browser And Save Backlog

1. Add generated-preset compare/revert and richer source-recipe notes.
2. Add smart folders for Five Star, Favorites, Recent, Generated, UKG Bass, UKG Chops, House Chords, Tech House Bass, Minimal Plucks, and Dirty Techno.
3. Add preset compare/revert before overwriting an existing preset.
4. Add duplicate-name prompt instead of silently replacing older folder copies.
5. Add preset version history for user presets.
6. Add per-preset notes and "source recipe" metadata.
7. Add audio preview rendering for factory and user presets.
8. Add preview phrase selection by preset role: bass, stab, pluck, chord, FX, chop.
9. Add missing sample/wavetable dependency warnings directly in Library rows.
10. Add preset pack import/export with folder structure and metadata manifest.
11. Add "collect and save" for samples/wavetables used by a preset pack.
12. Add find-similar search based on tags, the new macro value strip, and parameter distance.
13. Add author/pack/category bulk edit for user presets.
14. Add browser keyboard shortcuts for favorite, rating, load, save as, and audition.
15. Add rating/favorite bias to random recipe selection.

## UI Direction

1. Keep LAB as the dedicated randomization page.
2. Keep HOME as the fast performance view with a compact randomization entry point.
3. Keep LIBRARY as the deep browser/save metadata view.
4. The Random Lab should eventually become a two-column workflow:
   - Left: recipe, scope, locks, section intensities, and generator buttons.
   - Right: candidates, history, diff, save draft, and audition controls.
5. Avoid showing every randomization option at once. Use focused sub-tabs or collapsible groups once the workflow grows.
6. Use icons for save, undo, redo, favorite, audition, lock, and refresh once the UI component library is extracted.
7. Add visual "changed" badges to sections after randomization.

## Next Implementation Slice

Best next slice: add deeper generated-candidate history and candidate-row favorite/rating handoff.

Why:
- It directly supports randomization as the best feature.
- It keeps users from losing good ideas while exploring.
- It naturally connects candidate slots to favorite, rating, notes, and browser organization.
- It can be implemented without changing plugin parameter IDs.

Candidate polish behavior:
- Expand four candidate slots into a scrollable history once the compact slot workflow is stable.
- Candidate rows can carry favorite/rating actions before or after save-from-slot.
- Candidate rows can carry short notes such as "best bass", "too bright", or "save for B".
- Library can group generated preset variants into a smart collection once generated-save metadata exists.
