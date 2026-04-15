# Git Testing Guide for ISLAND

## Overview

This guide covers testing the ISLAND project locally before pushing to GitHub, validating CI/CD pipelines, and setting up pre-commit hooks.

---

## 1. Local Testing Before Push

### Quick Test (All-in-One)

```bash
# From repo root
./scripts/ci-cd.sh all
```

This runs:

1. Python backend tests (142 tests)
2. Docker build
3. Start services with docker-compose
4. Health check

**Expected time**: 10-15 minutes (first run), 5-8 minutes (cached)

### Individual Tests

#### Python Backend Tests Only

```bash
cd RFSN_NPC_AI/Python

# Install test dependencies
python -m venv .venv
source .venv/bin/activate  # or .venv\Scripts\activate on Windows
pip install -r requirements-core.txt
pip install pytest pytest-cov pytest-asyncio

# Run all tests
pytest tests/ -v

# Run with coverage
pytest tests/ -v --cov=. --cov-report=html
open htmlcov/index.html

# Run specific test file
pytest tests/test_bandit_learner.py -v

# Run specific test
pytest tests/test_bandit_learner.py::TestStateActionBandit::test_persistence_save -v

# Run fast tests only (skip slow ones)
pytest tests/ -v -m "not slow"
```

**Expected result**: 142 passed

#### Docker Build Only

```bash
# From repo root
docker build -t island-backend:test -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Check image
docker images | grep island-backend

# Inspect layers
docker history island-backend:test
```

**Expected time**: 15-30 minutes first run, ~2 minutes cached

#### Docker Compose Up

```bash
cd RFSN_NPC_AI

# Start services
docker compose up --pull always

# In another terminal, test the API
curl -X GET http://127.0.0.1:8000/api/status

# View logs
docker compose logs -f orchestrator

# Stop services
docker compose down
```

**Expected output**: HTTP 200 with `{"status": "healthy"}`

#### Code Quality Checks

```bash
cd RFSN_NPC_AI/Python

# Install linters
pip install black isort flake8 pylint

# Run black (code formatting)
black . --check
black . --diff

# Run isort (import sorting)
isort . --check-only
isort . --diff

# Run flake8 (linting)
flake8 . --max-line-length=120 --extend-ignore=E203,W503

# Run pylint (advanced linting)
pylint Python/*.py --disable=all --enable=E,F
```

**Expected result**: No errors (warnings ok)

---

## 2. Git Setup & Pre-Commit Hooks

### Install Pre-Commit Framework

```bash
# Install pre-commit
pip install pre-commit

# Create .pre-commit-config.yaml in repo root
cat > .pre-commit-config.yaml << 'EOF'
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
        args: ['--safe']
      - id: check-json
      - id: check-merge-conflict
      - id: check-added-large-files
        args: ['--maxkb=1000']
      - id: detect-private-key

  - repo: https://github.com/psf/black
    rev: 23.12.1
    hooks:
      - id: black
        language_version: python3.11
        files: '^RFSN_NPC_AI/Python/'

  - repo: https://github.com/PyCQA/isort
    rev: 5.13.2
    hooks:
      - id: isort
        args: ['--profile', 'black']
        files: '^RFSN_NPC_AI/Python/'

  - repo: https://github.com/PyCQA/flake8
    rev: 6.1.0
    hooks:
      - id: flake8
        args: ['--max-line-length=120', '--extend-ignore=E203,W503']
        files: '^RFSN_NPC_AI/Python/'

  - repo: https://github.com/shellcheck-py/shellcheck-py
    rev: v0.9.0.5
    hooks:
      - id: shellcheck
        files: '^scripts/'

  - repo: https://github.com/hadialqattan/pycln
    rev: v2.2.2
    hooks:
      - id: pycln
        args: ['--all']
        files: '^RFSN_NPC_AI/Python/'
EOF
```

### Install Pre-Commit Hooks

```bash
# Install the git hooks
pre-commit install

# Run pre-commit on all files (first time)
pre-commit run --all-files

# Now pre-commit runs automatically on git commit
git add .
git commit -m "Add pre-commit hooks"
```

### Bypass Pre-Commit (If Needed)

```bash
git commit --no-verify
```

---

## 3. Git Workflow

### Feature Branch Development

```bash
# Update main
git fetch origin
git checkout main
git pull origin main

# Create feature branch
git checkout -b feature/my-feature

# Make changes and commit
git add .
git commit -m "Add my feature"

# Pre-commit hooks run automatically
# If checks fail, fix them and retry:
git add .
git commit -m "Add my feature"
```

### Before Pushing: Final Local Tests

```bash
# Run all checks
./scripts/ci-cd.sh all

# If all pass, push
git push origin feature/my-feature

# Create pull request on GitHub
```

### Push to GitHub

```bash
# Create feature branch and push
git checkout -b feature/my-feature
git add .
git commit -m "Implement feature"

# Push to remote
git push -u origin feature/my-feature

# GitHub UI: Create Pull Request
# Or via CLI:
gh pr create --title "My Feature" --body "Description"
```

---

## 4. GitHub Actions CI/CD Validation

### What Runs Automatically

When you push or create a PR:

#### `build-test.yml` Runs

1. **Python Tests** (Matrix: 3.10, 3.11, 3.12)
   - Installs dependencies
   - Runs `pytest tests/ -v --cov`
   - Uploads to codecov
   - Status: ✓ Pass or ✗ Fail

2. **Docker Build**
   - Builds `RFSN_NPC_AI/Dockerfile`
   - Uses GitHub Actions cache
   - Status: ✓ Pass or ✗ Fail

3. **Lint**
   - Black, isort, flake8, pylint
   - Optional markdown lint
   - Status: ✓ Pass or ⚠ Warnings

### View CI/CD Results

#### In GitHub UI

```text
Repository → Actions → [workflow name] → [run]
```

#### Via GitHub CLI

```bash
# List recent runs
gh run list

# Watch live
gh run watch

# View specific run
gh run view <run_id> --log

# View job status
gh run view <run_id>
```

#### Via Git Hooks (After Push)

```bash
# Check status locally
gh run list --branch feature/my-feature
```

### Fix CI/CD Failures

#### Python Test Failure

```bash
# Run locally to reproduce
cd RFSN_NPC_AI/Python
pytest tests/ -v

# Fix failing test
vim tests/test_*.py

# Run again
pytest tests/test_failed_test.py -v

# Commit and push
git add tests/test_*.py
git commit -m "Fix failing test"
git push
```

#### Docker Build Failure

```bash
# Build locally to reproduce
docker build -t island-backend:test -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Check Dockerfile for syntax errors
docker build --progress=plain -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Fix Dockerfile
vim RFSN_NPC_AI/Dockerfile

# Test again
docker build -t island-backend:test -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Commit and push
git add RFSN_NPC_AI/Dockerfile
git commit -m "Fix Dockerfile"
git push
```

#### Lint Failure (Code Style)

```bash
# Fix automatically
cd RFSN_NPC_AI/Python

black .
isort .

# Or fix manually
flake8 . --show-source
# Review output and fix

# Commit
git add .
git commit -m "Fix code style"
git push
```

---

## 5. Test Scenarios

### Scenario 1: New Feature (Python Code)

```bash
# 1. Create branch
git checkout -b feature/new-dialogue-action

# 2. Add code
echo "new code" >> RFSN_NPC_AI/Python/new_module.py

# 3. Add tests
echo "def test_new_feature(): assert True" >> RFSN_NPC_AI/Python/tests/test_new_module.py

# 4. Run local tests
./scripts/ci-cd.sh test

# Expected: 143 passed (142 + 1 new test)

# 5. Commit
git add .
git commit -m "Add new dialogue action"

# Pre-commit hooks run automatically
# If pass, continue:

# 6. Push
git push -u origin feature/new-dialogue-action

# 7. Monitor CI/CD
gh run watch

# Expected: All jobs pass ✓
```

### Scenario 2: Docker/Infrastructure Change

```bash
# 1. Create branch
git checkout -b feature/update-dockerfile

# 2. Modify Dockerfile
vim RFSN_NPC_AI/Dockerfile

# 3. Test locally
./scripts/ci-cd.sh build
./scripts/ci-cd.sh compose-up
./scripts/ci-cd.sh health

# 4. Commit
git add RFSN_NPC_AI/Dockerfile
git commit -m "Update Dockerfile for security hardening"

# 5. Push
git push -u origin feature/update-dockerfile

# 6. Monitor CI/CD
# Expected: Docker build job passes ✓
```

### Scenario 3: K8s/Terraform Change

```bash
# 1. Create branch
git checkout -b feature/update-k8s-replicas

# 2. Modify manifest
vim deployment/k8s/backend-hpa.yaml

# 3. Validate YAML locally
kubectl apply -f deployment/k8s/ --dry-run=client

# Or with kubeval:
pip install kubeval
kubeval deployment/k8s/*.yaml

# 4. Commit
git add deployment/k8s/backend-hpa.yaml
git commit -m "Update K8s HPA to scale to 15 replicas"

# 5. Push
git push -u origin feature/update-k8s-replicas

# 6. Monitor CI/CD
# Expected: Lint job passes ✓ (YAML validation via pre-commit)
```

### Scenario 4: Documentation Change

```bash
# 1. Create branch
git checkout -b docs/update-deployment-guide

# 2. Edit docs
vim DEPLOYMENT.md

# 3. Commit (no tests needed)
git add DEPLOYMENT.md
git commit -m "Update deployment guide with new examples"

# 4. Push
git push -u origin docs/update-deployment-guide

# 5. Monitor CI/CD
# Expected: All jobs pass ✓ (optional markdown lint)
```

---

## 6. Testing Specific Components

### Test Backend Services

```bash
# Start just the orchestrator (without dashboard)
cd RFSN_NPC_AI
docker compose up orchestrator

# In another terminal
curl -X POST http://127.0.0.1:8000/api/dialogue/stream \
  -H "Content-Type: application/json" \
  -d '{"npc_name":"test","user_input":"Hello"}'

# View response (SSE stream)
```

### Test K8s Manifests

```bash
# Validate YAML syntax
kubectl apply -f deployment/k8s/ --dry-run=client

# Dry-run deploy
kubectl apply -f deployment/k8s/ --dry-run=server

# Check for missing resources
kubectl apply -f deployment/k8s/ -o yaml | kubectl apply -f - --dry-run=client
```

### Test Terraform

```bash
cd deployment/terraform

# Validate HCL syntax
terraform fmt -check
terraform validate

# Plan (show what would be deployed)
terraform plan -var-file=example.tfvars -out=tfplan

# Show plan
terraform show tfplan

# Don't apply yet, just validate
terraform plan -var-file=example.tfvars | grep -E "^Plan:|^ " | head -20
```

---

## 7. Debugging Failed Tests

### Python Test Failures

```bash
# Run with verbose output
pytest tests/test_failing.py -vv -s

# Run with pdb debugger
pytest tests/test_failing.py --pdb

# Run with output capture disabled
pytest tests/test_failing.py -s

# Run with markers
pytest tests/ -m "not slow" -v

# Show print statements
pytest tests/ -v -s --tb=short
```

### Docker Build Failures

```bash
# Show full build log
docker build --progress=plain -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Build without cache
docker build --no-cache -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Build with specific target
docker build --target stage_name -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Check layer history
docker image history island-backend:test
```

### GitHub Actions Debug

```bash
# Enable debug logging
gh run view <run_id> --log --verbose

# Or re-run with debug
gh run rerun <run_id> --debug

# Check workflow file syntax
act -l  # List all workflows locally
act -j build-test  # Run build-test job locally (requires act tool)
```

---

## 8. Merging to Main

### Only After CI/CD Passes

```bash
# 1. Ensure all checks pass
gh run list --branch feature/my-feature
# Expected: All ✓ green

# 2. Create PR (if not already done)
gh pr create --title "My Feature" --body "Fixes #123"

# 3. Request review (optional)
gh pr review <pr_number> --request @teammate

# 4. Wait for approval + all checks to pass

# 5. Merge
gh pr merge <pr_number> --merge

# Or manually via GitHub UI: Click "Merge Pull Request"

# 6. Delete branch
git branch -d feature/my-feature
git push origin --delete feature/my-feature
```

### Branch Protection (Already Configured)

After merging to main, Docker Push workflow runs:

- Builds image
- Pushes to GHCR: `ghcr.io/owner/island/backend:main`

---

## 9. Testing Release Tags

### Create a Release

```bash
# Create and push a version tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# Monitor CI/CD (docker-push.yml runs)
gh run list --branch main
```

### Verify Release

```bash
# Check pushed images
docker pull ghcr.io/owner/island/backend:v1.0.0

# Check release on GitHub
gh release view v1.0.0
```

---

## 10. Common Git Commands

### Branch Management

```bash
# List branches
git branch -a

# Delete local branch
git branch -d feature/my-feature

# Delete remote branch
git push origin --delete feature/my-feature

# Rename branch
git branch -m old-name new-name

# Sync with main
git fetch origin
git rebase origin/main
```

### Commits & History

```bash
# View commit log
git log --oneline -10

# View changes before commit
git diff

# View changes in last commit
git show HEAD

# Amend last commit
git add .
git commit --amend --no-edit

# Revert commit
git revert <commit-hash>
```

### Stashing (Temporary Save)

```bash
# Save uncommitted changes
git stash

# List stashes
git stash list

# Restore stash
git stash pop

# Clear all stashes
git stash clear
```

---

## 11. Troubleshooting

### Pre-Commit Hooks Block Commit

**Problem**: Pre-commit hooks fail, commit blocked

**Solution**:

```bash
# Option 1: Fix issues and retry
# Hooks report the issue, fix code
git add .
git commit -m "message"

# Option 2: Bypass hooks (use with caution)
git commit --no-verify -m "message"

# Option 3: Run hooks manually to fix
pre-commit run --all-files
```

### CI/CD Fails on GitHub but Passes Locally

**Problem**: Tests pass locally but fail in GitHub Actions

**Solution**:

```bash
# Check Python version mismatch
python --version
# Should be 3.10 or 3.11 or 3.12

# CI tests all three versions
# If one fails, reproduce:
docker run -it python:3.10-slim bash
# Install and test inside container

# Check environment differences
env | grep PYTHON
env | grep PATH
```

### Docker Build Hangs

**Problem**: Docker build times out

**Solution**:

```bash
# Increase timeout
docker build --timeout=600 -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Or use buildx with higher timeout
docker buildx build --timeout=600 -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Check internet connection (layer download)
docker pull python:3.11-slim
```

### Git Push Rejected

**Problem**: `git push` rejected

**Solution**:

```bash
# Pull latest from remote first
git pull origin branch-name

# Resolve conflicts if any
# Then push again
git push origin branch-name

# Or force push (only for feature branches, not main)
git push -f origin feature/my-feature  # USE WITH CAUTION
```

---

## Summary: Quick Reference

### Before Committing

```bash
./scripts/ci-cd.sh all              # Test everything locally
```

### Before Pushing

```bash
git status                           # Check what's changed
git add .                            # Stage changes
git commit -m "message"              # Pre-commit hooks run
git push -u origin feature/branch    # Push to GitHub
```

### Monitor CI/CD

```bash
gh run watch                         # Watch live
gh run list --branch main            # See recent runs
```

### Merge to Main

```bash
gh pr create --title "Title"         # Create PR
# Wait for ✓ all checks
gh pr merge <number>                 # Merge after review
# docker-push.yml runs automatically
```

### Deploy (After Merge)

```bash
# Option 1: K8s (manual)
kubectl set image deployment/island-backend \
  -n island \
  orchestrator=ghcr.io/.../backend:main

# Option 2: ECS (manual)
aws ecs update-service --cluster island --service island-backend-service --force-new-deployment

# Option 3: Cloud Run (manual)
gcloud run deploy island-backend --image gcr.io/PROJECT/backend:main
```
