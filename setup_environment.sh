#!/usr/bin/env bash

# ISLAND environment setup script
# Detects a local Unreal Engine install and exports UE_PATH in the active shell rc.

set -euo pipefail

echo "=== ISLAND Environment Setup ==="
echo ""
echo "Searching for Unreal Engine installation..."

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

if ! UE_INSTALL_DIR="$(find_ue_path)"; then
    echo "ERROR: Could not find a local Unreal Engine installation."
    echo "Set UE_PATH manually before building, for example:"
    echo '  export UE_PATH="/Users/Shared/Epic Games/UE_5.7"'
    exit 1
fi

echo "Using Unreal Engine at: $UE_INSTALL_DIR"
echo ""

SHELL_NAME="$(basename "${SHELL:-bash}")"
case "$SHELL_NAME" in
    zsh)
        SHELL_RC="$HOME/.zshrc"
        ;;
    bash)
        SHELL_RC="$HOME/.bashrc"
        ;;
    *)
        SHELL_RC="$HOME/.profile"
        ;;
esac

mkdir -p "$(dirname "$SHELL_RC")"
touch "$SHELL_RC"

if grep -q 'export UE_PATH=' "$SHELL_RC" 2>/dev/null; then
    echo "UE_PATH already exists in $SHELL_RC"
    echo "Current value: $(grep 'export UE_PATH=' "$SHELL_RC" | tail -n 1)"
    read -r -p "Do you want to update it? (y/n) " REPLY
    if [[ ! "$REPLY" =~ ^[Yy]$ ]]; then
        echo "Keeping existing configuration."
        exit 0
    fi

    if [[ "$(uname -s)" == "Darwin" ]]; then
        sed -i '' '/export UE_PATH=/d' "$SHELL_RC"
    else
        sed -i '/export UE_PATH=/d' "$SHELL_RC"
    fi
fi

{
    echo ""
    echo "# Unreal Engine path for ISLAND builds"
    echo "export UE_PATH=\"$UE_INSTALL_DIR\""
} >> "$SHELL_RC"

echo ""
echo "✓ Added UE_PATH to $SHELL_RC"
echo ""
echo "To apply the changes immediately, run:"
echo "  source \"$SHELL_RC\""
echo ""
echo "To verify, run:"
echo "  echo \$UE_PATH"
echo ""
echo "Setup complete! You can now build ISLAND from the terminal or VS Code."
