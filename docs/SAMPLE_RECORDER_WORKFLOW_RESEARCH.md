# Sample Recorder Workflow Research

The current recorder should stay compact in the SAMPLE panel, but it needs the same confidence signals producers expect from mature sampling workflows: obvious source selection, visible incoming level, fast commit/edit actions, and clear next steps after capture.

## References Checked

- [Ableton Live recording and monitoring](https://help.ableton.com/hc/en-us/articles/7938193554460-Recording-Audio): source routing and monitor state need to be explicit before recording audio.
- [Ableton Simpler](https://www.ableton.com/en/manual/live-instrument-reference/): sampler workflows depend on fast warp/slice/edit handoff after a sound is captured or loaded.
- [Logic Pro Quick Sampler Recorder mode](https://support.apple.com/es-us/guide/logicpro/lgcpa8e3df79/mac): useful recorder controls include input selection, monitor, immediate start, and threshold start.
- [Native Instruments Maschine sampling](https://www.native-instruments.com/ni-tech-manuals/maschine-mk3-manual/en/sampling-and-sample-mapping): strong recorder patterns include Detect threshold, Sync start, fixed bar lengths, waiting states, and stop/cancel behavior.
- [FL Studio Edison](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Edison.htm): recorder placement in the signal chain determines dry/wet capture, and the recorder should make memory/export handoff clear.
- [Serato Sample Cues](https://support.serato.com/hc/en-us/articles/115000501614-Cues) and [Autoset](https://support.serato.com/hc/en-us/articles/115000489113-Autoset): cue pads, Autoset, and manual cue creation make the post-record chopping path fast.

## What We Already Have

- Source selection between `Post-FX Output` and `Host Input`.
- Post-FX capture records synth engine, sampler playback, FX rack, and output gain.
- Host Input capture records audio routed by Ableton into the plugin input.
- Rolling 16-second buffer with progress text and rolling/full state.
- `REC -> READY -> USE -> PLAY` flow rail.
- Commit, Play, Trim, Splice, and Mangle actions that enable only when usable.
- Recorder CTest coverage for capture, commit, overview creation, playback, Post-FX gain capture, and Host Input capture.
- Start mode selector with `Immediate`, `Detect -36 dB`, `Detect -24 dB`, and `Detect -12 dB`.
- Threshold-arm state that waits for the selected source to cross the selected threshold before filling the capture buffer.
- Length selector with `Free`, `1 Bar`, `2 Bars`, `4 Bars`, and `8 Bars`; bar lengths auto-stop from the host tempo at record start.

## Added First

- Visible source-level readout in the recorder status line, e.g. `Post-FX -18 dB` or `Host In -inf dB`.
- Post-FX and Host Input source-level telemetry in `SampleRecorderAudit`.
- Threshold-start audit coverage for below-threshold waiting, above-threshold capture start, and sampler commit.
- Fixed-length audit coverage for one-bar auto-stop timing and sampler commit.

## Next Recorder Priorities

1. Pre-roll buffer: keep a short lookback so threshold recording does not clip the attack.
2. Quantized transport start/stop: optionally wait for the next bar before fixed-length capture starts.
3. Monitor clarity: add a small Host Input route hint and feedback-safe monitor state.
4. Take handling: keep the last few captures until one is committed.
5. Drag/export audio: drag the committed capture as WAV into Ableton.
6. Slice while recording: add marker taps or lazy-chop pads during capture.
7. Normalize/fade options: post-commit level, fade-in, fade-out, and click cleanup.
