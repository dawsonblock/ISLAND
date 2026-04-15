"""Compatibility wrapper. Canonical implementation: app.api.main."""

from app.api import main as _main


app = _main.app
health_check = _main.health_check
shutdown_event = _main.shutdown_event
startup_event = _main.startup_event
streaming_engine = _main.streaming_engine


def __getattr__(name):
	return getattr(_main, name)


__all__ = [
	"app",
	"health_check",
	"shutdown_event",
	"startup_event",
	"streaming_engine",
]
