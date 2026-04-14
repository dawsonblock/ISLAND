#!/bin/bash
set -euo pipefail

# Island Backend CI/CD Helper Script
# Automates Docker build, test, and deployment workflows

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Backend tests
run_backend_tests() {
    log_info "Running Python backend tests..."
    cd "$REPO_ROOT/RFSN_NPC_AI/Python"
    
    if ! command -v pytest &> /dev/null; then
        log_warn "pytest not found, installing..."
        pip install pytest pytest-cov pytest-asyncio
    fi
    
    pytest tests/ -v --cov=. --cov-report=term --cov-report=xml
    log_info "Backend tests passed!"
}

# Docker build
build_docker() {
    local tag="${1:-island-backend:latest}"
    log_info "Building Docker image: $tag"
    
    cd "$REPO_ROOT"
    docker build \
        -f RFSN_NPC_AI/Dockerfile \
        -t "$tag" \
        .
    
    log_info "Docker build complete: $tag"
}

# Docker compose up
docker_compose_up() {
    log_info "Starting services with docker-compose..."
    cd "$REPO_ROOT/RFSN_NPC_AI"
    docker compose up --pull always -d
    
    log_info "Waiting for backend to be healthy..."
    sleep 5
    
    for i in {1..30}; do
        if curl -f http://127.0.0.1:8000/api/status > /dev/null 2>&1; then
            log_info "Backend is healthy!"
            return 0
        fi
        log_warn "Attempt $i/30: Backend not yet ready, retrying..."
        sleep 2
    done
    
    log_error "Backend failed to become healthy"
    return 1
}

# Docker compose down
docker_compose_down() {
    log_info "Stopping services..."
    cd "$REPO_ROOT/RFSN_NPC_AI"
    docker compose down
    log_info "Services stopped"
}

# Health check
health_check() {
    log_info "Health check..."
    
    if curl -f http://127.0.0.1:8000/api/status > /dev/null 2>&1; then
        log_info "Backend is healthy"
        return 0
    else
        log_error "Backend health check failed"
        return 1
    fi
}

# Push to registry
push_to_registry() {
    local tag="$1"
    local registry="${2:-ghcr.io}"
    
    log_info "Tagging image for registry: $registry/$tag"
    docker tag island-backend:latest "$registry/$tag"
    
    log_info "Pushing to $registry..."
    docker push "$registry/$tag"
    
    log_info "Push complete!"
}

# Main
main() {
    local command="${1:-help}"
    
    case "$command" in
        test)
            run_backend_tests
            ;;
        build)
            local tag="${2:-island-backend:latest}"
            build_docker "$tag"
            ;;
        compose-up)
            docker_compose_up
            ;;
        compose-down)
            docker_compose_down
            ;;
        health)
            health_check
            ;;
        push)
            local tag="${2:-island-backend:latest}"
            local registry="${3:-ghcr.io}"
            push_to_registry "$tag" "$registry"
            ;;
        all)
            run_backend_tests
            build_docker "island-backend:latest"
            docker_compose_up
            health_check
            ;;
        *)
            echo "Usage: $0 <command> [args]"
            echo ""
            echo "Commands:"
            echo "  test                  Run Python backend tests"
            echo "  build [TAG]           Build Docker image"
            echo "  compose-up            Start services with docker-compose"
            echo "  compose-down          Stop services"
            echo "  health                Check backend health"
            echo "  push TAG [REGISTRY]   Push image to registry"
            echo "  all                   Run test, build, compose-up, health-check"
            exit 1
            ;;
    esac
}

main "$@"
