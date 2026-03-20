"""Compatibility shim. Real implementation: app.memory.memory_manager."""

from app.memory.memory_manager import ConversationManager, ConversationTurn, list_backups

__all__ = ["ConversationManager", "ConversationTurn", "list_backups"]
