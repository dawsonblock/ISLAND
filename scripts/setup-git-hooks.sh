#!/bin/bash
# Setup Git hooks for ISLAND project

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Setting up Git hooks..."

# Configure git to use custom hooks directory
git config core.hooksPath .githooks

# Make hooks executable
chmod +x .githooks/pre-push

echo "✓ Git hooks installed:"
echo "  - pre-push: Runs tests before pushing to main"
echo ""
echo "To install pre-commit framework:"
echo "  pip install pre-commit"
echo "  pre-commit install"
echo "  pre-commit run --all-files"
