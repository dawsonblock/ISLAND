#!/bin/bash
# ISLAND Game Launcher - Auto-compiles and launches the editor

set -e

PROJECT_DIR="/Users/dawsonblock/Desktop/ISLAND"
UE_PATH="/Users/Shared/Epic Games/UE_5.7"
PROJECT_FILE="$PROJECT_DIR/MyProject.uproject"

echo "🏝️  ISLAND Game Launcher"
echo "========================"

# Step 1: Build the project
echo ""
echo "🔨 Compiling C++ code..."
"$UE_PATH/Engine/Build/BatchFiles/Mac/Build.sh" \
    MyProjectEditor Mac Development \
    -project="$PROJECT_FILE" \
    -quiet

if [ $? -eq 0 ]; then
    echo "✅ Build succeeded!"
else
    echo "❌ Build failed!"
    exit 1
fi

# Step 2: Launch the editor
echo ""
echo "🚀 Launching Unreal Editor..."
open "$PROJECT_FILE"

echo ""
echo "✅ Done! The editor should be opening now."
