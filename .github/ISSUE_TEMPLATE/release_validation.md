---
name: Release validation
about: Track the automated and Ableton release gate for a Nate VST build
title: "Release validation: vX.Y.Z"
labels: release, validation
assignees: ""
---

## Build

- Version:
- Commit:
- Package path:
- Package manifest:
- Package summary:
- VST3 path:
- Validation summary:
- CTest log:
- pluginval report:
- Signed package path:
- Notarization report:
- Result:

## Automated Gate

- [ ] `tools/validate_release.sh` passed
- [ ] CTest passed
- [ ] pluginval passed or exception documented
- [ ] Release package was created with `tools/package_release.sh`
- [ ] `RELEASE_MANIFEST.txt` and `*-release-summary.txt` were attached or linked
- [ ] `.pkg` installer was created with `CREATE_PKG=1`
- [ ] VST3 bundle was signed or unsigned beta exception documented
- [ ] Installer package was signed or unsigned beta exception documented
- [ ] Installer package was notarized/stapled or exception documented
- [ ] Release validation artifact was attached or linked

## Ableton Matrix

| Scope | macOS | CPU | Ableton | Sample rates | Buffer sizes | Result | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Local primary |  |  |  | 44.1, 48 kHz | 64, 128, 512 |  |  |
| Low-latency stress |  |  |  | 48 kHz | 32, 64 |  |  |
| High-rate stress |  |  |  | 96 kHz | 128, 512 |  |  |
| Compatibility smoke |  |  |  | 44.1 kHz | 128, 512 |  |  |
| Apple Silicon smoke |  |  |  | 48 kHz | 128 |  |  |

## Host Scan And Load

- [ ] Ableton rescans the VST3 cleanly
- [ ] Nate VST loads on a fresh MIDI track
- [ ] Editor opens, resizes, and switches every main panel
- [ ] Output meter and bottom keyboard respond to mouse audition
- [ ] Four plugin copies open independently
- [ ] Plugin enable/disable causes no stuck notes, loud bursts, or lost editor state

## MIDI, Automation, And State

- [ ] MIDI clip playback works
- [ ] Internal sequencer follows Ableton tempo/transport
- [ ] Automation lanes target the intended controls after save/reopen
- [ ] WT position, macro route, HOME macro, and XY automation work
- [ ] Loop brace moves and transport restarts recover SEQ/Pump phase cleanly
- [ ] Preset state, scene state, FX order, macro values, wavetable frame edits, MOD routes, and macro assignments restore
- [ ] Duplicate track sounds identical and edits independently

## Sampler And Recording

- [ ] WAV or AIFF drag/drop loads
- [ ] Missing-sample behavior is readable
- [ ] Recorder level moves before and during recording
- [ ] Host Input capture works
- [ ] Post-FX capture works
- [ ] Threshold/pre-roll capture works
- [ ] One-bar and four-bar fixed-length capture work while stopped and playing
- [ ] Slice Keys state saves and reopens

## Drag, Freeze, And Export

- [ ] Scene-chain MIDI clip drags into Ableton
- [ ] Dragged MIDI preserves ratchets, conditions, slide overlap, chord colors, groove timing, and scene-chain length mode
- [ ] Freeze/flatten renders finite aligned audio
- [ ] Exported WAV reopens correctly
- [ ] Preset export/reimport works if release notes claim preset portability

## Safety

- [ ] No stuck notes after rapid MIDI input, transport stop/start, and plugin disable/enable
- [ ] Default preset levels do not clip unexpectedly
- [ ] CPU remains reasonable during preset audition and sequencer playback
- [ ] Factory preset category sweep completed
- [ ] Quietest, hottest, and most CPU-heavy presets noted
- [ ] Mono/sub behavior stays reasonable on bass presets with Width enabled
- [ ] Preview audition does not alter the current loaded patch unless explicitly loaded

## Blocking Failures

- Crash, stuck note, non-finite audio, uncontrolled full-scale burst, or lost saved state:
- pluginval failure or exception:
- Ableton save/reopen failure:
- Missing-sample stale-audio behavior:
- UI clipping that hides primary controls:
- Preset loudness concerns:
