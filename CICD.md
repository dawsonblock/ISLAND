# Build & Test Workflow

## Overview

The ISLAND project includes automated testing and Docker build workflows.

### CI/CD Pipelines

#### 1. Build & Test (`.github/workflows/build-test.yml`)

Runs on every push and PR:

- **Python Tests** (3.10, 3.11, 3.12)
  - 142 unit tests covering streaming, learning layer, world model
  - Code coverage with codecov
  - Run time: ~5-10 minutes

- **Docker Build** 
  - Builds backend image from Dockerfile
  - Caches layers for speed
  - Run time: ~15-30 minutes (first run), ~2-5 minutes (cached)

- **Code Quality**
  - Black (code formatting)
  - isort (import sorting)
  - Flake8 (linting)
  - Optional markdown lint

#### 2. Docker Push (`.github/workflows/docker-push.yml`)

Runs on push to `main` or tags:

- Logs into Docker Hub (requires `DOCKER_USERNAME`, `DOCKER_PASSWORD`)
- Logs into GitHub Container Registry (auto)
- Pushes with semantic versioning tags
- Tags: branch, semver (v1.2.3), commit SHA

### Setup

#### GitHub Secrets

Required for Docker Push workflow:

```bash
DOCKER_USERNAME=your_docker_hub_username
DOCKER_PASSWORD=your_docker_hub_password (or personal access token)
```

Optional (for Kubernetes deployment):

```bash
KUBE_CONFIG=<base64 encoded kubeconfig>
ECR_REGISTRY=123456789.dkr.ecr.us-east-1.amazonaws.com
ECR_REPOSITORY=island-backend
AWS_ROLE_TO_ASSUME=arn:aws:iam::123456789:role/github-actions
```

Add secrets via:
```bash
# GitHub UI: Settings → Secrets and variables → Actions
# Or CLI:
gh secret set DOCKER_PASSWORD --body "<token>"
```

#### Branch Protection

Configure in GitHub:

1. Settings → Branches → Add rule
2. Branch name pattern: `main`
3. Require checks to pass:
   - `backend-test (3.10, 3.11, 3.12)`
   - `backend-docker`
   - `lint`

### Local Development

Use the CI/CD helper script:

```bash
# Run tests
./scripts/ci-cd.sh test

# Build Docker image
./scripts/ci-cd.sh build island-backend:latest

# Start local services
./scripts/ci-cd.sh compose-up

# Stop services
./scripts/ci-cd.sh compose-down

# Health check
./scripts/ci-cd.sh health

# All of the above
./scripts/ci-cd.sh all

# Push to registry
./scripts/ci-cd.sh push island-backend:v1.0.0 ghcr.io
```

### Deployment Workflows (Manual)

#### Deploy to Kubernetes

```bash
# After Docker Push workflow succeeds:
kubectl set image deployment/island-backend \
  -n island \
  orchestrator=ghcr.io/dawsonblock/island/backend:main
```

#### Deploy to ECS

```bash
# Trigger task update after Docker Push
aws ecs update-service \
  --cluster island-cluster \
  --service island-backend-service \
  --force-new-deployment
```

#### Deploy to Cloud Run

```bash
gcloud run deploy island-backend \
  --image gcr.io/PROJECT_ID/island-backend:latest \
  --region us-central1
```

### Testing Locally

#### Run Backend Tests

```bash
cd RFSN_NPC_AI/Python
python -m venv .venv
source .venv/bin/activate
pip install -r requirements-core.txt
pytest tests/ -v
```

#### Run Specific Tests

```bash
# Learning layer
pytest tests/test_bandit_learner.py -v

# Streaming
pytest tests/test_streaming_fixes.py -v

# Fidelity (tokenization)
pytest tests/test_fidelity.py -v

# Coverage
pytest tests/ --cov=. --cov-report=html
open htmlcov/index.html
```

### Build & Push Manually

```bash
# Build
docker build -t island-backend:latest -f RFSN_NPC_AI/Dockerfile .

# Tag for registry
docker tag island-backend:latest ghcr.io/USERNAME/island-backend:latest

# Login (if needed)
docker login ghcr.io

# Push
docker push ghcr.io/USERNAME/island-backend:latest
```

### Troubleshooting

#### Docker Build Fails

```bash
# Check Dockerfile syntax
docker build --no-cache -f RFSN_NPC_AI/Dockerfile .

# View build logs
docker buildx build --progress=plain -f RFSN_NPC_AI/Dockerfile .
```

#### Tests Fail Locally But Pass in CI

- Check Python version: `python --version` (should be 3.10+)
- Check dependencies: `pip install -r RFSN_NPC_AI/Python/requirements-core.txt`
- Clear cache: `pytest --cache-clear tests/`

#### CI Secrets Missing

- Verify secrets are set: `gh secret list`
- Check secret names match workflow file exactly
- Re-create secret if corrupted: `gh secret delete NAME && gh secret set NAME`

### Monitoring CI/CD

View workflow runs:

```bash
# List recent runs
gh run list --repo=USERNAME/ISLAND

# View logs for specific run
gh run view RUN_ID --log

# Watch live
gh run watch
```

Or via GitHub UI:
- Actions tab → Select workflow → Recent runs

### Deployment Strategy

**Development** (push to `develop`):
- Build image with tag `develop`
- Deploy to dev cluster/Fargate
- Run integration tests

**Staging** (merge to `staging` branch):
- Build image with tag `staging`
- Deploy to staging cluster
- Run smoke tests + performance tests

**Production** (tag with `v*`):
- Build image with semver tag (v1.0.0, v1.0.1)
- Deploy to prod after manual approval
- Run canary deployment (5% traffic, then 25%, then 100%)

