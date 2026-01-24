#!/usr/bin/env python3
"""
RFSN Latency Optimizations Module
Implements Gemini's recommendations:
- Instant Barks (latency masking)
- Pipeline Reordering (async action emission)  
- User-Centric Rewards (fix echo chamber)
- Aggressive Context Pruning
"""

import asyncio
import json
import logging
import time
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional, Callable, Any
import re

logger = logging.getLogger("rfsn.optimizations")

# ─────────────────────────────────────────────────────────────
# 1. INSTANT BARKS - Latency Masking System
# ─────────────────────────────────────────────────────────────

class BarkCategory(Enum):
    """Categories for instant barks"""
    GREET = "greet"
    THREATEN = "threaten"
    AGREE = "agree"
    DISAGREE = "disagree"
    QUESTION = "question"
    HELP = "help"
    TRADE = "trade"
    FAREWELL = "farewell"
    IDLE = "idle"
    COMBAT = "combat"
    SURPRISE = "surprise"
    GRATEFUL = "grateful"


@dataclass
class InstantBark:
    """A pre-recorded bark for instant playback"""
    text: str
    audio_path: Optional[str] = None
    duration_ms: int = 500


# Default barks per category - can be overridden per NPC
DEFAULT_BARKS: Dict[BarkCategory, List[InstantBark]] = {
    BarkCategory.GREET: [
        InstantBark("Hey there!", duration_ms=400),
        InstantBark("Well, hello!", duration_ms=450),
        InstantBark("Ah, you again.", duration_ms=500),
    ],
    BarkCategory.THREATEN: [
        InstantBark("You asked for it!", duration_ms=600),
        InstantBark("Don't test me.", duration_ms=500),
        InstantBark("I'm warning you.", duration_ms=550),
    ],
    BarkCategory.AGREE: [
        InstantBark("Alright then.", duration_ms=400),
        InstantBark("Fair enough.", duration_ms=400),
        InstantBark("You got it.", duration_ms=350),
    ],
    BarkCategory.DISAGREE: [
        InstantBark("I don't think so.", duration_ms=500),
        InstantBark("No way.", duration_ms=300),
        InstantBark("Not a chance.", duration_ms=450),
    ],
    BarkCategory.QUESTION: [
        InstantBark("Hmm, let me think...", duration_ms=600),
        InstantBark("Good question.", duration_ms=400),
        InstantBark("Well...", duration_ms=300),
    ],
    BarkCategory.HELP: [
        InstantBark("Of course!", duration_ms=350),
        InstantBark("I can help with that.", duration_ms=600),
        InstantBark("Let's see...", duration_ms=400),
    ],
    BarkCategory.TRADE: [
        InstantBark("Looking to trade?", duration_ms=500),
        InstantBark("Let's see what you've got.", duration_ms=600),
        InstantBark("Business, eh?", duration_ms=400),
    ],
    BarkCategory.FAREWELL: [
        InstantBark("Take care.", duration_ms=350),
        InstantBark("Until next time.", duration_ms=450),
        InstantBark("Safe travels.", duration_ms=400),
    ],
    BarkCategory.IDLE: [
        InstantBark("Hmm.", duration_ms=200),
        InstantBark("...", duration_ms=100),
    ],
    BarkCategory.COMBAT: [
        InstantBark("Die!", duration_ms=250),
        InstantBark("Take that!", duration_ms=300),
        InstantBark("You'll regret this!", duration_ms=500),
    ],
    BarkCategory.SURPRISE: [
        InstantBark("What the—", duration_ms=350),
        InstantBark("Whoa!", duration_ms=250),
        InstantBark("Huh?", duration_ms=200),
    ],
    BarkCategory.GRATEFUL: [
        InstantBark("Thanks!", duration_ms=300),
        InstantBark("Much appreciated.", duration_ms=450),
        InstantBark("You're too kind.", duration_ms=450),
    ],
}


class InstantBarkSystem:
    """
    Manages instant bark playback for latency masking.
    
    When an action is determined, this fires immediately
    while the LLM generates the full response.
    """
    
    def __init__(self, custom_barks: Optional[Dict[str, Dict[BarkCategory, List[InstantBark]]]] = None):
        self.npc_barks: Dict[str, Dict[BarkCategory, List[InstantBark]]] = custom_barks or {}
        self._bark_index: Dict[str, Dict[BarkCategory, int]] = {}
    
    def get_bark(self, npc_name: str, category: BarkCategory) -> InstantBark:
        """Get the next bark for an NPC in a category (round-robin)"""
        # Use NPC-specific barks or fall back to defaults
        barks = self.npc_barks.get(npc_name, {}).get(category, DEFAULT_BARKS.get(category, []))
        
        if not barks:
            return InstantBark("...", duration_ms=100)
        
        # Round-robin selection
        key = f"{npc_name}:{category.value}"
        if key not in self._bark_index:
            self._bark_index[key] = 0
        
        idx = self._bark_index[key]
        bark = barks[idx % len(barks)]
        self._bark_index[key] = (idx + 1) % len(barks)
        
        return bark
    
    def action_to_category(self, action: str) -> BarkCategory:
        """Map NPC action to bark category"""
        action_map = {
            "greet": BarkCategory.GREET,
            "threaten": BarkCategory.THREATEN,
            "attack": BarkCategory.COMBAT,
            "agree": BarkCategory.AGREE,
            "disagree": BarkCategory.DISAGREE,
            "help": BarkCategory.HELP,
            "trade": BarkCategory.TRADE,
            "offer": BarkCategory.TRADE,
            "farewell": BarkCategory.FAREWELL,
            "flee": BarkCategory.SURPRISE,
            "apologize": BarkCategory.GRATEFUL,
            "accept": BarkCategory.AGREE,
            "refuse": BarkCategory.DISAGREE,
        }
        return action_map.get(action.lower(), BarkCategory.IDLE)


# ─────────────────────────────────────────────────────────────
# 2. USER-CENTRIC REWARD SYSTEM  
# ─────────────────────────────────────────────────────────────

@dataclass
class UserSentimentResult:
    """Result of analyzing user sentiment"""
    sentiment: float  # -1.0 to 1.0
    confidence: float  # 0.0 to 1.0
    is_explicit_negative: bool = False
    triggers_found: List[str] = field(default_factory=list)


class UserCentricRewardSystem:
    """
    Computes rewards based on USER's reaction, not NPC's emotion.
    Fixes the "echo chamber" problem identified by Gemini.
    """
    
    # Explicit negative triggers - hard punish
    NEGATIVE_TRIGGERS = [
        "shut up", "stop", "bad bot", "ignore me", "go away",
        "you're annoying", "useless", "stupid npc", "broken",
        "doesn't work", "hate you", "worst", "terrible"
    ]
    
    # Positive triggers
    POSITIVE_TRIGGERS = [
        "thanks", "thank you", "helpful", "good job", "nice",
        "perfect", "exactly", "great", "love it", "amazing",
        "well done", "that's right", "correct"
    ]
    
    # Neutral/continuation signals (not rewarded positively)
    CONTINUATION_SIGNALS = [
        "ok", "okay", "i see", "hmm", "interesting", "go on",
        "what else", "and then"
    ]
    
    def __init__(self, llm_judge: Optional[Callable[[str], float]] = None):
        """
        Args:
            llm_judge: Optional async function that uses LLM to rate sentiment.
                      Called asynchronously after the turn completes.
        """
        self.llm_judge = llm_judge
        self._judge_cache: Dict[str, float] = {}
    
    def analyze_user_input(self, user_input: str) -> UserSentimentResult:
        """
        Analyze user input for sentiment.
        This is the PRIMARY reward source.
        """
        text = user_input.lower().strip()
        triggers_found = []
        
        # Check for explicit negatives first (hard punish)
        for trigger in self.NEGATIVE_TRIGGERS:
            if trigger in text:
                triggers_found.append(f"negative:{trigger}")
                return UserSentimentResult(
                    sentiment=-1.0,
                    confidence=1.0,
                    is_explicit_negative=True,
                    triggers_found=triggers_found
                )
        
        # Check for positives
        positive_count = 0
        for trigger in self.POSITIVE_TRIGGERS:
            if trigger in text:
                positive_count += 1
                triggers_found.append(f"positive:{trigger}")
        
        if positive_count > 0:
            # Scale by number of positive triggers, max 1.0
            sentiment = min(1.0, positive_count * 0.5)
            return UserSentimentResult(
                sentiment=sentiment,
                confidence=0.8,
                triggers_found=triggers_found
            )
        
        # Check for continuation (neutral, not a positive signal)
        for signal in self.CONTINUATION_SIGNALS:
            if signal in text:
                triggers_found.append(f"continuation:{signal}")
                return UserSentimentResult(
                    sentiment=0.0,  # Neutral, not positive!
                    confidence=0.5,
                    triggers_found=triggers_found
                )
        
        # No clear signal - slight positive for engagement
        return UserSentimentResult(
            sentiment=0.1,
            confidence=0.3,
            triggers_found=triggers_found
        )
    
    def compute_reward(
        self,
        user_input: str,
        previous_npc_action: str,
        conversation_continued: bool
    ) -> float:
        """
        Compute reward based on user's reaction.
        
        Args:
            user_input: What the user just said
            previous_npc_action: What action the NPC took last turn
            conversation_continued: Did the user continue talking?
        
        Returns:
            Reward value from -1.0 to 1.0
        """
        result = self.analyze_user_input(user_input)
        
        # Hard punish for explicit negatives
        if result.is_explicit_negative:
            logger.warning(f"User explicit negative: {result.triggers_found}")
            return -1.0
        
        # Use sentiment as base reward
        reward = result.sentiment
        
        # Slight penalty if conversation did NOT continue (user walked away)
        if not conversation_continued:
            reward -= 0.3
        
        # Clamp
        return max(-1.0, min(1.0, reward))


# ─────────────────────────────────────────────────────────────
# 3. PIPELINE REORDERING - Async Action Emission
# ─────────────────────────────────────────────────────────────

@dataclass
class PipelineStage:
    """A stage in the optimized pipeline"""
    name: str
    start_time: float = 0.0
    end_time: float = 0.0
    
    @property
    def duration_ms(self) -> float:
        return (self.end_time - self.start_time) * 1000


class OptimizedPipeline:
    """
    Reordered pipeline that emits action IMMEDIATELY,
    then does heavy lifting (memory retrieval, prompt building)
    while the client plays the instant bark.
    """
    
    def __init__(
        self,
        action_scorer,
        memory_governance,
        bark_system: InstantBarkSystem,
        context_limit: int = 4  # Aggressive pruning per Gemini
    ):
        self.action_scorer = action_scorer
        self.memory_governance = memory_governance  
        self.bark_system = bark_system
        self.context_limit = context_limit
        self.stages: List[PipelineStage] = []
    
    async def process_request(
        self,
        user_input: str,
        npc_state: dict,
        player_signal: str,
        memory_manager,
        npc_name: str
    ):
        """
        Process request with optimized pipeline ordering.
        
        Yields events in order:
        1. metadata (with action) - IMMEDIATELY
        2. bark - IMMEDIATELY after action
        3. sentences - as LLM generates them
        """
        request_start = time.time()
        
        # ─────────────────────────────────────────────
        # STAGE 1: Score Action (FAST - 50ms)
        # ─────────────────────────────────────────────
        stage = PipelineStage(name="action_scoring", start_time=time.time())
        
        selected_action = None
        if self.action_scorer:
            try:
                state_snapshot = self.action_scorer.create_snapshot(npc_state)
                action_scores = self.action_scorer.score_actions(state_snapshot, player_signal)
                selected_action = max(action_scores, key=action_scores.get)
            except Exception as e:
                logger.error(f"Action scoring failed: {e}")
                selected_action = "talk"  # Safe default
        
        stage.end_time = time.time()
        self.stages.append(stage)
        
        # ─────────────────────────────────────────────
        # YIELD IMMEDIATELY - Client triggers bark
        # ─────────────────────────────────────────────
        bark = self.bark_system.get_bark(
            npc_name, 
            self.bark_system.action_to_category(selected_action or "idle")
        )
        
        # Metadata event with action
        metadata_event = {
            "type": "meta",
            "npc_action": selected_action,
            "player_signal": player_signal,
            "instant_bark": bark.text,
            "bark_duration_ms": bark.duration_ms,
            "bark_audio_path": bark.audio_path,
        }
        yield f"data: {json.dumps(metadata_event)}\n\n"
        
        # ─────────────────────────────────────────────
        # STAGE 2: Memory & Context (SLOW - runs in parallel with bark)
        # ─────────────────────────────────────────────
        stage = PipelineStage(name="memory_retrieval", start_time=time.time())
        
        # Create tasks for parallel execution
        async def retrieve_memory():
            if self.memory_governance and user_input:
                return self.memory_governance.semantic_search(
                    user_input, k=3, min_score=0.4
                )
            return []
        
        async def get_history():
            if memory_manager:
                return memory_manager.get_context_window(limit=self.context_limit)
            return ""
        
        # Run in parallel while client plays bark
        memory_task = asyncio.create_task(retrieve_memory())
        history_task = asyncio.create_task(get_history())
        
        relevant_memories, history = await asyncio.gather(memory_task, history_task)
        
        stage.end_time = time.time()
        self.stages.append(stage)
        
        # ─────────────────────────────────────────────
        # STAGE 3: Build Prompt (FAST - 10ms)
        # ─────────────────────────────────────────────
        stage = PipelineStage(name="prompt_building", start_time=time.time())
        
        semantic_context = ""
        if relevant_memories:
            semantic_context = "Relevant Facts:\n" + "\n".join(
                [f"- {m[0].content}" for m in relevant_memories]
            ) + "\n"
        
        # Build compact prompt
        full_prompt = self._build_prompt(
            npc_state=npc_state,
            history=history,
            semantic_context=semantic_context,
            user_input=user_input,
            selected_action=selected_action
        )
        
        stage.end_time = time.time()
        self.stages.append(stage)
        
        # Return prompt for LLM generation
        yield {
            "type": "prompt_ready",
            "prompt": full_prompt,
            "action": selected_action,
            "pipeline_timing": {
                s.name: s.duration_ms for s in self.stages
            }
        }
    
    def _build_prompt(
        self,
        npc_state: dict,
        history: str,
        semantic_context: str,
        user_input: str,
        selected_action: str
    ) -> str:
        """Build the LLM prompt with aggressive context pruning"""
        npc_name = npc_state.get("npc_name", "NPC")
        mood = npc_state.get("mood", "neutral")
        relationship = npc_state.get("relationship", "stranger")
        
        # Compact system prompt
        system_prompt = f"""You are {npc_name}. Mood: {mood}. Relationship: {relationship}.
You MUST perform the action: {selected_action.upper()}.
Keep your response under 2 sentences. Be natural and in-character.
{semantic_context}"""

        # Minimal history (aggressively pruned)
        if history:
            # Only keep last few exchanges
            lines = history.strip().split('\n')[-6:]  # Max 3 exchanges
            history = '\n'.join(lines)
        
        prompt = f"System: {system_prompt}\n{history}\nPlayer: {user_input}\n{npc_name}:"
        
        return prompt


# ─────────────────────────────────────────────────────────────
# 4. CLAUSE-BASED STREAM TOKENIZER
# ─────────────────────────────────────────────────────────────

class ClauseTokenizer:
    """
    Relaxed tokenizer that splits on clauses ("breath groups")
    rather than just full sentences.
    
    This allows TTS to start speaking sooner.
    """
    
    # Break on these if buffer is long enough
    CLAUSE_BREAKS = [
        ', and ', ', but ', ', so ', ', yet ', ', or ',
        '; ', '— ', ' - ', '... ',
    ]
    
    # Always break on these (full sentences)
    SENTENCE_BREAKS = ['. ', '! ', '? ', '.\n', '!\n', '?\n']
    
    # Minimum words before clause break is allowed
    MIN_WORDS_FOR_CLAUSE_BREAK = 8
    
    def __init__(self):
        self.buffer = ""
        self._abbreviations = {
            'mr.', 'mrs.', 'ms.', 'dr.', 'prof.', 'sr.', 'jr.',
            'st.', 'vs.', 'etc.', 'i.e.', 'e.g.', 'no.', 'vol.'
        }
    
    def feed(self, chunk: str) -> List[str]:
        """
        Feed a chunk of text, return any complete clauses/sentences.
        """
        self.buffer += chunk
        results = []
        
        while True:
            # Check for sentence breaks first
            best_break = None
            best_pos = len(self.buffer)
            
            for terminator in self.SENTENCE_BREAKS:
                pos = self.buffer.find(terminator)
                if pos != -1 and pos < best_pos:
                    # Check for abbreviations
                    pre_text = self.buffer[:pos + 1].lower()
                    is_abbrev = any(pre_text.endswith(abbr) for abbr in self._abbreviations)
                    if not is_abbrev:
                        best_break = terminator
                        best_pos = pos
            
            # If no sentence break and buffer is long, try clause break
            word_count = len(self.buffer.split())
            if best_break is None and word_count >= self.MIN_WORDS_FOR_CLAUSE_BREAK:
                for terminator in self.CLAUSE_BREAKS:
                    pos = self.buffer.find(terminator)
                    if pos != -1 and pos < best_pos:
                        # Only break if we have enough words before the break
                        pre_words = len(self.buffer[:pos].split())
                        if pre_words >= self.MIN_WORDS_FOR_CLAUSE_BREAK // 2:
                            best_break = terminator
                            best_pos = pos
            
            if best_break is not None:
                # Extract the clause/sentence
                clause = self.buffer[:best_pos + len(best_break)].strip()
                self.buffer = self.buffer[best_pos + len(best_break):]
                if clause:
                    results.append(clause)
            else:
                break
        
        return results
    
    def flush(self) -> Optional[str]:
        """Flush any remaining buffer content"""
        if self.buffer.strip():
            result = self.buffer.strip()
            self.buffer = ""
            return result
        return None


# ─────────────────────────────────────────────────────────────
# 5. INTEGRATION HELPERS
# ─────────────────────────────────────────────────────────────

def create_optimized_pipeline(
    action_scorer,
    memory_governance,
    context_limit: int = 4
) -> OptimizedPipeline:
    """Factory function to create an optimized pipeline"""
    bark_system = InstantBarkSystem()
    return OptimizedPipeline(
        action_scorer=action_scorer,
        memory_governance=memory_governance,
        bark_system=bark_system,
        context_limit=context_limit
    )


def create_user_reward_system(llm_judge=None) -> UserCentricRewardSystem:
    """Factory function to create user-centric reward system"""
    return UserCentricRewardSystem(llm_judge=llm_judge)


# ─────────────────────────────────────────────────────────────
# Example usage
# ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    # Test instant barks
    bark_system = InstantBarkSystem()
    bark = bark_system.get_bark("Guard", BarkCategory.THREATEN)
    print(f"Bark: '{bark.text}' ({bark.duration_ms}ms)")
    
    # Test user sentiment
    reward_system = UserCentricRewardSystem()
    
    test_inputs = [
        "Thanks, that was helpful!",
        "Shut up, you're annoying",
        "Ok, I see",
        "What else?",
        "You're the best!"
    ]
    
    for inp in test_inputs:
        result = reward_system.analyze_user_input(inp)
        print(f"'{inp}' -> sentiment={result.sentiment:.2f}, conf={result.confidence:.2f}")
    
    # Test clause tokenizer
    tokenizer = ClauseTokenizer()
    
    test_text = "Hello there, and welcome to my shop. I have many fine wares, but first let me tell you about our special today! It's truly amazing."
    
    # Simulate streaming
    for i in range(0, len(test_text), 10):
        chunk = test_text[i:i+10]
        clauses = tokenizer.feed(chunk)
        for clause in clauses:
            print(f"  -> Clause: '{clause}'")
    
    # Flush remaining
    remaining = tokenizer.flush()
    if remaining:
        print(f"  -> Final: '{remaining}'")
