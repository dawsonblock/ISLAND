Just remembered about it too#!/bin/bash

# Island Survival System - Environment Setup Script
# Run this script to set up your environment for building Unreal Engine projects from VS Code

echo "=== Island Survival System - Environment Setup ==="
echo ""

# Detect Unreal Engine installation
echo "Searching for Unreal Engine installation..."
UE_INSTALL_DIR=""

if [ -d "/Users/Shared/Epic Games" ]; then
    # Find the most recent UE installation
    for dir in "/Users/Shared/Epic Games"/UE_*; do
        if [ -d "$dir" ]; then
            UE_INSTALL_DIR="$dir"
            echo "Found: $UE_INSTALL_DIR"
        fi
    done
fi

if [ -z "$UE_INSTALL_DIR" ]; then
    echo "ERROR: Could not find Unreal Engine installation in /Users/Shared/Epic Games/"
    echo "Please install Unreal Engine or manually set UE_PATH"
    exit 1
fi

echo ""
echo "Using Unreal Engine at: $UE_INSTALL_DIR"
echo ""

# Check if already in shell config
SHELL_RC="$HOME/.zshrc"
if grep -q "export UE_PATH=" "$SHELL_RC" 2>/dev/null; then
    echo "UE_PATH already exists in $SHELL_RC"
    echo "Current value: $(grep 'export UE_PATH=' $SHELL_RC)"
    read -p "Do you want to update it? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Keeping existing configuration."
        exit 0
    fi
    # Remove old line
    sed -i '' '/export UE_PATH=/d' "$SHELL_RC"
fi

# Add to shell config
echo "" >> "$SHELL_RC"
echo "# Unreal Engine path for VS Code builds" >> "$SHELL_RC"
echo "export UE_PATH=\"$UE_INSTALL_DIR\"" >> "$SHELL_RC"

echo ""
echo "✓ Added UE_PATH to $SHELL_RC"
echo ""
echo "To apply the changes immediately, run:"
echo "  source ~/.zshrc"
echo ""
echo "Or restart your terminal and VS Code."
echo ""
echo "To verify, run:"
echo "  echo \$UE_PATH"
echo ""
echo "Setup complete! You can now build Unreal projects from VS Code."
