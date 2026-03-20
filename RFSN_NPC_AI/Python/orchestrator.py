"""Compatibility shim. Real implementation: app.api.main."""

from app.api.main import app, health_check, streaming_engine

__all__ = ["app", "health_check", "streaming_engine"]
