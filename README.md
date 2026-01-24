<div align="center">

# 🏝️ ISLAND

### Autonomous NPC Intelligence for Unreal Engine 5.5

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5-0E1128?style=for-the-badge&logo=unrealengine)](https://unrealengine.com)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![Python](https://img.shields.io/badge/Python-3.10+-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://python.org)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

**A bounded decision system where LLMs are deliberately demoted to renderers.**

[Features](#-features) • [Architecture](#-architecture) • [Quick Start](#-quick-start) • [Components](#-component-reference) • [API](#-api-reference)

---

</div>

## 🎯 What This Is

ISLAND is **not** "an NPC with an LLM." It's a **bounded decision system** where:

- **Action selection happens before language** — the LLM never invents goals
- **State drives intent** — mood, relationship, and affinity determine behavior
- **Learning is scoped and reversible** — per-state isolation, bounded rewards, explicit bans
- **75+ C++ components** — comprehensive NPC AI framework

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

### 🎭 Social Systems

- **Witness System** — NPCs observe and gossip about player
- **Group Conversations** — Multi-NPC dialogue with turns
- **NPC Memory** — Persistent conversation tracking
- **Relationship Decay** — Time-based standing changes
- **NPC Barks** — Context-aware one-liners

</td>
</tr>
<tr>
<td>

### 🎮 Game Systems

- **Faction System** — Group reputation with propagation
- **Quest Integration** — Quest-aware NPC dialogue
- **Dynamic Pricing** — Reputation-based merchant prices
- **NPC Schedules** — Daily routines and patrol routes
- **NPC Needs** — Hunger, energy, social needs

</td>
<td>

### 🌍 Environment & Presentation

- **Weather Reactions** — NPCs react to environment
- **Lip Sync** — Audio-driven facial animation
- **Dialogue Camera** — Focus, over-shoulder, two-shot
- **NPC Portraits** — Character cards with faction colors
- **Reputation HUD** — Visual faction standings

</td>
</tr>
<tr>
<td>

### 🔒 Behavior & Stealth

- **NPC Awareness** — Detection, FOV, hearing
- **Emotional Contagion** — NPCs influence each other's moods
- **Voice Modulation** — Emotion-driven TTS parameters
- **Emotion Persistence** — Save/load emotional states

</td>
<td>

### 🛠️ Developer Tools

- **Console Commands** — 10+ debug commands
- **Blueprint Library** — Static helper functions
- **Mock Server** — Offline testing
- **Forward Declarations** — Optimized compilation
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
│  │   Temporal Memory + Witness System + NPC Memory            │ │
│  └───────────────────────────┬────────────────────────────────┘ │
│  ┌───────────────────────────▼────────────────────────────────┐ │
│  │   Action Lattice + Needs + Schedule + Awareness            │ │
│  └───────────────────────────┬────────────────────────────────┘ │
│  ┌───────────────────────────▼────────────────────────────────┐ │
│  │   Bandit Selector + Emotion Blend + Relationship Decay    │ │
│  └───────────────────────────┬────────────────────────────────┘ │
└──────────────────────────────┼──────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      RENDER LAYER (LLM)                         │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │   Action Hint + NPC State + Context → Natural Language    │ │
│  │   + Voice Modulation + Lip Sync + Facial Animation        │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start

### Prerequisites

- Unreal Engine 5.5+
- Python 3.10+ (for RFSN server)
- [Ollama](https://ollama.ai) with `llama3.2`

### 1. Clone & Setup

```bash
git clone https://github.com/dawsonblock/ISLAND.git
cd ISLAND
cd RFSN_NPC_AI/Python
pip install -r requirements.txt
python orchestrator.py  # Or: python mock_server.py
```

### 2. Open in Unreal

1. Open `MyProject.uproject`
2. Build (Ctrl+Shift+B)
3. Place sample NPCs in level
4. Play and interact (E key)

---

## 📦 Component Reference

### Core Components

| Component | Description |
|-----------|-------------|
| `URfsnNpcClientComponent` | HTTP SSE client for RFSN backend |
| `URfsnDialogueManager` | Active dialogue management |
| `URfsnTemporalMemory` | State-action-outcome memory |
| `URfsnActionLattice` | Expanded action construction |
| `URfsnEmotionBlend` | VAD emotion model with facial animation |
| `URfsnBackstoryGenerator` | LLM-driven procedural backstories |

### Social & Memory

| Component | Description |
|-----------|-------------|
| `URfsnWitnessSystem` | NPCs observe and share rumors |
| `URfsnNpcMemory` | Persistent conversation tracking |
| `URfsnGroupConversation` | Multi-NPC dialogue with turns |
| `URfsnNpcBarks` | Context-aware one-liners (15+ triggers) |
| `URfsnRelationshipDecay` | Time-based relationship changes |

### Behavior & Environment

| Component | Description |
|-----------|-------------|
| `URfsnNpcSchedule` | Daily routines and patrol routes |
| `URfsnNpcNeeds` | Hunger, energy, social, safety needs |
| `URfsnNpcAwareness` | Detection with FOV and hearing |
| `URfsnWeatherReactions` | Weather and time-of-day awareness |

### Economy & Quests

| Component | Description |
|-----------|-------------|
| `URfsnDynamicPricing` | Reputation-based merchant prices |
| `URfsnQuestIntegration` | Quest-aware NPC dialogue |
| `URfsnFactionSystem` | Group reputation with propagation |

### Presentation

| Component | Description |
|-----------|-------------|
| `URfsnLipSync` | Audio-driven facial animation |
| `URfsnNpcPortrait` | Character card data aggregator |
| `URfsnReputationHud` | Faction standing display |
| `URfsnDialogueCamera` | Focus, over-shoulder, two-shot modes |
| `URfsnNpcLookAt` | Smooth rotation to player |

---

## 📚 API Reference

### Emotion Blending

```cpp
// Apply emotional stimulus
EmotionBlend->ApplyStimulus(TEXT("Joy"), 0.8f);

// Get dialogue tone for LLM
FString Tone = EmotionBlend->ToDialogueTone();
// → "warm, energetic, assertive"

// Get voice modulation
float Pitch = EmotionBlend->GetVoicePitchModifier();
float Speed = EmotionBlend->GetVoiceSpeedModifier();
```

### Witness System

```cpp
// Record player action
WitnessSystem->RecordPlayerAction(
    ERfsnWitnessEventType::Help, 
    "helped the wounded merchant",
    Location, TargetNpcId, 0.7f, true);

// Get gossip for NPC dialogue
FString Gossip = WitnessSystem->GetGossipForNpc(NpcId);
// → "I heard that you helped the wounded merchant"
```

### NPC Memory

```cpp
// Check if NPC has met player
if (Memory->HasMetPlayer()) {
    FString Context = Memory->GetMemoryContext(3);
}

// Create memory from conversation
Memory->StartConversation();
Memory->RecordPlayerStatement(PlayerText);
FGuid MemId = Memory->EndConversation();
```

### Dynamic Pricing

```cpp
// Get price with reputation modifier
float Price = Pricing->GetPrice("medkit");

// Apply event modifier
Pricing->AddPriceModifier("shortage", 1.5f, "supplies");
```

### NPC Schedules

```cpp
// Check current activity
ERfsnActivityType Activity = Schedule->CurrentActivity;

// Get target location
FVector Target = Schedule->GetCurrentTargetLocation();

// Interrupt for dialogue
Schedule->InterruptSchedule(ERfsnActivityType::Idle, 60.0f);
```

### Group Conversations

```cpp
// Start NPC group chat
GroupConv->StartConversation({NpcA, NpcB, NpcC}, "weather");

// Player joins
GroupConv->PlayerJoin();
GroupConv->PlayerSpeak("What do you think?");
```

---

## 📊 Statistics

| Metric | Count |
|--------|-------|
| **C++ Classes** | 75+ |
| **Subsystems** | 10+ |
| **Console Commands** | 10 |
| **Default Factions** | 5 |
| **Bark Triggers** | 15 |
| **Weather Types** | 9 |
| **Emotion States** | 8 |
| **Lines of Code** | 30,000+ |

---

## 🗂️ Project Structure

```
ISLAND/
├── Source/MyProject/
│   ├── Public/                     # 75+ Headers
│   │   ├── Rfsn*.h                 # All RFSN components
│   │   └── RfsnForwardDeclarations.h
│   ├── Private/                    # Implementations
│   └── MyProjectPCH.h              # Shared PCH
├── RFSN_NPC_AI/
│   └── Python/
│       ├── orchestrator.py         # Main server
│       └── mock_server.py          # Offline testing
├── Content/                        # UE assets
├── SETUP_INSTRUCTIONS.md           # Detailed setup
├── RFSN_BLUEPRINT_GUIDE.md         # Blueprint guide
└── README.md                       # This file
```

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
