#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
JOBS="${JOBS:-}"
PLUGIN_SOURCE="${PLUGIN_SOURCE:-"$BUILD_DIR/NateVST_artefacts/VST3/Nate VST.vst3"}"
PLUGIN_DEST="${PLUGIN_DEST:-"$HOME/Library/Audio/Plug-Ins/VST3/Nate VST.vst3"}"
PRESETS_SOURCE="${PRESETS_SOURCE:-"$ROOT_DIR/Resources/Factory Presets"}"
PRESETS_DEST="${PRESETS_DEST:-"$HOME/Library/Application Support/Nate VST/Factory Presets"}"
SKIP_BUILD="${SKIP_BUILD:-0}"
INSTALL_PRESETS="${INSTALL_PRESETS:-1}"
DRY_RUN="${DRY_RUN:-0}"

if [[ -z "$JOBS" ]]; then
    if command -v sysctl >/dev/null 2>&1; then
        JOBS="$(sysctl -n hw.ncpu 2>/dev/null || true)"
    fi
    JOBS="${JOBS:-4}"
fi

run() {
    if [[ "$DRY_RUN" == "1" ]]; then
        printf 'DRY RUN:'
        printf ' %q' "$@"
        printf '\n'
    else
        "$@"
    fi
}

copy_bundle() {
    local source="$1"
    local dest="$2"
    local parent
    parent="$(dirname "$dest")"

    if [[ ! -d "$source" && ! -f "$source" ]]; then
        echo "Missing source: $source" >&2
        exit 1
    fi

    run mkdir -p "$parent"
    run rm -rf "$dest"
    run cp -R "$source" "$dest"
}

echo "== Nate VST install =="
echo "Root: $ROOT_DIR"
echo "Build: $BUILD_DIR"
echo "Plugin source: $PLUGIN_SOURCE"
echo "Plugin destination: $PLUGIN_DEST"
echo "Preset source: $PRESETS_SOURCE"
echo "Preset destination: $PRESETS_DEST"
echo

if [[ "$SKIP_BUILD" != "1" ]]; then
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "Build directory does not exist: $BUILD_DIR" >&2
        echo "Run: cmake -S \"$ROOT_DIR\" -B \"$BUILD_DIR\"" >&2
        exit 1
    fi

    echo "== Build VST3 =="
    cmake --build "$BUILD_DIR" --target NateVST -j "$JOBS"
    echo
fi

echo "== Install VST3 =="
copy_bundle "$PLUGIN_SOURCE" "$PLUGIN_DEST"
echo

if [[ "$INSTALL_PRESETS" == "1" ]]; then
    echo "== Install factory presets =="
    copy_bundle "$PRESETS_SOURCE" "$PRESETS_DEST"
    echo
fi

echo "Install workflow completed."
