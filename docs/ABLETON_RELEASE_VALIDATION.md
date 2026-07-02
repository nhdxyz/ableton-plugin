# Ableton Release Validation

Run the automated gate first:

```sh
tools/validate_release.sh
```

Then run this manual Ableton pass against the built VST3.

## Host Scan And Load

- Run `tools/install_vst3.sh` or confirm `Nate VST.vst3` in `~/Library/Audio/Plug-Ins/VST3`.
- Rescan plugins in Ableton Live.
- Load Nate VST on a fresh MIDI track.
- Confirm the editor opens, resizes, and switches every main panel.
- Confirm the output meter and bottom keyboard respond to mouse audition.

## MIDI And Automation

- Play a MIDI clip into the plugin with the internal sequencer off.
- Enable the internal sequencer and confirm it follows Ableton tempo/transport.
- Automate cutoff, output gain, macro 1, sample start, pump depth, and delay mix.
- Save, close, and reopen the Live Set; confirm automation lanes still target the intended controls.

## State Save/Reopen

- Load a factory preset, edit macros, edit FX order, edit a sequencer scene, and save the Live Set.
- Reopen the set and confirm the preset state, scene state, FX order, and macro values restore.
- Duplicate the track and confirm the duplicate sounds identical but can be edited independently.
- Test Undo/Redo Edit in the plugin after reopening.

## Sampler And Recording

- Record the plugin post-FX output into the sampler.
- Route a mic or audio track into the plugin input and record Host Input.
- Commit the capture, trim, splice, select slices, and save/reopen the Live Set.
- Move or rename the sample file and confirm missing-sample UI behavior is readable.

## Drag, Freeze, And Export

- Drag a single sequencer MIDI clip into Ableton.
- Drag a scene-chain MIDI clip into Ableton.
- Freeze the track, flatten it, and confirm the rendered audio is finite and aligned.
- Export a short WAV from Ableton and reopen the project.

## Safety Checks

- Play the loudest factory bass and stab presets with Guard on and off.
- Confirm no stuck notes after rapid MIDI input, transport stop/start, and plugin disable/enable.
- Confirm the plugin does not clip unexpectedly at default preset levels.
- Confirm CPU remains reasonable during factory preset audition and sequencer playback.

Record the Ableton version, macOS version, CPU architecture, VST3 path, and any failures in the release notes.
