"""Compatibility shim. Real implementation: app.dialogue.action_scorer."""

from app.dialogue.action_scorer import (
    ActionScore,
    ActionScorer,
    DecisionPipeline,
    UtilityFunction,
)

__all__ = ["ActionScore", "ActionScorer", "DecisionPipeline", "UtilityFunction"]
