# Ableton Release Validation

Run the automated gate first:

```sh
tools/validate_release.sh
```

If pluginval is not installed locally, download the official release binary into `build/pluginval-bin/` and run the same gate:

```sh
PLUGINVAL_AUTO_DOWNLOAD=1 tools/validate_release.sh
```

GitHub Actions also runs pluginval at strictness 5. CI sets `PLUGINVAL_SKIP_GUI_TESTS=1` because the hosted runner is headless; local release validation should keep GUI tests enabled unless the exception is documented in the release notes.

Create a local release archive after the gate passes:

```sh
tools/package_release.sh
```

Create a macOS installer package for beta testers:

```sh
CREATE_PKG=1 tools/package_release.sh
```

The package script normalizes the generated `.pkg` payload and fails if AppleDouble `._*` or `.DS_Store` records remain in the installer BOM.

For a public release, sign the VST3 bundle and installer package with Developer ID identities:

```sh
CREATE_PKG=1 \
CODE_SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)" \
INSTALLER_SIGN_IDENTITY="Developer ID Installer: Your Name (TEAMID)" \
tools/package_release.sh
```

To notarize and staple the signed installer package, provide notarytool credentials. A stored keychain profile is preferred for local release machines:

```sh
xcrun notarytool store-credentials "nate-vst-notary" \
  --apple-id "you@example.com" \
  --team-id "TEAMID" \
  --password "app-specific-password"

CREATE_PKG=1 NOTARIZE=1 NOTARY_PROFILE="nate-vst-notary" \
CODE_SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)" \
INSTALLER_SIGN_IDENTITY="Developer ID Installer: Your Name (TEAMID)" \
tools/package_release.sh
```

The automated gate writes a release evidence bundle under:

```text
build/release-validation/
```

Attach `summary.txt`, `ctest-output.txt`, and the pluginval report to release notes or the release issue. Then run this manual Ableton pass against the exact built VST3 named in `summary.txt`.

## Test Matrix

Complete at least the required row before sharing a beta. Complete every row before tagging a public release.

| Scope | macOS | CPU | Ableton | Sample rates | Buffer sizes | Required |
| --- | --- | --- | --- | --- | --- | --- |
| Local primary | Current dev macOS | Current machine | Current Live 12 | 44.1, 48 kHz | 64, 128, 512 | Yes |
| Low-latency stress | Current dev macOS | Current machine | Current Live 12 | 48 kHz | 32, 64 | Yes |
| High-rate stress | Current dev macOS | Current machine | Current Live 12 | 96 kHz | 128, 512 | Yes |
| Compatibility smoke | Oldest supported macOS available | Intel or Rosetta if available | Live 11 or oldest supported Live | 44.1 kHz | 128, 512 | Public release |
| Apple Silicon smoke | Current supported macOS | Apple Silicon native | Current Live 12 | 48 kHz | 128 | Public release |

Record results in this format:

```text
Date:
Commit:
Package/version:
macOS:
CPU:
Ableton version:
VST3 path:
Sample rate:
Buffer size:
Pluginval report:
CTest report:
Result:
Notes:
```

## Host Scan And Load

- Run `tools/install_vst3.sh` or confirm `Nate VST.vst3` in `~/Library/Audio/Plug-Ins/VST3`.
- Rescan plugins in Ableton Live.
- Load Nate VST on a fresh MIDI track.
- Confirm the editor opens, resizes, and switches every main panel.
- Confirm the output meter and bottom keyboard respond to mouse audition.
- Instantiate at least four plugin copies in one set and confirm each instance opens independently.
- Toggle plugin enable/disable and confirm no stuck notes, loud bursts, or lost editor state.

## MIDI And Automation

- Play a MIDI clip into the plugin with the internal sequencer off.
- Enable the internal sequencer and confirm it follows Ableton tempo/transport.
- Automate cutoff, output gain, macro 1, sample start, pump depth, and delay mix.
- Save, close, and reopen the Live Set; confirm automation lanes still target the intended controls.
- Automate Osc 1/Osc 2 WT position and one macro assignment route.
- Record automation from the HOME macro/XY controls into Arrangement view.
- Move the Live loop brace and restart transport; confirm SEQ and Pump phase recover cleanly.

## State Save/Reopen

- Load a factory preset, edit macros, edit FX order, edit a sequencer scene, and save the Live Set.
- Reopen the set and confirm the preset state, scene state, FX order, and macro values restore.
- Duplicate the track and confirm the duplicate sounds identical but can be edited independently.
- Test Undo/Redo Edit in the plugin after reopening.
- Save/reopen after changing wavetable frames, Source Lab frame-slot edits, and Wave Tools stack transforms.
- Save/reopen after changing MOD routes, route bypass states, route shape/polarity/slew, and macro assignments.
- Save/reopen after loading a user preset from a nested category folder.

## Sampler And Recording

- Record the plugin post-FX output into the sampler.
- Route a mic or audio track into the plugin input and record Host Input.
- Confirm the recorder status line shows the selected source and a moving dB level before and during recording.
- Commit the capture, trim, splice, select slices, and save/reopen the Live Set.
- Move or rename the sample file and confirm missing-sample UI behavior is readable.
- Test threshold recording with pre-roll at quiet and loud input levels.
- Test one-bar and four-bar fixed-length recording while Live is stopped and while Live is playing.
- Save/reopen Slice Keys state: regions, pitch, gain, pan, chance, reverse, stutter, choke, nudge, and fade.

## Drag, Freeze, And Export

- Drag a single sequencer MIDI clip into Ableton.
- Drag a scene-chain MIDI clip into Ableton.
- Freeze the track, flatten it, and confirm the rendered audio is finite and aligned.
- Export a short WAV from Ableton and reopen the project.
- Confirm dragged MIDI preserves ratchets, conditions, slide overlap, chord colors, groove timing, and scene-chain length mode.
- Duplicate a track after dragging MIDI and confirm temporary drag files are not required for the set to reopen.
- Export and reimport a Nate VST preset file if release notes claim preset portability.

## Safety Checks

- Review `ProcessorPerformanceAudit` output from CTest for idle, poly synth + FX, and sequencer + FX realtime ratios.
- Play the loudest factory bass and stab presets with Guard on and off.
- Confirm no stuck notes after rapid MIDI input, transport stop/start, and plugin disable/enable.
- Confirm the plugin does not clip unexpectedly at default preset levels.
- Confirm CPU remains reasonable during factory preset audition and sequencer playback.
- Sweep factory presets by category: Bass, Chords/Stabs, Leads, Pads, FX, Sequenced, Sample/Chop, and Construction Kits.
- For each category, note the quietest, hottest, and most CPU-heavy preset.
- Confirm mono/sub behavior stays reasonable on bass presets with Width enabled.
- Confirm preview audition does not alter the current loaded patch unless the user explicitly loads a preset.

## Failure Rules

- Any crash, stuck note, non-finite audio, uncontrolled full-scale burst, or lost saved-state item blocks release.
- Any pluginval failure at the selected strictness blocks release unless a written exception is added to release notes.
- Any Ableton save/reopen failure blocks release.
- Any missing-sample behavior that silently plays stale audio blocks release.
- UI clipping that hides primary controls at the supported editor sizes blocks release.
- Preset loudness inconsistency does not automatically block an internal beta, but public release notes must identify known loudness spread until role-targeted loudness validation is implemented.

Record the Ableton version, macOS version, CPU architecture, VST3 path, and any failures in the release notes.
