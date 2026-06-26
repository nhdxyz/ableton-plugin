# Nate VST

Mac-first VST3 instrument plugin for Ableton Live, focused on house, tech house, techno, minimal, and related electronic production.

The long-term direction is a hybrid synth with subtractive synthesis, wavetable-style movement, sampling/slicing, random sound generation, internal pattern tools, and an effects area.

## Current Target

- macOS first.
- Ableton Live as the main host.
- VST3 first.
- JUCE + CMake.
- C++20.

## Product Direction

The first useful version should load in Ableton, respond to MIDI, generate sound, expose automatable parameters, and save state in a Live Set.

Planned major areas:

- Synth engine: oscillator 1, oscillator 2, sub, noise, unison/spread, envelopes, multimode filter, drive, modulation.
- Sound Lab: guided randomization, mutation, recipes, and safety rules.
- Sample Lab: load audio, cut it with start/end controls, reverse/pitch/gain/mix it, and trigger it from MIDI.
- Sequencer: internal 16-step piano-roll style pattern area for riffs, basslines, and generated ideas.
- FX: post-synth/post-sample distortion, chorus, delay, reverb, output trim, and safety clipping.
- Library: save and load categorized `.natevstpreset` files from the user preset folder.

## Interface

The editor uses focused panels instead of one tall stacked page:

- `HOME`
- `SYNTH`
- `LAB`
- `SAMPLE`
- `SEQ`
- `FX`
- `LIBRARY`

The `HOME` panel is the default dashboard for quick patch shaping, source mixing, unison width, performance macros, randomization, and preset recall. Waveform, oscillator 2 waveform, filter mode, and sequencer-rate controls use segmented selectors. Longer lists, such as recipes and presets, stay as browser-style dropdowns for now.

User presets are stored at:

```text
~/Library/Application Support/Nate VST/Presets
```

See [PLAN.md](./PLAN.md) for the full roadmap and [docs/MODULATION_WORKFLOW.md](./docs/MODULATION_WORKFLOW.md) for the first modulation/performance design.

## Local Development

Expected tools:

- Xcode or Xcode Command Line Tools.
- CMake.
- Ninja or Make.
- Ableton Live for host testing.
- pluginval for validation later.

The initial project scaffold uses CMake FetchContent to download JUCE during configuration.

## Build

Once CMake is installed:

```sh
cmake -S . -B build -G Ninja
cmake --build build --config Debug
```

The generated VST3 can be copied to:

```text
~/Library/Audio/Plug-Ins/VST3
```

See [CHANGELOG.md](./CHANGELOG.md) for detailed implementation notes by iteration.
