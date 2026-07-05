#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
VERSION="${VERSION:-}"
PLUGIN_PATH="${PLUGIN_PATH:-"$BUILD_DIR/NateVST_artefacts/VST3/Nate VST.vst3"}"
RELEASE_VALIDATION_DIR="${RELEASE_VALIDATION_DIR:-"$BUILD_DIR/release-validation"}"
RELEASE_VALIDATION_REPORT="${RELEASE_VALIDATION_REPORT:-"$RELEASE_VALIDATION_DIR/summary.txt"}"
CTEST_OUTPUT_FILE="${CTEST_OUTPUT_FILE:-"$RELEASE_VALIDATION_DIR/ctest-output.txt"}"
PLUGINVAL_OUTPUT_DIR="${PLUGINVAL_OUTPUT_DIR:-"$BUILD_DIR/pluginval"}"
PLUGINVAL_OUTPUT_FILE="${PLUGINVAL_OUTPUT_FILE:-nate-vst-pluginval.txt}"
ABLETON_REPORT_PATH="${ABLETON_REPORT_PATH:-"$RELEASE_VALIDATION_DIR/ableton-validation-report.md"}"
PACKAGE_DIR="${PACKAGE_DIR:-}"
PACKAGE_MANIFEST_PATH="${PACKAGE_MANIFEST_PATH:-}"
PACKAGE_SUMMARY_PATH="${PACKAGE_SUMMARY_PATH:-}"
ZIP_PATH="${ZIP_PATH:-}"
PKG_PATH="${PKG_PATH:-}"

if [[ -z "$VERSION" ]]; then
    VERSION="$(sed -nE 's/^project\(NateVST VERSION ([^ ]+) .*/\1/p' "$ROOT_DIR/CMakeLists.txt" | head -n 1)"
    VERSION="${VERSION:-unknown}"
fi

GIT_REVISION="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || printf 'unknown')"
GIT_STATUS="$(git -C "$ROOT_DIR" status --short 2>/dev/null || true)"
CREATED_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
MACOS_VERSION="$(sw_vers -productVersion 2>/dev/null || printf 'unknown')"
CPU_ARCH="$(uname -m 2>/dev/null || printf 'unknown')"
PLUGINVAL_REPORT="$PLUGINVAL_OUTPUT_DIR/$PLUGINVAL_OUTPUT_FILE"

path_or_pending() {
    local path="$1"
    if [[ -n "$path" ]]; then
        printf '%s\n' "$path"
    else
        printf 'pending\n'
    fi
}

mkdir -p "$(dirname "$ABLETON_REPORT_PATH")"

cat > "$ABLETON_REPORT_PATH" <<EOF
# Nate VST Ableton Validation Report

This report is for recording the manual Ableton pass against one exact build. Do not reuse it for another commit or package.

## Build Evidence

| Field | Value |
| --- | --- |
| Created UTC | $CREATED_UTC |
| Version | $VERSION |
| Commit | $GIT_REVISION |
| macOS used to create report | $MACOS_VERSION |
| CPU architecture | $CPU_ARCH |
| VST3 path | $PLUGIN_PATH |
| Release validation summary | $RELEASE_VALIDATION_REPORT |
| CTest log | $CTEST_OUTPUT_FILE |
| pluginval report | $PLUGINVAL_REPORT |
| Package folder | $(path_or_pending "$PACKAGE_DIR") |
| Package manifest | $(path_or_pending "$PACKAGE_MANIFEST_PATH") |
| Package summary | $(path_or_pending "$PACKAGE_SUMMARY_PATH") |
| ZIP artifact | $(path_or_pending "$ZIP_PATH") |
| PKG artifact | $(path_or_pending "$PKG_PATH") |

## Git Status At Report Creation

\`\`\`text
${GIT_STATUS:-clean}
\`\`\`

## Manual Run Metadata

| Field | Value |
| --- | --- |
| Tester |  |
| Date |  |
| Ableton Live version |  |
| macOS version |  |
| CPU |  |
| Audio interface |  |
| VST3 installed path |  |
| Sample rate |  |
| Buffer size |  |
| Result | TODO |
| Notes |  |

## Matrix

Use PASS, FAIL, BLOCKED, or N/A in the Result column. Required rows must pass before a beta is shared.

| Scope | macOS | CPU | Ableton | Sample rates | Buffer sizes | Required | Result | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Local primary |  |  | Current Live 12 | 44.1, 48 kHz | 64, 128, 512 | Yes | TODO |  |
| Low-latency stress |  |  | Current Live 12 | 48 kHz | 32, 64 | Yes | TODO |  |
| High-rate stress |  |  | Current Live 12 | 96 kHz | 128, 512 | Yes | TODO |  |
| Compatibility smoke |  |  | Live 11 or oldest supported Live | 44.1 kHz | 128, 512 | Public release | TODO |  |
| Apple Silicon smoke |  |  | Current Live 12 | 48 kHz | 128 | Public release | TODO |  |

## Host Scan And Load

| Check | Result | Notes |
| --- | --- | --- |
| Install or copy the exact VST3 above into the user VST3 folder | TODO |  |
| Ableton rescans the VST3 cleanly | TODO |  |
| Nate VST loads on a fresh MIDI track | TODO |  |
| Editor opens, resizes, and switches every main panel | TODO |  |
| Output meter and bottom keyboard respond to mouse audition | TODO |  |
| Four plugin copies open independently | TODO |  |
| Plugin enable/disable causes no stuck notes, loud bursts, or lost editor state | TODO |  |

## MIDI, Automation, And State

| Check | Result | Notes |
| --- | --- | --- |
| MIDI clip playback works with internal sequencer off | TODO |  |
| Internal sequencer follows Ableton tempo and transport | TODO |  |
| Automate cutoff, output gain, macro 1, sample start, pump depth, and delay mix | TODO |  |
| Save/reopen keeps automation lanes targeting the intended controls | TODO |  |
| Osc 1/Osc 2 WT position automation works | TODO |  |
| Macro assignment route automation works | TODO |  |
| HOME macro/XY automation records into Arrangement view | TODO |  |
| Moving the Live loop brace and restarting transport recovers SEQ/Pump phase | TODO |  |
| Preset state restores after save/reopen | TODO |  |
| Sequencer scenes restore after save/reopen | TODO |  |
| FX order restores after save/reopen | TODO |  |
| Wavetable frame edits restore after save/reopen | TODO |  |
| MOD routes, bypass states, shape/polarity/slew, and macro assignments restore | TODO |  |
| Duplicate track sounds identical and edits independently | TODO |  |
| Plugin Undo/Redo Edit works after reopening | TODO |  |

## Sampler And Recording

| Check | Result | Notes |
| --- | --- | --- |
| WAV or AIFF load works from the SAMPLE panel | TODO |  |
| Missing-sample behavior is readable after moving or renaming the sample file | TODO |  |
| Recorder status line shows selected source and moving dB before recording | TODO |  |
| Post-FX Output capture records synth/sampler/FX/output gain | TODO |  |
| Host Input capture records audio routed into the plugin input | TODO |  |
| Threshold recording starts only above threshold | TODO |  |
| Pre-roll prepends audio before threshold trigger | TODO |  |
| One-bar fixed capture works while Live is stopped | TODO |  |
| Four-bar fixed capture works while Live is playing | TODO |  |
| Commit, trim, splice, and slice selection work on a capture | TODO |  |
| Slice Keys state saves/reopens: regions, pitch, gain, pan, chance, reverse, stutter, choke, nudge, fade | TODO |  |
| Recorder WAV export/reveal/drag target points at the selected take | TODO |  |

## Drag, Freeze, And Export

| Check | Result | Notes |
| --- | --- | --- |
| Single-pattern MIDI clip drags into Ableton | TODO |  |
| Scene-chain MIDI clip drags into Ableton | TODO |  |
| Dragged MIDI preserves ratchets, conditions, slide overlap, chord colors, groove timing, and scene-chain length mode | TODO |  |
| Duplicate a track after MIDI drag and reopen without requiring temp drag files | TODO |  |
| Freeze renders finite aligned audio | TODO |  |
| Flatten renders finite aligned audio | TODO |  |
| Exported WAV from Ableton reopens correctly | TODO |  |
| Preset export/reimport works if release notes claim preset portability | TODO |  |

## Safety And Factory Sweep

| Check | Result | Notes |
| --- | --- | --- |
| No stuck notes after rapid MIDI input | TODO |  |
| No stuck notes after transport stop/start | TODO |  |
| No stuck notes after plugin disable/enable | TODO |  |
| Default preset levels do not clip unexpectedly | TODO |  |
| CPU remains reasonable during factory preset audition | TODO |  |
| CPU remains reasonable during sequencer playback | TODO |  |
| Factory preset category sweep completed | TODO |  |
| Quietest preset noted | TODO |  |
| Hottest preset noted | TODO |  |
| Most CPU-heavy preset noted | TODO |  |
| Mono/sub behavior stays reasonable on bass presets with Width enabled | TODO |  |
| Preview audition does not alter the current loaded patch unless explicitly loaded | TODO |  |

## Blocking Failures

| Failure class | Status | Details |
| --- | --- | --- |
| Crash, stuck note, non-finite audio, uncontrolled full-scale burst, or lost saved state | TODO |  |
| pluginval failure or exception | TODO |  |
| Ableton save/reopen failure | TODO |  |
| Missing-sample stale-audio behavior | TODO |  |
| UI clipping that hides primary controls | TODO |  |
| Preset loudness concern | TODO |  |

## Final Decision

| Field | Value |
| --- | --- |
| Release approved | TODO |
| Approved by |  |
| Approval UTC |  |
| Required follow-up issues |  |
EOF

echo "Ableton validation report: $ABLETON_REPORT_PATH"
