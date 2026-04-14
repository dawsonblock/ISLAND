# GitHub Actions Testing Guide

Quick reference for testing on GitHub via Git.

## 1. Initial Setup (One-Time)

```bash
# Install pre-commit framework
pip install pre-commit

# Install pre-commit hooks in your git config
pre-commit install

# Install custom git hooks
./scripts/setup-git-hooks.sh

# Run pre-commit on all files
pre-commit run --all-files
```

## 2. Local Testing Before Commit

```bash
# Automatic (pre-commit runs on commit)
git add .
git commit -m "message"

# Manual (run anytime)
pre-commit run --all-files

# Run specific hook
pre-commit run black --all-files
pre-commit run flake8 --all-files
```

## 3. Commit & Push

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes
echo "code" >> RFSN_NPC_AI/Python/module.py

# Commit (pre-commit hooks run)
git add .
git commit -m "Add feature"

# Pre-push hook runs tests (before pushing to main)
git push -u origin feature/my-feature
```

## 4. GitHub Actions Auto-Run

When you push:
- `.github/workflows/build-test.yml` runs automatically
  - Python tests (3.10, 3.11, 3.12)
  - Docker build
  - Code quality checks
  - Status visible in PR

View results:
```bash
gh run list
gh run watch
gh pr view --web  # Open PR in browser
```

## 5. Merge to Main (After Checks Pass)

```bash
# Create PR
gh pr create --title "My Feature"

# Wait for all checks ✓
# Then merge
gh pr merge <number>

# `.github/workflows/docker-push.yml` runs
# Docker image pushed to registries
```

## Test Checklist

- [ ] Local tests pass: `./scripts/ci-cd.sh all`
- [ ] Pre-commit hooks pass: `pre-commit run --all-files`
- [ ] Docker builds: `docker build -t test -f RFSN_NPC_AI/Dockerfile .`
- [ ] Push: `git push -u origin branch`
- [ ] GitHub Actions all green ✓
- [ ] Merge to main: `gh pr merge <number>`
- [ ] Docker image in registry: `docker pull ghcr.io/.../backend:main`

