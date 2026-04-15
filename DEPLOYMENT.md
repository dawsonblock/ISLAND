# ISLAND Deployment Guide

## Overview

ISLAND consists of two main components:

1. **Unreal Engine Frontend** (5.5 C++ game)
2. **RFSN Python Backend** (FastAPI + Kokoro TTS + Ollama LLM)

This guide covers containerizing and deploying the Python backend to production. The Unreal game ships standalone and connects to the backend via HTTP/SSE.

---

## Quick Start (Local Development)

### Prerequisites

- Docker 20.10+
- Docker Compose 2.0+
- 8GB+ RAM
- 4GB disk space (for models)

### Run Locally

```bash
cd RFSN_NPC_AI
docker compose up --pull always
```

**Services**:
- Backend: http://127.0.0.1:8000
- Dashboard: http://127.0.0.1:8080
- Health check: GET /api/status

### Environment Variables

Create `.env` in the repo root:

```bash
# Required in production
JWT_SECRET=your-secret-key-here-32-chars-minimum

# Optional (defaults to config.json values)
RFSN_LOG_LEVEL=INFO
RFSN_PORT=8000
RFSN_HOST=0.0.0.0
OLLAMA_HOST=http://localhost:11434
```

---

## Docker Image

### Build Locally

```bash
# From repo root
docker build -t island-backend:latest -f RFSN_NPC_AI/Dockerfile RFSN_NPC_AI

# Run standalone
docker run -p 8000:8000 \
  -v $(pwd)/RFSN_NPC_AI/Models:/app/Models \
  -v $(pwd)/RFSN_NPC_AI/memory:/app/memory \
  -e JWT_SECRET=dev-secret \
  island-backend:latest
```

### Image Details

- **Base**: `python:3.11-slim`
- **Size**: ~1.2GB (with models)
- **Entrypoint**: `python -m uvicorn orchestrator:app --host 0.0.0.0 --port 8000`
- **Healthcheck**: `/api/status` every 30s
- **Exposed Port**: 8000

---

## Kubernetes Deployment

### Prerequisites

- Kubernetes 1.24+ cluster
- kubectl configured
- Persistent storage class available

### Deploy to Kubernetes

```bash
kubectl apply -f deployment/k8s/backend-namespace.yaml
kubectl apply -f deployment/k8s/backend-configmap.yaml
kubectl apply -f deployment/k8s/backend-secret.yaml
kubectl apply -f deployment/k8s/backend-deployment.yaml
kubectl apply -f deployment/k8s/backend-service.yaml
kubectl apply -f deployment/k8s/backend-hpa.yaml
```

### Verify Deployment

```bash
# Check pods
kubectl get pods -n island

# View logs
kubectl logs -n island -f deployment/island-backend

# Port-forward to test locally
kubectl port-forward -n island svc/island-backend 8000:8000

# Check HPA status
kubectl get hpa -n island
```

---

## Cloud Deployments

### AWS ECS/Fargate

```bash
# Build and push to ECR
aws ecr get-login-password --region us-east-1 | \
  docker login --username AWS --password-stdin $ECR_REGISTRY

docker tag island-backend:latest $ECR_REGISTRY/island-backend:latest
docker push $ECR_REGISTRY/island-backend:latest

# Deploy with CloudFormation or Terraform
terraform apply -var image_uri=$ECR_REGISTRY/island-backend:latest
```

**Recommended Settings**:
- Memory: 2GB
- vCPU: 0.5–1
- Environment: Fargate on-demand
- Logging: CloudWatch logs group `/island/backend`

### Google Cloud Run

```bash
# Push to Artifact Registry
gcloud auth configure-docker us-central1-docker.pkg.dev

docker tag island-backend:latest \
  us-central1-docker.pkg.dev/PROJECT_ID/island/backend:latest

docker push us-central1-docker.pkg.dev/PROJECT_ID/island/backend:latest

# Deploy
gcloud run deploy island-backend \
  --image us-central1-docker.pkg.dev/PROJECT_ID/island/backend:latest \
  --region us-central1 \
  --memory 2Gi \
  --cpu 1 \
  --set-env-vars JWT_SECRET=$JWT_SECRET,LOG_LEVEL=INFO \
  --allow-unauthenticated
```

### Azure Container Instances

```bash
# Push to ACR
az acr login --name island

docker tag island-backend:latest island.azurecr.io/island-backend:latest
docker push island.azurecr.io/island-backend:latest

# Deploy
az container create \
  --resource-group island-rg \
  --name island-backend \
  --image island.azurecr.io/island-backend:latest \
  --ports 8000 \
  --environment-variables \
    JWT_SECRET=$JWT_SECRET \
    LOG_LEVEL=INFO \
  --memory 2
```

---

## Production Checklist

### Security

- [ ] Change `JWT_SECRET` to a strong random value (32+ chars)
- [ ] Use HTTPS/TLS in reverse proxy (nginx, Traefik)
- [ ] Set `LOG_LEVEL=WARNING` in production
- [ ] Enable authentication on `/api/*` endpoints
- [ ] Rotate JWT secrets regularly
- [ ] Run container as non-root (already done in image)

### Performance

- [ ] Enable container resource limits (2GB memory, 1 CPU)
- [ ] Configure autoscaling (HPA for K8s, autoscaling group for ECS)
- [ ] Set up CDN for static assets (`/api/static/*`)
- [ ] Enable request compression (gzip)
- [ ] Cache responses where appropriate (Redis)

### Observability

- [ ] Forward logs to centralized logging (Datadog, ELK, Splunk)
- [ ] Monitor `/api/metrics` endpoint
- [ ] Set up alerting on:
  - Pod/container crashes
  - High error rates (5xx responses)
  - High latency (p99 > 3s)
  - Memory/CPU exhaustion
- [ ] Use distributed tracing (Jaeger, DataDog APM)

### Data & Backups

- [ ] Persist `/app/Models` volume (NFS, EBS, Persistent Disk)
- [ ] Persist `/app/memory` volume with daily backups
- [ ] Test backup restoration regularly
- [ ] Use read-only config mounts (`config.json:ro`)
- [ ] Enable audit logging for conversation data

### Networking

- [ ] Use service mesh (optional, Istio/Linkerd) for traffic management
- [ ] Configure rate limiting (50 req/sec per client)
- [ ] Enable CORS only for trusted origins
- [ ] Use private networking (VPC/VNet) for backend
- [ ] Restrict egress to Ollama/LLM endpoints only

---

## Monitoring & Logs

### Local Development

```bash
# View logs in real-time
docker compose logs -f orchestrator

# Check resource usage
docker stats rfsn-orchestrator

# Access dashboard
open http://127.0.0.1:8080
```

### Production Logging

All logs are output to stdout (captured by container runtime):

```bash
# Kubernetes
kubectl logs -n island -f deployment/island-backend

# Docker
docker logs -f rfsn-orchestrator

# ECS
aws logs tail /island/backend --follow

# Cloud Run
gcloud logging read "resource.type=cloud_run_revision AND resource.labels.service_name=island-backend" --limit 50
```

### Key Metrics

- **First Token Latency**: < 1.5s
- **Sentence Detection**: < 50ms
- **TTS Generation**: < 100ms
- **Queue Throughput**: 10+ items/sec
- **Error Rate**: < 0.1%

---

## Scaling

### Horizontal Scaling

For multiple replicas, use:

**Docker Compose**:
```yaml
services:
  orchestrator:
    deploy:
      replicas: 3
```

**Kubernetes HPA**:
```bash
kubectl autoscale deployment island-backend \
  --min=1 --max=10 \
  --cpu-percent=70
```

**AWS ECS**:
- Set desired count to 2+
- Enable Service Auto Scaling based on CPU/memory

### Load Balancing

Behind a reverse proxy (nginx, Traefik, AWS ALB):

```nginx
upstream island_backend {
    server localhost:8000;
    server localhost:8001;
    server localhost:8002;
}

server {
    listen 80;
    server_name api.island.example.com;

    location / {
        proxy_pass http://island_backend;
        proxy_http_version 1.1;
        proxy_set_header Connection "";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

---

## Troubleshooting

### Container won't start

```bash
docker logs rfsn-orchestrator
# Check for: missing requirements, model files, port conflicts
```

### Health check failing

```bash
curl -v http://127.0.0.1:8000/api/status
# Expected: HTTP 200 with {"status": "healthy"}
```

### High memory usage

```bash
docker stats rfsn-orchestrator
# Likely cause: large conversation history in memory
# Solution: increase memory limit or enable memory cleanup
```

### Slow responses

```bash
# Check LLM backend (Ollama)
curl http://localhost:11434/api/tags

# Profile orchestrator
docker exec rfsn-orchestrator python -m cProfile -o /tmp/prof.stats orchestrator.py
```

### Models not loading

```bash
docker exec rfsn-orchestrator ls -lah /app/Models
# Ensure models volume is mounted and has read permissions
```

---

## Integration with Unreal Frontend

### Unreal Client Configuration

In the Unreal game, set the RFSN backend URL in Blueprint or code:

```cpp
// C++
FString BackendURL = TEXT("http://api.island.example.com:8000");
// or from environment
BackendURL = FPlatformMisc::GetEnvironmentVariable(TEXT("RFSN_BACKEND_URL"));
```

### Connection Testing

From Unreal or locally:

```bash
# Test streaming endpoint
curl -X POST http://127.0.0.1:8000/api/dialogue/stream \
  -H "Content-Type: application/json" \
  -d '{"npc_name":"test","user_input":"Hello"}'

# Test health
curl http://127.0.0.1:8000/api/status
```

### Offline Mode

The Unreal slice remains fully playable if the backend is down:
- Tower progression works offline
- Dialogue falls back to local barks
- No gameplay blocking on network errors

---

## Next Steps

1. **Set up CI/CD**: Use `.github/workflows/build-test.yml` and `docker-push.yml`
2. **Choose cloud platform**: AWS/GCP/Azure templates provided above
3. **Configure monitoring**: Datadog, CloudWatch, or ELK
4. **Scale to 3+ replicas** behind load balancer
5. **Enable TLS** with Let's Encrypt (Certbot/cert-manager)
6. **Document your deployment** in team wiki

---

## Support & References

- **Orchestrator README**: `RFSN_NPC_AI/README.md`
- **Python Tests**: `RFSN_NPC_AI/Python/tests/`
- **Config Reference**: `RFSN_NPC_AI/config.json`
- **API Docs**: `http://<backend-url>:8000/docs` (Swagger UI)

