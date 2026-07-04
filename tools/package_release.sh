#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
JOBS="${JOBS:-}"
VERSION="${VERSION:-}"
DIST_DIR="${DIST_DIR:-"$ROOT_DIR/dist"}"
RUN_VALIDATE="${RUN_VALIDATE:-1}"
SKIP_PLUGINVAL="${SKIP_PLUGINVAL:-0}"
PLUGINVAL_AUTO_DOWNLOAD="${PLUGINVAL_AUTO_DOWNLOAD:-0}"
PLUGINVAL_SKIP_GUI_TESTS="${PLUGINVAL_SKIP_GUI_TESTS:-0}"
CREATE_PKG="${CREATE_PKG:-0}"
CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY:-}"
INSTALLER_SIGN_IDENTITY="${INSTALLER_SIGN_IDENTITY:-}"
PKG_IDENTIFIER="${PKG_IDENTIFIER:-com.nate.natevst.installer}"
NOTARIZE="${NOTARIZE:-0}"
NOTARIZE_TIMEOUT="${NOTARIZE_TIMEOUT:-30m}"
NOTARY_PROFILE="${NOTARY_PROFILE:-}"
NOTARY_KEYCHAIN="${NOTARY_KEYCHAIN:-}"
NOTARY_APPLE_ID="${NOTARY_APPLE_ID:-}"
NOTARY_PASSWORD="${NOTARY_PASSWORD:-}"
NOTARY_TEAM_ID="${NOTARY_TEAM_ID:-}"
NOTARY_KEY="${NOTARY_KEY:-}"
NOTARY_KEY_ID="${NOTARY_KEY_ID:-}"
NOTARY_ISSUER="${NOTARY_ISSUER:-}"
DRY_RUN="${DRY_RUN:-0}"
export COPYFILE_DISABLE="${COPYFILE_DISABLE:-1}"

if [[ -z "$VERSION" ]]; then
    VERSION="$(sed -nE 's/^project\(NateVST VERSION ([^ ]+) .*/\1/p' "$ROOT_DIR/CMakeLists.txt" | head -n 1)"
    VERSION="${VERSION:-dev}"
fi

PACKAGE_NAME="${PACKAGE_NAME:-"NateVST-$VERSION-macos"}"
PACKAGE_DIR="${PACKAGE_DIR:-"$DIST_DIR/$PACKAGE_NAME"}"
ZIP_PATH="${ZIP_PATH:-"$DIST_DIR/$PACKAGE_NAME.zip"}"
PKG_ROOT="${PKG_ROOT:-"$DIST_DIR/$PACKAGE_NAME-pkgroot"}"
PKG_UNSIGNED_PATH="${PKG_UNSIGNED_PATH:-"$DIST_DIR/$PACKAGE_NAME-unsigned.pkg"}"
PKG_PATH="${PKG_PATH:-"$DIST_DIR/$PACKAGE_NAME.pkg"}"
NOTARY_LOG_PATH="${NOTARY_LOG_PATH:-"$DIST_DIR/$PACKAGE_NAME-notarytool.txt"}"
PACKAGE_MANIFEST_PATH="${PACKAGE_MANIFEST_PATH:-"$PACKAGE_DIR/RELEASE_MANIFEST.txt"}"
PACKAGE_SUMMARY_PATH="${PACKAGE_SUMMARY_PATH:-"$DIST_DIR/$PACKAGE_NAME-release-summary.txt"}"
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

copy_path() {
    local source="$1"
    local dest="$2"
    local parent
    parent="$(dirname "$dest")"

    if [[ "$DRY_RUN" != "1" ]]; then
        require_path "$source"
    fi
    run mkdir -p "$parent"
    run rm -rf "$dest"
    if command -v ditto >/dev/null 2>&1; then
        run ditto --norsrc --noextattr --noacl --noqtn "$source" "$dest"
    else
        run env COPYFILE_DISABLE=1 cp -R "$source" "$dest"
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
    local signing_note="This package folder was not Developer ID signed by this script."
    local pkg_note="A macOS .pkg installer was not created for this package."
    local notarization_note="Notarization was not requested."

    if [[ -n "$CODE_SIGN_IDENTITY" ]]; then
        signing_note="The copied VST3 bundle was signed with: $CODE_SIGN_IDENTITY"
    fi

    if [[ "$CREATE_PKG" == "1" ]]; then
        pkg_note="A macOS .pkg installer can be found next to this folder: $(basename "$PKG_PATH")"
    fi

    if [[ "$NOTARIZE" == "1" ]]; then
        notarization_note="The .pkg installer was submitted with notarytool and stapled after approval."
    fi

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
- $signing_note
- $pkg_note
- $notarization_note

Checksums:
- SHA256SUMS contains file checksums for the package folder contents.
EOF
}

sign_plugin_bundle() {
    local plugin_path="$1"

    if [[ -z "$CODE_SIGN_IDENTITY" ]]; then
        return 0
    fi

    echo "== Sign VST3 bundle =="
    run codesign --force --deep --timestamp --options runtime --sign "$CODE_SIGN_IDENTITY" "$plugin_path"
    run codesign --verify --deep --strict --verbose=2 "$plugin_path"
    echo
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

file_checksum() {
    local path="$1"
    if [[ "$DRY_RUN" == "1" ]]; then
        printf 'dry-run'
        return 0
    fi

    if [[ -e "$path" ]]; then
        shasum -a 256 "$path" | awk '{ print $1 }'
    else
        printf 'missing'
    fi
}

write_package_manifest() {
    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: write package manifest to $PACKAGE_MANIFEST_PATH"
        return 0
    fi

    local git_revision
    local validation_summary
    local notary_api_key_note="not set"
    local notary_apple_id_note="not set"
    git_revision="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || printf 'unknown')"
    validation_summary="$BUILD_DIR/release-validation/summary.txt"
    if [[ -n "$NOTARY_KEY_ID" ]]; then
        notary_api_key_note="configured"
    fi
    if [[ -n "$NOTARY_APPLE_ID" ]]; then
        notary_apple_id_note="configured"
    fi

    cat > "$PACKAGE_MANIFEST_PATH" <<EOF
Nate VST release manifest

Version: $VERSION
Git revision: $git_revision
Created UTC: $(date -u '+%Y-%m-%dT%H:%M:%SZ')

Package folder: $PACKAGE_DIR
Zip artifact: $ZIP_PATH
Installer pkg requested: $CREATE_PKG
Installer pkg path: $PKG_PATH

Validation:
- RUN_VALIDATE: $RUN_VALIDATE
- Validation summary: $validation_summary
- SKIP_PLUGINVAL: $SKIP_PLUGINVAL
- PLUGINVAL_AUTO_DOWNLOAD: $PLUGINVAL_AUTO_DOWNLOAD
- PLUGINVAL_SKIP_GUI_TESTS: $PLUGINVAL_SKIP_GUI_TESTS

Signing:
- VST3 code signing identity: ${CODE_SIGN_IDENTITY:-unsigned}
- Installer signing identity: ${INSTALLER_SIGN_IDENTITY:-unsigned}

Notarization:
- NOTARIZE: $NOTARIZE
- Notary log: $NOTARY_LOG_PATH
- Notary profile: ${NOTARY_PROFILE:-not set}
- Notary API key: $notary_api_key_note
- Notary Apple ID: $notary_apple_id_note

Included payload:
- Nate VST.vst3
- Factory Presets
- README.md
- ABLETON_RELEASE_VALIDATION.md
- INSTALL.txt
- SHA256SUMS

Release checklist:
- Attach this manifest, SHA256SUMS, build/release-validation/summary.txt, ctest-output.txt, and the pluginval report to the release issue.
- Complete the Ableton matrix in ABLETON_RELEASE_VALIDATION.md against this exact build before tagging.
EOF
}

write_package_summary() {
    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: write package summary to $PACKAGE_SUMMARY_PATH"
        return 0
    fi

    local pkg_status="not requested"
    local pkg_checksum="not requested"
    local notary_status="not requested"

    if [[ "$CREATE_PKG" == "1" ]]; then
        pkg_status="$PKG_PATH"
        pkg_checksum="$(file_checksum "$PKG_PATH")"
    fi

    if [[ "$NOTARIZE" == "1" ]]; then
        notary_status="submitted and stapled; log: $NOTARY_LOG_PATH"
    fi

    cat > "$PACKAGE_SUMMARY_PATH" <<EOF
Nate VST package summary

Version: $VERSION
Created UTC: $(date -u '+%Y-%m-%dT%H:%M:%SZ')
Package folder: $PACKAGE_DIR
Package manifest: $PACKAGE_MANIFEST_PATH
Zip artifact: $ZIP_PATH
Zip SHA256: $(file_checksum "$ZIP_PATH")
Installer pkg: $pkg_status
Installer pkg SHA256: $pkg_checksum
VST3 signing identity: ${CODE_SIGN_IDENTITY:-unsigned}
Installer signing identity: ${INSTALLER_SIGN_IDENTITY:-unsigned}
Notarization: $notary_status
Validation summary: $BUILD_DIR/release-validation/summary.txt
Package checksums: $PACKAGE_DIR/SHA256SUMS
Ableton checklist: $PACKAGE_DIR/ABLETON_RELEASE_VALIDATION.md
EOF
}

clean_macos_metadata() {
    local path="$1"

    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: remove .DS_Store, AppleDouble files, and xattrs from $path"
        return 0
    fi

    if [[ -d "$path" ]]; then
        find "$path" \( -name .DS_Store -o -name '._*' \) -delete
        if command -v xattr >/dev/null 2>&1; then
            xattr -cr "$path" 2>/dev/null || true
        fi
    fi
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

normalize_pkg_payload() {
    local pkg_path="$1"

    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: normalize pkg payload metadata in $pkg_path"
        return 0
    fi

    require_path "$pkg_path"

    local temp_dir
    temp_dir="$(mktemp -d)"
    local expanded_full="$temp_dir/full"
    local expanded_raw="$temp_dir/raw"
    local normalized_pkg="$temp_dir/normalized.pkg"

    pkgutil --expand-full "$pkg_path" "$expanded_full"
    pkgutil --expand "$pkg_path" "$expanded_raw"

    clean_macos_metadata "$expanded_full/Payload"
    mkbom "$expanded_full/Payload" "$expanded_raw/Bom"

    (
        cd "$expanded_full/Payload"
        COPYFILE_DISABLE=1 find . -print \
            | LC_ALL=C sort \
            | cpio -o --format odc 2>"$temp_dir/cpio.log" \
            | gzip -c > "$expanded_raw/Payload"
    )

    pkgutil --flatten "$expanded_raw" "$normalized_pkg"
    mv "$normalized_pkg" "$pkg_path"
    rm -rf "$temp_dir"
}

build_notarytool_args() {
    NOTARYTOOL_ARGS=()

    if [[ -n "$NOTARY_PROFILE" ]]; then
        NOTARYTOOL_ARGS=(--keychain-profile "$NOTARY_PROFILE")
        if [[ -n "$NOTARY_KEYCHAIN" ]]; then
            NOTARYTOOL_ARGS+=(--keychain "$NOTARY_KEYCHAIN")
        fi
        return 0
    fi

    if [[ -n "$NOTARY_KEY" || -n "$NOTARY_KEY_ID" || -n "$NOTARY_ISSUER" ]]; then
        if [[ -z "$NOTARY_KEY" || -z "$NOTARY_KEY_ID" || -z "$NOTARY_ISSUER" ]]; then
            echo "NOTARY_KEY, NOTARY_KEY_ID, and NOTARY_ISSUER must all be set for API-key notarization." >&2
            return 1
        fi

        NOTARYTOOL_ARGS=(--key "$NOTARY_KEY" --key-id "$NOTARY_KEY_ID" --issuer "$NOTARY_ISSUER")
        return 0
    fi

    if [[ -n "$NOTARY_APPLE_ID" || -n "$NOTARY_PASSWORD" || -n "$NOTARY_TEAM_ID" ]]; then
        if [[ -z "$NOTARY_APPLE_ID" || -z "$NOTARY_PASSWORD" || -z "$NOTARY_TEAM_ID" ]]; then
            echo "NOTARY_APPLE_ID, NOTARY_PASSWORD, and NOTARY_TEAM_ID must all be set for Apple ID notarization." >&2
            return 1
        fi

        NOTARYTOOL_ARGS=(--apple-id "$NOTARY_APPLE_ID" --password "$NOTARY_PASSWORD" --team-id "$NOTARY_TEAM_ID")
        return 0
    fi

    echo "Set NOTARY_PROFILE, NOTARY_KEY/NOTARY_KEY_ID/NOTARY_ISSUER, or NOTARY_APPLE_ID/NOTARY_PASSWORD/NOTARY_TEAM_ID." >&2
    return 1
}

create_pkg_installer() {
    if [[ "$CREATE_PKG" != "1" ]]; then
        return 0
    fi

    echo "== Assemble macOS installer root =="
    run rm -rf "$PKG_ROOT"
    copy_path "$PACKAGE_DIR/Nate VST.vst3" "$PKG_ROOT/Library/Audio/Plug-Ins/VST3/Nate VST.vst3"
    copy_path "$PACKAGE_DIR/Factory Presets" "$PKG_ROOT/Library/Application Support/Nate VST/Factory Presets"
    clean_macos_metadata "$PKG_ROOT"
    echo

    echo "== Build macOS pkg =="
    run rm -f "$PKG_UNSIGNED_PATH" "$PKG_PATH"
    local pkgbuild_output="$PKG_PATH"
    if [[ -n "$INSTALLER_SIGN_IDENTITY" ]]; then
        pkgbuild_output="$PKG_UNSIGNED_PATH"
    fi

    run pkgbuild \
        --root "$PKG_ROOT" \
        --identifier "$PKG_IDENTIFIER" \
        --version "$VERSION" \
        --install-location "/" \
        --filter '(^|/)\.DS_Store$' \
        --filter '(^|/)\._.*' \
        --filter '(^|/)\.svn($|/)' \
        --filter '(^|/)CVS($|/)' \
        "$pkgbuild_output"

    normalize_pkg_payload "$pkgbuild_output"

    if [[ -n "$INSTALLER_SIGN_IDENTITY" ]]; then
        run productsign --sign "$INSTALLER_SIGN_IDENTITY" "$PKG_UNSIGNED_PATH" "$PKG_PATH"
        run rm -f "$PKG_UNSIGNED_PATH"
        run pkgutil --check-signature "$PKG_PATH"
    fi

    if [[ "$DRY_RUN" != "1" ]] && pkgutil --payload-files "$PKG_PATH" | grep -E '(^|/)\._|(^|/)\.DS_Store$' >/dev/null; then
        echo "Package payload contains macOS metadata sidecar entries after normalization: $PKG_PATH" >&2
        exit 1
    fi
    echo
}

notarize_pkg_installer() {
    if [[ "$NOTARIZE" != "1" ]]; then
        return 0
    fi

    if [[ "$CREATE_PKG" != "1" ]]; then
        echo "NOTARIZE=1 requires CREATE_PKG=1." >&2
        exit 1
    fi

    if [[ -z "$CODE_SIGN_IDENTITY" || -z "$INSTALLER_SIGN_IDENTITY" ]]; then
        echo "NOTARIZE=1 requires CODE_SIGN_IDENTITY and INSTALLER_SIGN_IDENTITY." >&2
        exit 1
    fi

    if [[ "$DRY_RUN" != "1" ]]; then
        require_path "$PKG_PATH"
    fi

    if ! build_notarytool_args; then
        exit 1
    fi

    echo "== Notarize macOS pkg =="
    if [[ "$DRY_RUN" == "1" ]]; then
        echo "DRY RUN: xcrun notarytool submit $PKG_PATH --wait --timeout $NOTARIZE_TIMEOUT [credentials redacted]"
    else
        xcrun notarytool submit "$PKG_PATH" \
            --wait \
            --timeout "$NOTARIZE_TIMEOUT" \
            "${NOTARYTOOL_ARGS[@]}" 2>&1 | tee "$NOTARY_LOG_PATH"
    fi
    run xcrun stapler staple "$PKG_PATH"
    run xcrun stapler validate "$PKG_PATH"
    run spctl -a -t install -vv "$PKG_PATH"
    echo
}

echo "== Nate VST release package =="
echo "Root: $ROOT_DIR"
echo "Build: $BUILD_DIR"
echo "Version: $VERSION"
echo "Package: $PACKAGE_DIR"
echo "Zip: $ZIP_PATH"
if [[ "$CREATE_PKG" == "1" ]]; then
    echo "Pkg: $PKG_PATH"
fi
echo

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Build directory does not exist: $BUILD_DIR" >&2
    echo "Run: cmake -S \"$ROOT_DIR\" -B \"$BUILD_DIR\"" >&2
    exit 1
fi

if [[ "$RUN_VALIDATE" == "1" ]]; then
    echo "== Validate release before packaging =="
    BUILD_DIR="$BUILD_DIR" \
        JOBS="$JOBS" \
        SKIP_PLUGINVAL="$SKIP_PLUGINVAL" \
        PLUGINVAL_AUTO_DOWNLOAD="$PLUGINVAL_AUTO_DOWNLOAD" \
        PLUGINVAL_SKIP_GUI_TESTS="$PLUGINVAL_SKIP_GUI_TESTS" \
        "$ROOT_DIR/tools/validate_release.sh"
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
copy_path "$PLUGIN_SOURCE" "$PACKAGE_DIR/Nate VST.vst3"
copy_path "$PRESETS_SOURCE" "$PACKAGE_DIR/Factory Presets"
run cp "$ROOT_DIR/README.md" "$PACKAGE_DIR/README.md"
run cp "$ROOT_DIR/docs/ABLETON_RELEASE_VALIDATION.md" "$PACKAGE_DIR/ABLETON_RELEASE_VALIDATION.md"
sign_plugin_bundle "$PACKAGE_DIR/Nate VST.vst3"
write_install_notes "$PACKAGE_DIR/INSTALL.txt"
write_package_manifest

clean_macos_metadata "$PACKAGE_DIR"

write_checksums "$PACKAGE_DIR"
create_zip
create_pkg_installer
notarize_pkg_installer
write_package_summary
echo

echo "Release package completed:"
echo "$PACKAGE_DIR"
echo "$ZIP_PATH"
echo "$PACKAGE_SUMMARY_PATH"
if [[ "$CREATE_PKG" == "1" ]]; then
    echo "$PKG_PATH"
fi
