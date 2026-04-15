"""Package-local bridge for the legacy root-level model_manager module."""

from ._root_module_bridge import load_root_module


_module = load_root_module("model_manager")

ModelManager = _module.ModelManager
setup_models = _module.setup_models
ensure_llm_model_exists = _module.ensure_llm_model_exists


__all__ = ["ModelManager", "setup_models", "ensure_llm_model_exists"]