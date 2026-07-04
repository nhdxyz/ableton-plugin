#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGINVAL_DOWNLOAD_DIR="${PLUGINVAL_DOWNLOAD_DIR:-"$ROOT_DIR/build/pluginval-bin"}"
PLUGINVAL_DOWNLOAD_URL="${PLUGINVAL_DOWNLOAD_URL:-}"
PLUGINVAL_ZIP_PATH="${PLUGINVAL_ZIP_PATH:-"$PLUGINVAL_DOWNLOAD_DIR/pluginval.zip"}"

detect_download_url() {
    if [[ -n "$PLUGINVAL_DOWNLOAD_URL" ]]; then
        printf '%s\n' "$PLUGINVAL_DOWNLOAD_URL"
        return 0
    fi

    case "$(uname -s)" in
        Darwin)
            printf '%s\n' "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip"
            ;;
        Linux)
            printf '%s\n' "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Linux.zip"
            ;;
        *)
            echo "No default pluginval binary URL for $(uname -s). Set PLUGINVAL_DOWNLOAD_URL." >&2
            return 1
            ;;
    esac
}

find_pluginval_binary() {
    for candidate in \
        "$PLUGINVAL_DOWNLOAD_DIR/pluginval.app/Contents/MacOS/pluginval" \
        "$PLUGINVAL_DOWNLOAD_DIR/pluginval"
    do
        if [[ -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    find "$PLUGINVAL_DOWNLOAD_DIR" -type f -name pluginval -perm -111 -print -quit 2>/dev/null
}

if resolved="$(find_pluginval_binary)" && [[ -n "$resolved" ]]; then
    printf '%s\n' "$resolved"
    exit 0
fi

download_url="$(detect_download_url)"
mkdir -p "$PLUGINVAL_DOWNLOAD_DIR"

echo "Downloading pluginval from $download_url" >&2
curl --fail --location --show-error "$download_url" --output "$PLUGINVAL_ZIP_PATH"

echo "Unpacking pluginval into $PLUGINVAL_DOWNLOAD_DIR" >&2
unzip -q -o "$PLUGINVAL_ZIP_PATH" -d "$PLUGINVAL_DOWNLOAD_DIR"

if [[ "$(uname -s)" == "Darwin" ]] && command -v xattr >/dev/null 2>&1; then
    xattr -dr com.apple.quarantine "$PLUGINVAL_DOWNLOAD_DIR" 2>/dev/null || true
fi

if ! resolved="$(find_pluginval_binary)" || [[ -z "$resolved" ]]; then
    echo "Could not find pluginval executable after unpacking $PLUGINVAL_ZIP_PATH" >&2
    exit 1
fi

chmod +x "$resolved"
printf '%s\n' "$resolved"
