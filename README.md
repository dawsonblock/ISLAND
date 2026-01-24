<div align="center">

# 🏝️ ISLAND

### Autonomous NPC Intelligence for Unreal Engine 5.5

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5-0E1128?style=for-the-badge&logo=unrealengine)](https://unrealengine.com)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![Python](https://img.shields.io/badge/Python-3.10+-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://python.org)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

**A bounded decision system where LLMs are deliberately demoted to renderers.**

[Features](#-features) • [Architecture](#-architecture) • [Quick Start](#-quick-start) • [Documentation](#-documentation) • [API Reference](#-api-reference)

---

</div>

## 🎯 What This Is

ISLAND is **not** "an NPC with an LLM." It's a **bounded decision system** where:

- **Action selection happens before language** — the LLM never invents goals
- **State drives intent** — mood, relationship, and affinity determine behavior
- **Learning is scoped and reversible** — per-state isolation, bounded rewards, explicit bans
- **The Unity contract is practical** — observation in, decision out, execution report back

This architecture puts ISLAND ahead of most "AI NPC" implementations by separating reasoning from speech generation.

---

## ✨ Features

<table>
<tr>
<td width="50%">

### 🧠 Core Intelligence

- **RFSN Integration** — Local LLM + TTS orchestrator
- **Bandit Learner** — UCB1-based behavioral learning
- **Temporal Memory** — Anticipatory context scoring
- **Expanded Action Lattice** — Nuanced intent modifiers
- **Emotion Blending** — VAD emotion model with transitions
- **Procedural Backstories** — LLM-generated NPC histories

</td>
<td width="50%">

### 🎮 Game Systems

- **Faction System** — Group reputation with propagation
- **Relationship Persistence** — SaveGame integration
- **NPC Conversations** — Multi-NPC dialogue
- **Director Control** — Pacing and tension management

</td>
</tr>
<tr>
<td>

### 🎨 Presentation

- **Dialogue Camera** — Focus, over-shoulder, two-shot
- **NPC Look-At** — Smooth rotation to player
- **Facial Animation** — Emotion-driven morph targets
- **Audio Attenuation** — Distance + occlusion
- **Ambient Chatter** — Idle contextual dialogue

</td>
<td>

### 🛠️ Developer Tools

- **Console Commands** — 10 debug commands
- **Blueprint Library** — Static helper functions
- **Mock Server** — Offline testing
- **Performance Metrics** — Latency tracking
- **NPC Config Asset** — Editor-based NPC setup

</td>
</tr>
</table>

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         GAME LAYER                              │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────┐ │
│  │ Player Input │ │ NPC Trigger  │ │ Director Bridge          │ │
│  └──────┬───────┘ └──────┬───────┘ └────────────┬─────────────┘ │
└─────────┼────────────────┼──────────────────────┼───────────────┘
          │                │                      │
          ▼                ▼                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                      DECISION LAYER                             │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Temporal Memory                          │ │
│  │   State-Action-Outcome Traces → Context Similarity Bias   │ │
│  └───────────────────────────┬────────────────────────────────┘ │
│                              │                                  │
│  ┌───────────────────────────▼────────────────────────────────┐ │
│  │                   Action Lattice                           │ │
│  │   Base Action + Intensity + Compliance + Motive → Hint    │ │
│  └───────────────────────────┬────────────────────────────────┘ │
│                              │                                  │
│  ┌───────────────────────────▼────────────────────────────────┐ │
│  │                   Bandit Selector                          │ │
│  │   UCB1 Scoring + Bias Application → Selected Action       │ │
│  └───────────────────────────┬────────────────────────────────┘ │
└──────────────────────────────┼──────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      RENDER LAYER (LLM)                         │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │   Action Hint + NPC State + Context → Natural Language    │ │
│  │   (LLM cannot invent goals, shift tone, or leak structure) │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Action before Language** | LLM renders intent, doesn't decide it |
| **Per-state Bandit Isolation** | Learning doesn't leak across contexts |
| **Bounded Rewards** | Prevents runaway optimization |
| **Temporal Memory** | "This feels like last time" anticipation |
| **Expanded Action Lattice** | Nuance via action space, not prompts |

---

## 🚀 Quick Start

### Prerequisites

- Unreal Engine 5.5+
- Python 3.10+ (for RFSN server)
- [Ollama](https://ollama.ai) with `llama3.2`

### 1. Clone

```bash
git clone https://github.com/dawsonblock/ISLAND.git
cd ISLAND
```

### 2. Start RFSN Server

```bash
cd RFSN_NPC_AI/Python
pip install -r requirements.txt
python orchestrator.py
```

Or use the mock server for offline testing:

```bash
python mock_server.py
```

### 3. Open in Unreal

```
1. Open MyProject.uproject
2. Build (Ctrl+Shift+B)
3. Place RfsnSampleMerchant or RfsnSampleGuard in level
4. Create IA_Dialogue input action
5. Play and interact
```

---

## 📖 Documentation

### Core Components

| Component | Type | Description |
|-----------|------|-------------|
| `URfsnNpcClientComponent` | ActorComponent | HTTP SSE client for RFSN |
| `URfsnDialogueManager` | WorldSubsystem | Manages active dialogues |
| `URfsnTemporalMemory` | ActorComponent | State-action-outcome memory |
| `URfsnActionLattice` | Static Library | Expanded action construction |
| `URfsnEmotionBlend` | ActorComponent | VAD emotion model with facial animation |
| `URfsnBackstoryGenerator` | ActorComponent | LLM-driven procedural backstories |

### Subsystems

| Subsystem | Scope | Description |
|-----------|-------|-------------|
| `URfsnDialogueManager` | World | Active dialogue management |
| `URfsnRelationshipManager` | GameInstance | Persistent relationships |
| `URfsnFactionSystem` | GameInstance | Group reputation |
| `URfsnNpcConversation` | World | NPC-to-NPC dialogue |
| `URfsnHttpPool` | GameInstance | Connection pooling |
| `URfsnMetrics` | GameInstance | Performance tracking |

### Console Commands

```
RfsnDebug         Toggle debug HUD
RfsnTalk          Talk to nearest NPC
RfsnSay <msg>     Send custom message
RfsnListNpcs      List all RFSN NPCs
RfsnSetMood       Change NPC mood
RfsnMockMode      Toggle offline mode
RfsnDumpLog       Show conversation history
RfsnClearLog      Clear history
RfsnServerStatus  Check server connection
RfsnReload        Reload NPC configs
```

---

## 📚 API Reference

### Temporal Memory

```cpp
// Record outcome after player response
Memory->RecordOutcome(Action, Outcome, Mood, Relationship, Affinity, PlayerSignal);

// Get biases before selection
TArray<FRfsnActionBias> Biases = Memory->GetActionBiases(Mood, Rel, Affinity, Signal);

// Check for hesitation trigger
if (Memory->HasNegativeMemory(Mood, Rel, Affinity)) {
    // Apply caution
}
```

### Action Lattice

```cpp
// Build nuanced action
FRfsnExpandedAction Action = URfsnActionLattice::BuildAction(
    BaseAction, Affinity, Bias, bHasNegativeMemory);

// Or use factory methods
FRfsnExpandedAction::Hesitant(ERfsnNpcAction::Help);
FRfsnExpandedAction::Reluctant(ERfsnNpcAction::Trade);
FRfsnExpandedAction::Conflicted(ERfsnNpcAction::Help, ERfsnNpcAction::Attack);

// Render for LLM
FString Hint = Action.ToPromptHint();
// → "hesitantly Help, only partially (guarded)"
```

### Faction System

```cpp
// Get/modify reputation
float Rep = FactionSys->GetReputation("bandits");
FactionSys->ModifyReputation("bandits", -10.0f); // Also affects allies/enemies

// Check tier
FString Tier = FactionSys->GetReputationTier("bandits");
// → "Hostile", "Unfriendly", "Neutral", "Friendly", "Allied"
```

### NPC Conversation

```cpp
// Start NPC-to-NPC dialogue
FString ConvId = ConvMgr->StartDialogue(NpcA, NpcB, "the weather");

// Group discussion
ConvMgr->StartGroupDiscussion({Npc1, Npc2, Npc3}, "survival plans");

// Player interrupts
ConvMgr->PlayerJoinConversation(ConvId);
```

### Emotion Blending

```cpp
// Apply emotional stimulus
EmotionBlend->ApplyStimulus(TEXT("Joy"), 0.8f);
EmotionBlend->ApplyStimulusEnum(ERfsnCoreEmotion::Fear, 0.5f);

// Get dialogue tone for LLM
FString Tone = EmotionBlend->ToDialogueTone();
// → "warm, energetic, assertive"

// Get mood string for prompts
FString Mood = EmotionBlend->ToMoodString();
// → "Intensely Joy"

// Apply to mesh morph targets
EmotionBlend->ApplyToSkeletalMesh(SkeletalMeshComponent);

// Get facial expression weights
FRfsnFacialExpression Expr = EmotionBlend->GetFacialExpression();
// Expr.Joy, Expr.Anger, Expr.Fear, etc.
```

### Procedural Backstory

```cpp
// Backstory generates automatically on first dialogue interaction
// Or trigger manually:
BackstoryGen->GenerateBackstory();

// Get context for LLM prompts
FString Context = BackstoryGen->GetDialogueContext();
// → "Background: Marcus grew up on the..."

// Get short context
FString Short = BackstoryGen->GetShortContext();
// → "A Trader who is known for being cautious."

// Check if backstory exists
if (BackstoryGen->HasBackstory()) {
    FString Goal = BackstoryGen->CachedBackstory.PersonalGoal;
}
```

---

## 🗂️ Project Structure

```
ISLAND/
├── Source/MyProject/
│   ├── Public/                     # Headers
│   │   ├── RfsnNpcClientComponent.h
│   │   ├── RfsnTemporalMemory.h
│   │   ├── RfsnActionLattice.h
│   │   ├── RfsnFactionSystem.h
│   │   └── ... (40+ headers)
│   ├── Private/                    # Implementations
│   └── MyProjectPCH.h              # Shared PCH
├── RFSN_NPC_AI/
│   └── Python/
│       ├── orchestrator.py         # Main server
│       └── mock_server.py          # Offline testing
├── Content/                        # UE assets
├── SETUP_INSTRUCTIONS.md           # Detailed setup guide
├── RFSN_BLUEPRINT_GUIDE.md         # Blueprint integration
└── README.md                       # This file
```

---

## 📊 Statistics

| Metric | Count |
|--------|-------|
| C++ Classes | 48+ |
| Subsystems | 7 |
| Console Commands | 10 |
| Default Factions | 5 |
| Core Emotions | 8 |
| Sample NPCs | 2 |
| Lines of Code | ~12,000+ |

---

## 🔧 Build Optimization

The project includes aggressive compile-time optimizations:

- **Unity Builds** — 2-3x faster full rebuilds
- **Shared PCH** — Common headers pre-compiled
- **Forward Declarations** — Minimal header parsing
- **IWYU Enforcement** — Clean include graphs
- **Shipping-only Optimization** — Fast iteration in dev

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing`)
5. Open a Pull Request

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.

---

<div align="center">

**Built with 🏝️ by [dawsonblock](https://github.com/dawsonblock)**

</div>
