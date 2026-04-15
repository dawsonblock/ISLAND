#!/usr/bin/env python3
"""
Semantic Memory Module
Efficient vector-based memory retrieval using SentenceTransformers.
"""
import os
import json
import logging
from dataclasses import dataclass
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Any

import numpy as np

from ..runtime_paths import runtime_dir

os.environ.setdefault("TOKENIZERS_PARALLELISM", "false")

SentenceTransformer = None
cosine_similarity = None

logger = logging.getLogger(__name__)


def _patch_multiprocess_resource_tracker() -> None:
    """Work around the Python 3.12 cleanup bug triggered by sentence-transformers."""
    try:
        from multiprocess import resource_tracker as mp_resource_tracker
    except ImportError:
        return

    if getattr(mp_resource_tracker.ResourceTracker, "_rfsn_safe_del", False):
        return

    original_del = getattr(mp_resource_tracker.ResourceTracker, "__del__", None)
    if original_del is None:
        return

    def _safe_del(self):
        try:
            original_del(self)
        except AttributeError as exc:
            if "_recursion_count" not in str(exc):
                raise

    mp_resource_tracker.ResourceTracker.__del__ = _safe_del
    mp_resource_tracker.ResourceTracker._rfsn_safe_del = True


def _load_embedding_dependencies() -> bool:
    global SentenceTransformer, cosine_similarity

    if SentenceTransformer is not None and cosine_similarity is not None:
        return True

    _patch_multiprocess_resource_tracker()

    try:
        from sentence_transformers import SentenceTransformer as sentence_transformer_cls
        from sklearn.metrics.pairwise import cosine_similarity as cosine_similarity_fn
    except ImportError:
        return False

    SentenceTransformer = sentence_transformer_cls
    cosine_similarity = cosine_similarity_fn
    return True

@dataclass
class VectorEntry:
    memory_id: str
    vector_index: int  # Index in the numpy array

class SemanticMemory:
    """
    Manages semantic embeddings for memories.
    Uses 'all-MiniLM-L6-v2' (lightweight, efficient) by default.
    """
    
    def __init__(self, storage_dir: str | Path | None = None, model_name: str = "all-MiniLM-L6-v2"):
        self.storage_dir = Path(storage_dir) if storage_dir is not None else runtime_dir("memory", "vectors")
        self.storage_dir.mkdir(parents=True, exist_ok=True)
        self.model_name = model_name
        
        self.index_file = self.storage_dir / "index.json"
        self.vectors_file = self.storage_dir / "vectors.npy"
        
        self.entries: List[VectorEntry] = []
        self.vectors: Optional[np.ndarray] = None
        self.model = None
        self.enabled = False
        
        # Initialize model
        self._load_model()
        # Load data
        self._load_data()
        
    def _load_model(self):
        """Load the embedding model"""
        if not _load_embedding_dependencies():
            logger.error("sentence-transformers not installed. Semantic memory disabled.")
            return

        try:
            logger.info(f"Loading Semantic Model: {self.model_name}...")
            # Use local cache or download
            self.model = SentenceTransformer(self.model_name)
            self.enabled = True
            logger.info("Semantic Model loaded.")
        except Exception as e:
            logger.error(f"Failed to load semantic model: {e}")
            self.enabled = False

    def _load_data(self):
        """Load index and vectors from disk"""
        if not self.enabled:
            return
            
        try:
            if self.index_file.exists():
                with open(self.index_file, 'r') as f:
                    data = json.load(f)
                    self.entries = [VectorEntry(**e) for e in data]
            
            if self.vectors_file.exists():
                self.vectors = np.load(self.vectors_file)
                
            if self.vectors is not None and len(self.entries) != self.vectors.shape[0]:
                logger.warning("Vector index mismatch! Resetting semantic memory.")
                self.clear()
                
            logger.info(f"Loaded {len(self.entries)} semantic memories.")
            
        except Exception as e:
            logger.error(f"Failed to load semantic data: {e}")
            self.clear()

    def _save_data(self):
        """Save index and vectors to disk"""
        if not self.enabled:
            return
            
        try:
            with open(self.index_file, 'w') as f:
                json.dump([vars(e) for e in self.entries], f)
            
            if self.vectors is not None:
                np.save(self.vectors_file, self.vectors)
                
        except Exception as e:
            logger.error(f"Failed to save semantic data: {e}")

    def embed(self, text: str) -> Optional[np.ndarray]:
        """Generate embedding for text"""
        if not self.enabled or not self.model:
            return None
        try:
            # Generate normalized embedding for cosine similarity
            return self.model.encode(text, normalize_embeddings=True)
        except Exception as e:
            logger.error(f"Embedding generation failed: {e}")
            return None

    def add_memory(self, memory_id: str, content: str):
        """Add a memory embedding"""
        if not self.enabled:
            return
            
        # Check if already exists
        for entry in self.entries:
            if entry.memory_id == memory_id:
                return # Idempotent

        vector = self.embed(content)
        if vector is None:
            return
            
        if self.vectors is None:
            self.vectors = np.array([vector])
        else:
            self.vectors = np.vstack([self.vectors, vector])
            
        entry = VectorEntry(memory_id=memory_id, vector_index=len(self.entries))
        self.entries.append(entry)
        self._save_data()
        logger.debug(f"Added semantic memory {memory_id}")

    def search(self, query: str, k: int = 5, min_score: float = 0.3) -> List[Tuple[str, float]]:
        """
        Search for relevant memories.
        Returns list of (memory_id, score).
        """
        if not self.enabled or self.vectors is None or len(self.entries) == 0:
            return []
            
        query_vector = self.embed(query)
        if query_vector is None:
            return []
            
        # Reshape for sklearn
        query_vector = query_vector.reshape(1, -1)
        
        # Calculate cosine similarity
        scores = cosine_similarity(query_vector, self.vectors)[0]
        
        # Get top k
        top_indices = np.argsort(scores)[::-1][:k]
        
        results = []
        for idx in top_indices:
            score = float(scores[idx])
            if score >= min_score:
                memory_id = self.entries[idx].memory_id
                results.append((memory_id, score))
        
        return results

    def clear(self):
        """Clear all semantic data"""
        self.entries = []
        self.vectors = None
        self._save_data()
