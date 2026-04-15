#!/usr/bin/env bash
# ISLAND Game Launcher - build the editor target and open the project

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_FILE="$SCRIPT_DIR/MyProject.uproject"

find_ue_path() {
    local candidate=""
    local search_roots=(
        "/Users/Shared/Epic Games"
        "/opt/unreal-engine"
        "$HOME/Epic Games"
    )

    for root in "${search_roots[@]}"; do
        if [[ -d "$root" ]]; then
            while IFS= read -r path; do
                candidate="$path"
            done < <(ls -d "$root"/UE_* 2>/dev/null | sort -V)
            if [[ -n "$candidate" ]]; then
                echo "$candidate"
                return 0
            fi
        fi
    done

    return 1
}

configure_macos_toolchain() {
    local preferred_xcode="/Applications/Xcode.app/Contents/Developer"

    if [[ -z "${DEVELOPER_DIR:-}" && -d "$preferred_xcode" ]]; then
        export DEVELOPER_DIR="$preferred_xcode"
    fi

    if ! xcodebuild -version >/dev/null 2>&1; then
        echo "❌ Full Xcode is required to build the Mac editor target."
        echo "   Install Xcode and ensure /Applications/Xcode.app is available."
        echo "   You can also export DEVELOPER_DIR to a valid Xcode Developer directory."
        exit 1
    fi
}

UE_PATH="${UE_PATH:-}"
if [[ -z "$UE_PATH" ]]; then
    if ! UE_PATH="$(find_ue_path)"; then
        echo "❌ Could not find Unreal Engine. Set UE_PATH before running this script."
        exit 1
    fi
fi

OS_NAME="$(uname -s)"
TARGET_PLATFORM=""
BUILD_SCRIPT=""
EDITOR_BIN=""

case "$OS_NAME" in
    Darwin)
        configure_macos_toolchain
        TARGET_PLATFORM="Mac"
        BUILD_SCRIPT="$UE_PATH/Engine/Build/BatchFiles/Mac/Build.sh"
        EDITOR_BIN="$UE_PATH/Engine/Binaries/Mac/UnrealEditor.app"
        ;;
    Linux)
        TARGET_PLATFORM="Linux"
        BUILD_SCRIPT="$UE_PATH/Engine/Build/BatchFiles/Linux/Build.sh"
        EDITOR_BIN="$UE_PATH/Engine/Binaries/Linux/UnrealEditor"
        ;;
    *)
        echo "❌ Unsupported platform: $OS_NAME"
        exit 1
        ;;
esac

if [[ ! -x "$BUILD_SCRIPT" ]]; then
    echo "❌ Unreal build script not found at: $BUILD_SCRIPT"
    echo "   UE_PATH is currently: $UE_PATH"
    exit 1
fi

echo "🏝️  ISLAND Game Launcher"
echo "========================"
echo "Project: $PROJECT_FILE"
echo "Engine:  $UE_PATH"
if [[ -n "${DEVELOPER_DIR:-}" ]]; then
    echo "Xcode:   $DEVELOPER_DIR"
fi
echo ""
echo "🔨 Compiling C++ code..."

"$BUILD_SCRIPT" MyProjectEditor "$TARGET_PLATFORM" Development \
    -project="$PROJECT_FILE" \
    -quiet

echo "✅ Build succeeded!"

if [[ "${SKIP_LAUNCH:-0}" == "1" ]]; then
    echo "ℹ️  SKIP_LAUNCH=1 set, not opening the editor."
    exit 0
fi

echo ""
echo "🚀 Launching Unreal Editor..."

if [[ "$OS_NAME" == "Darwin" ]]; then
    open "$PROJECT_FILE"
else
    if [[ ! -x "$EDITOR_BIN" ]]; then
        echo "❌ Unreal Editor binary not found at: $EDITOR_BIN"
        exit 1
    fi
    "$EDITOR_BIN" "$PROJECT_FILE" >/dev/null 2>&1 &
fi

echo ""
echo "✅ Done! The editor should be opening now."
