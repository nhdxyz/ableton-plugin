#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
JOBS="${JOBS:-}"
PLUGIN_PATH="${PLUGIN_PATH:-"$BUILD_DIR/NateVST_artefacts/VST3/Nate VST.vst3"}"
PLUGINVAL_BIN="${PLUGINVAL_BIN:-pluginval}"
PLUGINVAL_STRICTNESS="${PLUGINVAL_STRICTNESS:-5}"
PLUGINVAL_TIMEOUT_MS="${PLUGINVAL_TIMEOUT_MS:-60000}"
PLUGINVAL_OUTPUT_DIR="${PLUGINVAL_OUTPUT_DIR:-"$BUILD_DIR/pluginval"}"
PLUGINVAL_OUTPUT_FILE="${PLUGINVAL_OUTPUT_FILE:-nate-vst-pluginval.txt}"
SKIP_PLUGINVAL="${SKIP_PLUGINVAL:-0}"

resolve_pluginval() {
    if command -v "$PLUGINVAL_BIN" >/dev/null 2>&1; then
        command -v "$PLUGINVAL_BIN"
        return 0
    fi

    for candidate in \
        "/Applications/pluginval.app/Contents/MacOS/pluginval" \
        "$HOME/Applications/pluginval.app/Contents/MacOS/pluginval"
    do
        if [[ -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

if [[ -z "$JOBS" ]]; then
    if command -v sysctl >/dev/null 2>&1; then
        JOBS="$(sysctl -n hw.ncpu 2>/dev/null || true)"
    fi
    JOBS="${JOBS:-4}"
fi

echo "== Nate VST release validation =="
echo "Root: $ROOT_DIR"
echo "Build: $BUILD_DIR"
echo

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Build directory does not exist: $BUILD_DIR" >&2
    echo "Run: cmake -S \"$ROOT_DIR\" -B \"$BUILD_DIR\"" >&2
    exit 1
fi

echo "== Build VST3 =="
cmake --build "$BUILD_DIR" --target NateVST -j "$JOBS"
echo

if [[ ! -d "$PLUGIN_PATH" && ! -f "$PLUGIN_PATH" ]]; then
    echo "Built plugin was not found at: $PLUGIN_PATH" >&2
    exit 1
fi

echo "== CTest suite =="
ctest --test-dir "$BUILD_DIR" --output-on-failure
echo

if [[ "$SKIP_PLUGINVAL" == "1" ]]; then
    echo "== pluginval =="
    echo "Skipped because SKIP_PLUGINVAL=1"
    echo
else
    if ! PLUGINVAL_RESOLVED="$(resolve_pluginval)"; then
        echo "pluginval is required for release validation but was not found: $PLUGINVAL_BIN" >&2
        echo "Install pluginval or set PLUGINVAL_BIN=/absolute/path/to/pluginval." >&2
        echo "For local smoke runs only, set SKIP_PLUGINVAL=1." >&2
        exit 1
    fi

    mkdir -p "$PLUGINVAL_OUTPUT_DIR"
    echo "== pluginval =="
    "$PLUGINVAL_RESOLVED" \
        --strictness-level "$PLUGINVAL_STRICTNESS" \
        --timeout-ms "$PLUGINVAL_TIMEOUT_MS" \
        --output-dir "$PLUGINVAL_OUTPUT_DIR" \
        --output-filename "$PLUGINVAL_OUTPUT_FILE" \
        --validate "$PLUGIN_PATH"
    echo
fi

echo "== Ableton manual validation =="
echo "Complete the checklist in docs/ABLETON_RELEASE_VALIDATION.md before tagging a release."
echo
echo "Release validation completed."
