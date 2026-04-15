"""Package-local bridge for the legacy root-level latency_optimizations module."""

from ._root_module_bridge import load_root_module


_module = load_root_module("latency_optimizations")

InstantBarkSystem = _module.InstantBarkSystem
UserCentricRewardSystem = _module.UserCentricRewardSystem
OptimizedPipeline = _module.OptimizedPipeline
ClauseTokenizer = _module.ClauseTokenizer
BarkCategory = _module.BarkCategory
create_user_reward_system = _module.create_user_reward_system


__all__ = [
    "InstantBarkSystem",
    "UserCentricRewardSystem",
    "OptimizedPipeline",
    "ClauseTokenizer",
    "BarkCategory",
    "create_user_reward_system",
]