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
PLUGINVAL_SKIP_GUI_TESTS="${PLUGINVAL_SKIP_GUI_TESTS:-0}"
PLUGINVAL_AUTO_DOWNLOAD="${PLUGINVAL_AUTO_DOWNLOAD:-0}"
PLUGINVAL_VST3_VALIDATOR="${PLUGINVAL_VST3_VALIDATOR:-}"
RELEASE_VALIDATION_DIR="${RELEASE_VALIDATION_DIR:-"$BUILD_DIR/release-validation"}"
RELEASE_VALIDATION_REPORT="${RELEASE_VALIDATION_REPORT:-"$RELEASE_VALIDATION_DIR/summary.txt"}"
BUILD_OUTPUT_FILE="${BUILD_OUTPUT_FILE:-"$RELEASE_VALIDATION_DIR/build-output.txt"}"
CTEST_OUTPUT_FILE="${CTEST_OUTPUT_FILE:-"$RELEASE_VALIDATION_DIR/ctest-output.txt"}"
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

resolve_vst3_validator() {
    if [[ -n "$PLUGINVAL_VST3_VALIDATOR" ]]; then
        if [[ -x "$PLUGINVAL_VST3_VALIDATOR" ]]; then
            printf '%s\n' "$PLUGINVAL_VST3_VALIDATOR"
            return 0
        fi

        echo "PLUGINVAL_VST3_VALIDATOR is set but is not executable: $PLUGINVAL_VST3_VALIDATOR" >&2
        return 2
    fi

    for candidate in \
        "$BUILD_DIR/validator" \
        "$BUILD_DIR/vst3validator" \
        "${VST3_SDK_DIR:-}/public.sdk/samples/vst-hosting/validator/build/validator" \
        "${VST3_SDK_DIR:-}/build/public.sdk/samples/vst-hosting/validator/validator" \
        "${VST3_SDK_DIR:-}/public.sdk/samples/vst-hosting/validator/Builds/MacOSX/build/Release/validator"
    do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
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

mkdir -p "$RELEASE_VALIDATION_DIR"
VERSION="$(sed -nE 's/^project\(NateVST VERSION ([^ ]+) .*/\1/p' "$ROOT_DIR/CMakeLists.txt" | head -n 1)"
VERSION="${VERSION:-unknown}"
GIT_REVISION="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || printf 'unknown')"
GIT_STATUS="$(git -C "$ROOT_DIR" status --short 2>/dev/null || true)"
VALIDATION_STARTED_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
CURRENT_STAGE="setup"

cat > "$RELEASE_VALIDATION_REPORT" <<EOF
# Nate VST Release Validation

Started UTC: $VALIDATION_STARTED_UTC
Version: $VERSION
Git revision: $GIT_REVISION
Build directory: $BUILD_DIR
Plugin path: $PLUGIN_PATH
Pluginval binary: $PLUGINVAL_BIN
Pluginval strictness: $PLUGINVAL_STRICTNESS
Pluginval timeout ms: $PLUGINVAL_TIMEOUT_MS
Pluginval skip GUI tests: $PLUGINVAL_SKIP_GUI_TESTS
Pluginval auto-download: $PLUGINVAL_AUTO_DOWNLOAD
Pluginval VST3 validator: ${PLUGINVAL_VST3_VALIDATOR:-auto-detect}
SKIP_PLUGINVAL: $SKIP_PLUGINVAL

EOF

if [[ -n "$GIT_STATUS" ]]; then
    {
        echo "Git status:"
        echo '```'
        printf '%s\n' "$GIT_STATUS"
        echo '```'
        echo
    } >> "$RELEASE_VALIDATION_REPORT"
else
    echo "Git status: clean" >> "$RELEASE_VALIDATION_REPORT"
    echo >> "$RELEASE_VALIDATION_REPORT"
fi

record_failure() {
    local exit_code=$?
    if [[ "$exit_code" -ne 0 ]]; then
        {
            echo "$CURRENT_STAGE: FAIL"
            echo
            echo "Failed UTC: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
            echo "Exit code: $exit_code"
        } >> "$RELEASE_VALIDATION_REPORT"
        echo "Release validation failed during $CURRENT_STAGE. Summary: $RELEASE_VALIDATION_REPORT" >&2
    fi
}
trap record_failure EXIT

echo "== Build VST3 and audit executables =="
CURRENT_STAGE="build"
cmake --build "$BUILD_DIR" -j "$JOBS" 2>&1 | tee "$BUILD_OUTPUT_FILE"
echo "Build: PASS" >> "$RELEASE_VALIDATION_REPORT"
echo "Build log: $BUILD_OUTPUT_FILE" >> "$RELEASE_VALIDATION_REPORT"
echo >> "$RELEASE_VALIDATION_REPORT"
echo

if [[ ! -d "$PLUGIN_PATH" && ! -f "$PLUGIN_PATH" ]]; then
    CURRENT_STAGE="plugin artifact"
    echo "Built plugin was not found at: $PLUGIN_PATH" >&2
    exit 1
fi

echo "== CTest suite =="
CURRENT_STAGE="ctest"
ctest --test-dir "$BUILD_DIR" --output-on-failure 2>&1 | tee "$CTEST_OUTPUT_FILE"
echo "CTest: PASS" >> "$RELEASE_VALIDATION_REPORT"
echo "CTest log: $CTEST_OUTPUT_FILE" >> "$RELEASE_VALIDATION_REPORT"
echo >> "$RELEASE_VALIDATION_REPORT"
echo

if [[ "$SKIP_PLUGINVAL" == "1" ]]; then
    echo "== pluginval =="
    echo "Skipped because SKIP_PLUGINVAL=1"
    echo "pluginval: SKIPPED" >> "$RELEASE_VALIDATION_REPORT"
    echo >> "$RELEASE_VALIDATION_REPORT"
    echo
else
    CURRENT_STAGE="pluginval discovery"
    if ! PLUGINVAL_RESOLVED="$(resolve_pluginval)"; then
        if [[ "$PLUGINVAL_AUTO_DOWNLOAD" == "1" ]]; then
            PLUGINVAL_RESOLVED="$("$ROOT_DIR/tools/fetch_pluginval.sh")"
        else
            echo "pluginval is required for release validation but was not found: $PLUGINVAL_BIN" >&2
            echo "Install pluginval, set PLUGINVAL_BIN=/absolute/path/to/pluginval, or set PLUGINVAL_AUTO_DOWNLOAD=1." >&2
            echo "For local smoke runs only, set SKIP_PLUGINVAL=1." >&2
            exit 1
        fi
    fi

    mkdir -p "$PLUGINVAL_OUTPUT_DIR"
    rm -f "$PLUGINVAL_OUTPUT_DIR/$PLUGINVAL_OUTPUT_FILE"
    echo "== pluginval =="
    CURRENT_STAGE="pluginval"
    pluginval_args=(
        --strictness-level "$PLUGINVAL_STRICTNESS"
        --timeout-ms "$PLUGINVAL_TIMEOUT_MS"
        --output-dir "$PLUGINVAL_OUTPUT_DIR"
        --output-filename "$PLUGINVAL_OUTPUT_FILE"
    )

    if [[ "$PLUGINVAL_SKIP_GUI_TESTS" == "1" ]]; then
        pluginval_args+=(--skip-gui-tests)
    fi

    CURRENT_STAGE="VST3 validator discovery"
    vst3_validator_status="SKIPPED"
    if VST3_VALIDATOR_RESOLVED="$(resolve_vst3_validator)"; then
        pluginval_args+=(--vst3validator "$VST3_VALIDATOR_RESOLVED")
        vst3_validator_status="ENABLED"
    elif [[ "$?" -eq 2 ]]; then
        exit 1
    fi

    CURRENT_STAGE="pluginval"
    "$PLUGINVAL_RESOLVED" "${pluginval_args[@]}" --validate "$PLUGIN_PATH"
    echo "pluginval: PASS" >> "$RELEASE_VALIDATION_REPORT"
    echo "pluginval binary: $PLUGINVAL_RESOLVED" >> "$RELEASE_VALIDATION_REPORT"
    echo "pluginval skip GUI tests: $PLUGINVAL_SKIP_GUI_TESTS" >> "$RELEASE_VALIDATION_REPORT"
    if [[ "$vst3_validator_status" == "ENABLED" ]]; then
        echo "pluginval VST3 validator: $VST3_VALIDATOR_RESOLVED" >> "$RELEASE_VALIDATION_REPORT"
    else
        echo "pluginval VST3 validator: SKIPPED (set PLUGINVAL_VST3_VALIDATOR=/path/to/validator to enable Steinberg VST3 validator coverage)" >> "$RELEASE_VALIDATION_REPORT"
    fi
    echo "pluginval log: $PLUGINVAL_OUTPUT_DIR/$PLUGINVAL_OUTPUT_FILE" >> "$RELEASE_VALIDATION_REPORT"
    echo >> "$RELEASE_VALIDATION_REPORT"
    echo
fi

echo "== Ableton manual validation =="
echo "Complete the checklist in docs/ABLETON_RELEASE_VALIDATION.md before tagging a release."
{
    echo "Ableton manual validation: REQUIRED"
    echo "Checklist: docs/ABLETON_RELEASE_VALIDATION.md"
    echo
    echo "Completed UTC: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
} >> "$RELEASE_VALIDATION_REPORT"
echo
echo "Release validation completed."
echo "Summary: $RELEASE_VALIDATION_REPORT"
CURRENT_STAGE="complete"
trap - EXIT
