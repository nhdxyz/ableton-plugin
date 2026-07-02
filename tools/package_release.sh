#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
JOBS="${JOBS:-}"
VERSION="${VERSION:-}"
DIST_DIR="${DIST_DIR:-"$ROOT_DIR/dist"}"
RUN_VALIDATE="${RUN_VALIDATE:-1}"
SKIP_PLUGINVAL="${SKIP_PLUGINVAL:-0}"
DRY_RUN="${DRY_RUN:-0}"

if [[ -z "$VERSION" ]]; then
    VERSION="$(sed -nE 's/^project\(NateVST VERSION ([^ ]+) .*/\1/p' "$ROOT_DIR/CMakeLists.txt" | head -n 1)"
    VERSION="${VERSION:-dev}"
fi

PACKAGE_NAME="${PACKAGE_NAME:-"NateVST-$VERSION-macos"}"
PACKAGE_DIR="${PACKAGE_DIR:-"$DIST_DIR/$PACKAGE_NAME"}"
ZIP_PATH="${ZIP_PATH:-"$DIST_DIR/$PACKAGE_NAME.zip"}"
PLUGIN_SOURCE="${PLUGIN_SOURCE:-"$BUILD_DIR/NateVST_artefacts/VST3/Nate VST.vst3"}"
PRESETS_SOURCE="${PRESETS_SOURCE:-"$ROOT_DIR/Resources/Factory Presets"}"

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

require_path() {
    local path="$1"
    if [[ ! -e "$path" ]]; then
        echo "Missing required path: $path" >&2
        exit 1
    fi
}

write_install_notes() {
    local path="$1"

    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: write install notes to $path"
        return 0
    fi

    cat > "$path" <<EOF
Nate VST $VERSION macOS package

Install:
1. Copy "Nate VST.vst3" to ~/Library/Audio/Plug-Ins/VST3/
2. Copy "Factory Presets" to ~/Library/Application Support/Nate VST/Factory Presets
3. Rescan VST3 plugins in Ableton Live, then load Nate VST on a MIDI track.

Release validation:
- Run the checklist in ABLETON_RELEASE_VALIDATION.md before tagging or sharing a release build.
- This package is not a signed/notarized installer yet.

Checksums:
- SHA256SUMS contains file checksums for the package folder contents.
EOF
}

write_checksums() {
    local package_dir="$1"

    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: write checksums in $package_dir/SHA256SUMS"
        return 0
    fi

    (
        cd "$package_dir"
        find . -type f ! -name SHA256SUMS -print0 \
            | sort -z \
            | xargs -0 shasum -a 256 > SHA256SUMS
    )
}

create_zip() {
    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: create zip $ZIP_PATH"
        return 0
    fi

    rm -f "$ZIP_PATH"
    if command -v zip >/dev/null 2>&1; then
        (cd "$DIST_DIR" && zip -qry -X "$ZIP_PATH" "$PACKAGE_NAME")
    elif command -v ditto >/dev/null 2>&1; then
        (cd "$DIST_DIR" && ditto -c -k --norsrc --keepParent "$PACKAGE_NAME" "$ZIP_PATH")
    else
        echo "Could not create zip: neither ditto nor zip is available" >&2
        exit 1
    fi
}

echo "== Nate VST release package =="
echo "Root: $ROOT_DIR"
echo "Build: $BUILD_DIR"
echo "Version: $VERSION"
echo "Package: $PACKAGE_DIR"
echo "Zip: $ZIP_PATH"
echo

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Build directory does not exist: $BUILD_DIR" >&2
    echo "Run: cmake -S \"$ROOT_DIR\" -B \"$BUILD_DIR\"" >&2
    exit 1
fi

if [[ "$RUN_VALIDATE" == "1" ]]; then
    echo "== Validate release before packaging =="
    BUILD_DIR="$BUILD_DIR" JOBS="$JOBS" SKIP_PLUGINVAL="$SKIP_PLUGINVAL" "$ROOT_DIR/tools/validate_release.sh"
else
    echo "== Build VST3 =="
    cmake --build "$BUILD_DIR" --target NateVST -j "$JOBS"
fi
echo

require_path "$PLUGIN_SOURCE"
require_path "$PRESETS_SOURCE"
require_path "$ROOT_DIR/README.md"
require_path "$ROOT_DIR/docs/ABLETON_RELEASE_VALIDATION.md"

echo "== Assemble package folder =="
run mkdir -p "$DIST_DIR"
run rm -rf "$PACKAGE_DIR"
run mkdir -p "$PACKAGE_DIR"
run cp -R "$PLUGIN_SOURCE" "$PACKAGE_DIR/Nate VST.vst3"
run cp -R "$PRESETS_SOURCE" "$PACKAGE_DIR/Factory Presets"
run cp "$ROOT_DIR/README.md" "$PACKAGE_DIR/README.md"
run cp "$ROOT_DIR/docs/ABLETON_RELEASE_VALIDATION.md" "$PACKAGE_DIR/ABLETON_RELEASE_VALIDATION.md"
write_install_notes "$PACKAGE_DIR/INSTALL.txt"

if [[ "$DRY_RUN" != "1" ]]; then
    find "$PACKAGE_DIR" -name .DS_Store -delete
fi

write_checksums "$PACKAGE_DIR"
create_zip
echo

echo "Release package completed:"
echo "$PACKAGE_DIR"
echo "$ZIP_PATH"
