"""Compatibility shim. Real implementation: app.learning.bandit_learner."""

from app.learning.bandit_learner import ArmStats, BanditConfig, StateActionBandit

__all__ = ["ArmStats", "BanditConfig", "StateActionBandit"]
