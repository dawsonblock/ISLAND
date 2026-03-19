#!/usr/bin/env python3
"""
RFSN Streaming Engine v9.0 - Thread-Safe Deque Backend
Fixes: sentence fragmentation, race conditions via deque+Condition, token bleeding, backpressure
"""

import threading
import re
import time
import logging
from typing import Generator, Optional, List, Callable, Dict, Any
from dataclasses import dataclass, field
from pathlib import Path
from .streaming_voice_system import DequeSpeechQueue, VoiceChunk

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

CLOSERS = set(['"', "'", "”", "’", ")", "]", "}"])
BOUNDARY_FLUSH_MS = 180  # flush boundary if no continuation arrives quickly
WORKER_TIMEOUT_SEC = 0.5  # TTS worker queue timeout (Phase 2)
QUEUE_SIZE_MIN = 1
QUEUE_SIZE_MAX = 50
SHUTDOWN_TIMEOUT_SEC = 5  # Worker shutdown grace period
ABBREVIATION_PATTERN = re.compile(r'(\w+)$')  # Pre-compiled for performance


@dataclass
class SentenceChunk:
    """Represents a complete sentence chunk for TTS"""
    text: str
    is_final: bool = False
    latency_ms: float = 0.0


class StreamTokenizer:
    """
    Stateful sentence splitter for streaming text.
    Defers end-of-buffer terminals until more context arrives or flush() is
    called, while preserving quotes, ellipses, and common abbreviations.
    """

    def __init__(self):
        self.buffer = ""
        self.in_quotes = False
        self._pending_boundary = False
        self._pending_boundary_deadline = 0.0
        self.abbreviations = {
            'mr', 'mrs', 'ms', 'dr', 'prof', 'sr', 'jr', 'st',
            'mt', 'capt', 'col', 'gen', 'lt', 'sgt', 'corp',
            'etc', 'vs', 'inc', 'ltd', 'fig', 'op'
        }

    def _is_abbreviation(self, text: str, period_idx: int) -> bool:
        if period_idx <= 0:
            return False
        word_match = ABBREVIATION_PATTERN.search(text[:period_idx])
        if not word_match:
            return False
        return word_match.group(1).lower() in self.abbreviations

    def _next_significant_char(self, start_idx: int):
        idx = start_idx
        while idx < len(self.buffer):
            if not self.buffer[idx].isspace():
                return idx, self.buffer[idx]
            idx += 1
        return None

    def _find_sentence_boundary(self):
        in_quotes = False
        quote_chars = {'"', '“', '”'}
        quote_closers = {'"', '”'}

        idx = 0
        while idx < len(self.buffer):
            char = self.buffer[idx]

            if char in quote_chars:
                in_quotes = not in_quotes

            if char in '.!?':
                terminal_end = idx
                is_ellipsis = char == '.' and self.buffer[idx:idx + 3] == '...'
                if is_ellipsis:
                    terminal_end = min(idx + 2, len(self.buffer) - 1)

                if char == '.' and not is_ellipsis and self._is_abbreviation(self.buffer, idx):
                    idx += 1
                    continue

                suffix_end = terminal_end + 1
                has_closing_quote = False
                while suffix_end < len(self.buffer) and self.buffer[suffix_end] in CLOSERS:
                    if self.buffer[suffix_end] in quote_closers:
                        has_closing_quote = True
                    suffix_end += 1

                if suffix_end >= len(self.buffer):
                    return None

                next_char = self.buffer[suffix_end]
                if next_char == '\n':
                    return suffix_end + 1

                if next_char.isspace():
                    next_sig = self._next_significant_char(suffix_end)
                    if next_sig is None:
                        return None

                    _, next_sig_char = next_sig
                    if in_quotes and has_closing_quote and next_sig_char.islower():
                        idx = terminal_end + 1
                        continue
                    return suffix_end + 1

            idx += 1

        return None

    def process(self, token: str) -> List[str]:
        if self._pending_boundary:
            next_sig = next((char for char in token if not char.isspace()), "")
            if next_sig and next_sig.isalnum():
                self._pending_boundary = False

        self.buffer += token

        sentences = []
        while True:
            split_idx = self._find_sentence_boundary()
            if split_idx is None:
                break

            sentence = self.buffer[:split_idx].strip()
            clean_sentence = self._clean_for_tts(sentence)
            if clean_sentence and len(clean_sentence) > 1:
                sentences.append(clean_sentence)

            self.buffer = self.buffer[split_idx:].lstrip()
            self._pending_boundary = False

        return sentences

    def _clean_for_tts(self, text: str) -> str:
        text = re.sub(r'\*[^\*]+\*', '', text)
        text = re.sub(r'\[[^\]]+\]', '', text)
        text = ' '.join(text.split())
        return text

    def flush(self) -> List[str]:
        res = []
        if self.buffer.strip():
            sentence = self.buffer.strip()
            clean_sentence = self._clean_for_tts(sentence)
            if clean_sentence and len(clean_sentence) > 1:
                logger.info(f"[TTS-DEBUG] Queueing flushed sentence: '{clean_sentence}'")
                res.append(clean_sentence)
        self.buffer = ""
        self.in_quotes = False
        self._pending_boundary = False
        self._pending_boundary_deadline = 0.0
        return res


@dataclass
class StreamingMetrics:
    """Performance metrics for streaming pipeline"""
    first_token_ms: float = 0.0
    first_sentence_ms: float = 0.0
    total_generation_ms: float = 0.0
    tts_queue_size: int = 0
    dropped_sentences: int = 0


@dataclass
class RFSNState:
    """RFSN mathematical state model for NPC behavior"""
    npc_name: str
    affinity: float = 0.0
    playstyle: str = "Unknown"
    mood: str = "Neutral"
    relationship: str = "Stranger"
    combat_active: bool = False
    
    def get_attitude_instruction(self) -> str:
        """Convert affinity math to English instruction for LLM"""
        if self.affinity >= 0.85:
            return "Devoted and loving. You would die for them."
        if self.affinity >= 0.65:
            return "Warm and affectionate close friend."
        if self.affinity >= 0.35:
            return "Friendly and supportive."
        if self.affinity >= 0.05:
            return "Polite and professional."
        if self.affinity >= -0.35:
            return "Neutral and distant."
        if self.affinity >= -0.65:
            return "Cold, suspicious, and dismissive."
        return "Hostile and openly aggressive."


class StreamingVoiceSystem:
    """
    Production voice system with:
    - Smart sentence boundaries (avoids abbreviations)
    - Thread-safe playback with backpressure
    - Automatic token filtering
    - Error handling (no silent failures)
    """
    
    # Tokens that should NEVER be spoken (LLM control tokens)
    BAD_TOKENS = {
        "<" + "|eot_id|>", "<" + "|end|>", "<" + "|start_header_id|>", 
        "<" + "|end_header_id|>", "<" + "|assistant|>", "<" + "|user|>", 
        "<" + "|system|>", "[INST]", "[/INST]", "</s>", "<|end|>"
    }
    
    
    def __init__(self, tts_engine: str = "piper", max_queue_size: int = 3):
        self.tts_engine = tts_engine
        # V9.0: Use DequeSpeechQueue instead of queue.Queue
        self.speech_queue = DequeSpeechQueue(maxsize=max_queue_size)
        self.metrics = StreamingMetrics()
        self._shutdown = False
        
        # State machine tokenizer
        self.tokenizer = StreamTokenizer()
        
        # Playback lock for thread safety
        self._playback_lock = threading.Lock()

        # TTS engine reference (set externally)
        self._tts_engine_ref = None
        
        # Start worker thread
        self.worker = threading.Thread(target=self._tts_worker, daemon=True)
        self.worker.start()
        
        self.enabled = True # Gating for audio playback (Patch 4)
        self.current_pitch = 1.0
        self.current_rate = 1.0
        
        logger.info(f"[VoiceSystem] Initialized {tts_engine} with tokenizer=StreamTokenizer (deque backend)")
    
    def set_tts_engine(self, engine):
        """Set the TTS engine reference"""
        self._tts_engine_ref = engine
    
    def _tts_worker(self):
        """Thread-safe worker using DequeSpeechQueue (no task_done race conditions)"""
        while not self._shutdown:
            try:
                chunk = self.speech_queue.get(timeout=WORKER_TIMEOUT_SEC)
                
                if chunk is None:  # Timeout or shutdown
                    continue
                
                # Clean text before TTS
                clean_text = self._clean_for_tts(chunk.text)
                if not clean_text:
                    logger.warning(f"Skipped empty/invalid TTS text: {chunk.text}")
                    continue
                
                # Thread-safe playback via subprocess
                with self._playback_lock:
                    self._subprocess_speak(chunk)
                
                # Update played count
                self.speech_queue.played_total += 1
                
            except Exception as e:
                logger.error(f"TTS worker error: {e}", exc_info=True)
                continue

    def set_max_queue_size(self, new_max: int):
        """Safely resize the queue at runtime (V9.0: deque handles this atomically)"""
        new_max = int(new_max)
        if new_max < QUEUE_SIZE_MIN: new_max = QUEUE_SIZE_MIN
        if new_max > QUEUE_SIZE_MAX: new_max = QUEUE_SIZE_MAX
        
        self.speech_queue.set_maxsize(new_max)
        logger.info(f"[VoiceSystem] Resized queue to {new_max}")
        self.metrics.tts_queue_size = len(self.speech_queue)

    def set_prosody(self, pitch: float = 1.0, rate: float = 1.0):
        """Update current prosody for future chunks"""
        self.current_pitch = pitch
        self.current_rate = rate
        logger.debug(f"[VoiceSystem] Prosody updated: pitch={pitch}, rate={rate}")

    def _clean_for_tts(self, text: str) -> str:
        """Targeted cleaning for TTS"""
        original = text
        for token in self.BAD_TOKENS:
            text = text.replace(token, "")
        text = re.sub(r'\*[^\*]+\*', '', text)
        text = re.sub(r'\[[^\]]+\]', '', text)
        text = re.sub(r'<[^>]+>', '', text)
        text = ' '.join(text.split())
        if len(text) < 2 and not text.isalnum(): return ""
        alpha_count = sum(c.isalpha() for c in text)
        if len(text) > 20 and alpha_count / len(text) < 0.3: return ""
        return text.strip()
    
    def _subprocess_speak(self, chunk: VoiceChunk):
        """Process-safe audio playback"""
        if self._tts_engine_ref is not None:
            try:
                # PiperTTSEngine now handles pitch and rate post-processing
                self._tts_engine_ref.speak_sync(chunk.text, pitch=chunk.pitch, rate=chunk.rate)
            except Exception as e:
                logger.error(f"TTS subprocess error: {e}")
        else:
            logger.info(f"[TTS-MOCK] Would speak: {chunk.text[:50]}... (pitch={chunk.pitch})")

    def speak(self, text: str, npc_id: str = "default", is_final: bool = False, pitch: Optional[float] = None, rate: Optional[float] = None) -> bool:
        """Queue text for TTS (V9.0: drop policy is handled by DequeSpeechQueue)"""
        if not text or self._shutdown or not self.enabled:
            return False
        
        # Use provided prosody or system defaults
        use_pitch = pitch if pitch is not None else self.current_pitch
        use_rate = rate if rate is not None else self.current_rate

        # DequeSpeechQueue handles backpressure internally with its drop policy
        self.speech_queue.put(VoiceChunk(
            text=text,
            npc_id=npc_id,
            created_ts=time.time(),
            is_final=is_final,
            pitch=use_pitch,
            rate=use_rate
        ))
        self.metrics.tts_queue_size = len(self.speech_queue)
        return True
    
    def process_stream(self, token_generator: Generator[str, None, None], pitch: Optional[float] = None, rate: Optional[float] = None) -> Generator[SentenceChunk, None, None]:
        """
        Main streaming pipeline using StreamTokenizer
        """
        start_time = time.time()
        first_token_time = None
        first_sentence_time = None
        
        # Clear tokenizer buffer
        self.tokenizer = StreamTokenizer()
        
        for token in token_generator:
            if first_token_time is None:
                first_token_time = time.time()
                self.metrics.first_token_ms = (first_token_time - start_time) * 1000
            
            # Filter tokens immediately
            if token.strip() in self.BAD_TOKENS or token.strip().startswith('<|'):
                continue
            
            # Process token through state machine
            sentences = self.tokenizer.process(token)
            
            for sentence in sentences:
                if first_sentence_time is None:
                    first_sentence_time = time.time()
                    self.metrics.first_sentence_ms = (first_sentence_time - start_time) * 1000
                
                # Queue with backpressure handling
                # CRITICAL: Cleaning is already done in tokenizer.process()
                # But we ensure we don't send empty chunks
                if sentence.strip():
                    logger.info(f"[TTS-DEBUG] Queueing sentence: '{sentence}' with pitch={pitch}")
                    self.speak(sentence, pitch=pitch, rate=rate)
                
                # Yield for API
                latency = (time.time() - start_time) * 1000
                yield SentenceChunk(text=sentence, latency_ms=latency)
        
        # Handle final fragment
        final_sentences = self.tokenizer.flush()
        for sent in final_sentences:
            self.speak(sent, pitch=pitch, rate=rate)
            yield SentenceChunk(
                text=sent, 
                is_final=True, 
                latency_ms=(time.time() - start_time) * 1000
            )
        
        self.metrics.total_generation_ms = (time.time() - start_time) * 1000
        self.metrics.tts_queue_size = len(self.speech_queue)
    
    def flush_pending(self):
        """Enqueue any remaining buffered text as final sentence(s)."""
        leftovers = self.tokenizer.flush()
        for s in leftovers:
            self.speak(s)
    
    def reset(self):
        """Hard reset: clears queue + tokenizer + metrics. (V9.0: uses deque.clear())"""
        self.tokenizer = StreamTokenizer()
        self.speech_queue.clear()
        self.metrics = StreamingMetrics()
    
    def shutdown(self):
        """Graceful shutdown (V9.0: uses deque.close())"""
        self._shutdown = True
        self.speech_queue.close()
        self.worker.join(timeout=5)
        logger.info("[VoiceSystem] Shutdown complete")


class StreamingMantellaEngine:
    """Streaming LLM wrapper with support for Ollama and llama-cpp backends"""
    
    def __init__(self, model_path: str = None, backend: str = "ollama", ollama_config: dict = None):
        self.model_path = model_path
        self.backend = backend
        self.llm = None
        self.ollama_client = None
        self.voice = StreamingVoiceSystem(tts_engine="kokoro", max_queue_size=3)
        self.temperature = 0.7
        self.max_tokens = 150
        
        # Initialize based on backend
        if backend == "ollama":
            self._init_ollama(ollama_config or {})
        elif backend == "llama_cpp" and model_path and Path(model_path).exists():
            self._load_llama_cpp(model_path)
        else:
            logger.warning(f"Backend: {backend}, Model: {model_path}. Running in mock mode.")
    
    def _init_ollama(self, config: dict):
        """Initialize Ollama client"""
        try:
            from ollama_client import OllamaClient
            
            host = config.get("ollama_host", "http://localhost:11434")
            model = config.get("ollama_model", "llama3.2")
            
            self.ollama_client = OllamaClient(
                host=host,
                model=model,
                temperature=config.get("temperature", 0.7),
                max_tokens=config.get("max_tokens", 150)
            )
            
            if self.ollama_client.is_available():
                logger.info(f"StreamingMantellaEngine: Ollama backend ready (model={model})")
            else:
                logger.warning("Ollama server not available. Start with: ollama serve")
                self.ollama_client = None
        except ImportError:
            logger.warning("ollama_client not found. Running in mock mode.")
            self.ollama_client = None
        except Exception as e:
            logger.error(f"Failed to initialize Ollama: {e}")
            self.ollama_client = None
    
    def _load_llama_cpp(self, model_path: str):
        """Load llama.cpp model with KV cache persistence"""
        try:
            from llama_cpp import Llama
            self.llm = Llama(
                model_path=model_path,
                n_ctx=2048,
                n_gpu_layers=-1, 
                verbose=False,
            )
            logger.info(f"StreamingMantellaEngine loaded: {Path(model_path).name}")
        except ImportError:
            logger.warning("llama-cpp-python not installed. Running in mock mode.")
        except Exception as e:
            logger.error(f"Failed to load model: {e}")
    
    def generate_streaming(self, prompt: str, max_tokens: int = 512, temperature: float = 0.7, pitch: Optional[float] = None, rate: Optional[float] = None) -> Generator[SentenceChunk, None, None]:
        """Generate with streaming and full metrics"""
        
        # Try Ollama first
        if self.ollama_client is not None:
            def ollama_token_gen():
                stop_sequences = [
                    "<|eot_id|>", "<|end|>", "</s>", 
                    "\nPlayer:", "\nUser:", "\nYou:", "\nNPC:", "Player:", "User:",
                    "\nSystem:", "System:", "[SYSTEM MODE:", "**System:"
                ]
                for token in self.ollama_client.generate_streaming(
                    prompt,
                    max_tokens=max_tokens,
                    temperature=temperature,
                    stop=stop_sequences
                ):
                    yield token
            
            yield from self.voice.process_stream(ollama_token_gen(), pitch=pitch, rate=rate)
            return
        
        # Fallback to llama-cpp
        if self.llm is not None:
            def llama_token_gen():
                stream = self.llm(
                    prompt,
                    max_tokens=max_tokens,
                    stop=["<|eot_id|>", "<|end|>", "</s>", 
                        "\nPlayer:", "\nUser:", "\nYou:", "\nNPC:", "Player:", "User:",
                        "\nSystem:", "System:", "[SYSTEM MODE:", "**System:"],
                    stream=True,
                    echo=False,
                    temperature=temperature,
                )
                for chunk in stream:
                    if 'choices' in chunk and chunk['choices']:
                        text = chunk['choices'][0].get('text', '')
                        if text:
                            yield text
            
            yield from self.voice.process_stream(llama_token_gen(), pitch=pitch, rate=rate)
            return
        
        # Mock mode
        def mock_tokens():
            text = "I am an NPC running in mock mode. Please start Ollama with: ollama serve"
            for word in text.split(" "):
                yield word + " "
                time.sleep(0.05)
            yield ""
        yield from self.voice.process_stream(mock_tokens(), pitch=pitch, rate=rate)

    def apply_tuning(self, *, temperature: float = None, max_tokens: int = None, max_queue_size: int = None):
        """Apply runtime performance settings"""
        if temperature is not None:
            self.temperature = float(temperature)
            if self.ollama_client:
                self.ollama_client.temperature = float(temperature)
        if max_tokens is not None:
            self.max_tokens = int(max_tokens)
            if self.ollama_client:
                self.ollama_client.max_tokens = int(max_tokens)
        if max_queue_size is not None:
            self.voice.set_max_queue_size(int(max_queue_size))
        
        logger.info(f"Tuning applied: temp={self.temperature}, tokens={self.max_tokens}")
    
    def shutdown(self):
        """Cleanup resources"""
        self.voice.shutdown()
        if self.ollama_client:
            self.ollama_client.shutdown()
        logger.info("StreamingMantellaEngine shutdown complete")


if __name__ == "__main__":
    # Quick test
    engine = StreamingMantellaEngine(backend="ollama")
    print("Testing streaming engine...")
    
    for chunk in engine.generate_streaming("Hello"):
        print(f"Chunk: {chunk.text} (final={chunk.is_final}, latency={chunk.latency_ms:.0f}ms)")
    
    engine.shutdown()
    print("Test complete!")

