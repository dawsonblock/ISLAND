"""Compatibility shim. Real implementation: app.dialogue.world_model."""

from app.dialogue.world_model import (
    HandAuthoredRules,
    NPCAction,
    PlayerSignal,
    RetrievalWorldModel,
    StateSnapshot,
    Transition,
    WorldModel,
)

__all__ = [
    "HandAuthoredRules",
    "NPCAction",
    "PlayerSignal",
    "RetrievalWorldModel",
    "StateSnapshot",
    "Transition",
    "WorldModel",
]
