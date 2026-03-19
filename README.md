<div align="center">

# 🏝️ ISLAND

### Autonomous NPC Intelligence for Unreal Engine 5.7

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-0E1128?style=for-the-badge&logo=unrealengine)](https://unrealengine.com)
[![C++](https://img.shields.io/badge/C++-20-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![Python](https://img.shields.io/badge/Python-3.12+-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://python.org)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

**A bounded decision system where LLMs are deliberately demoted to renderers.**

[Features](#-features) • [Quick Start](#-quick-start) • [Architecture](#-architecture) • [Components](#-component-reference) • [API](#-api-reference)

---

</div>

## 🎯 What This Is

ISLAND is **not** "an NPC with an LLM." It's a **bounded decision system** where:

- **Action selection happens before language** — the LLM never invents goals
- **State drives intent** — mood, relationship, and affinity determine behavior
- **Learning is scoped and reversible** — per-state isolation, bounded rewards, explicit bans
- **80+ C++ components** — comprehensive NPC AI framework
- **Dual-Model TTS** — Kokoro/Chatterbox routing based on narrative weight

---

## 🚀 Quick Start

### One-Command Launch

```bash
./launch_game.sh
```

This script auto-compiles and opens the Unreal Editor.

### Current Playable Slice

The active island loop is:

**spawn -> collect fuse/fuel/crank -> repair tower -> transmit -> survive cult convergence -> extract**

For the up-to-date runtime hookup guide, see:

- [`FIRST_PLAYABLE_SLICE_SETUP.md`](./FIRST_PLAYABLE_SLICE_SETUP.md)

### Prerequisites

| Requirement | Version |
|-------------|---------|
| macOS | 14+ |
| Unreal Engine | 5.7 |
| Python | 3.12+ |
| [Ollama](https://ollama.ai) | Latest + `llama3.2` |

### Full Setup

```bash
# 1. Clone
git clone https://github.com/dawsonblock/ISLAND.git
cd ISLAND

# 2. Start RFSN Server
cd RFSN_NPC_AI/Python
uv sync
uv run uvicorn orchestrator:app --host 127.0.0.1 --port 8000

# 3. Launch Game (new terminal)
cd ../..
./launch_game.sh
```

### In Unreal Editor

1. Make sure the map uses `IslandGameMode`
2. Place an `IslandRadioTower`, `IslandExtractionZone`, and `IslandAISpawnManager`
3. Place three `IslandPickupActor` instances for fuse, fuel, and crank
4. Press **Play**
5. Press **E** to scavenge parts and interact with the tower

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

### 🎤 Voice & Audio

- **Kokoro TTS** — Fast local voice synthesis
- **Voice Router** — Auto-selects quality based on context
- **Instant Barks** — Masks 2s latency with pre-recorded audio
- **Emotion-to-Voice** — Arousal/valence → style mapping
- **Audio Attenuation** — 3D spatial dialogue

</td>
<td>

### ⚡ Performance

- **Latency Optimizations** — Gemini-analyzed architecture
- **Pipeline Reordering** — Action before generation
- **User-Centric Rewards** — Fixes echo chamber bug
- **Clause Tokenizer** — Faster TTS chunking
- **Aggressive Pruning** — Context limit = 4

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
│                      RENDER LAYER (LLM + TTS)                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │   Action Hint + NPC State + Context → Natural Language    │ │
│  └────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │   Voice Router: Intensity=Low→Turbo | High→Full           │ │
│  │   + Instant Barks + Lip Sync + Facial Animation           │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

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

### Voice & Audio

| Component | Description |
|-----------|-------------|
| `URfsnVoiceRouter` | Routes TTS to appropriate backend |
| `URfsnInstantBark` | Plays barks immediately for latency masking |
| `URfsnTtsAudioComponent` | Procedural audio playback |
| `URfsnAudioSettings` | 3D attenuation and occlusion |

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

---

## 📊 Statistics

| Metric | Count |
|--------|-------|
| **C++ Classes** | 80+ |
| **Python Modules** | 40+ |
| **Tests Passing** | 244/258 (94.6%) |
| **Console Commands** | 10 |
| **Default Factions** | 5 |
| **Bark Categories** | 12 |
| **Lines of Code** | 40,000+ |

---

## 🗂️ Project Structure

```
ISLAND/
├── Source/MyProject/
│   ├── Public/                     # 80+ Headers
│   │   ├── Rfsn*.h                 # All RFSN components
│   │   └── RfsnForwardDeclarations.h
│   ├── Private/                    # Implementations
│   └── MyProjectPCH.h              # Shared PCH
├── RFSN_NPC_AI/
│   └── Python/
│       ├── orchestrator.py         # Main server
│       ├── kokoro_tts.py           # Kokoro TTS integration
│       ├── latency_optimizations.py # Performance tuning
│       └── mock_server.py          # Offline testing
├── Content/                        # UE assets
├── launch_game.sh                  # One-click build & launch
├── SETUP_INSTRUCTIONS.md           # Detailed setup
└── README.md                       # This file
```

---

## ⚡ Latency Optimization

| Optimization | Effect | Implementation |
|--------------|--------|----------------|
| **Instant Barks** | Masks ~2000ms perceived latency | Play generic bark immediately, stream unique response |
| **Pipeline Reordering** | Saves ~200ms | Yield action before memory retrieval |
| **User-Centric Rewards** | Fixes echo chamber | Analyze user input, not NPC emotion |
| **Clause Tokenizer** | Faster TTS | Split on clauses, not sentences |
| **Context Pruning** | Saves ~300ms | Limit history to 4 turns |

### Before vs After

```
BEFORE: Player speaks → [2000ms] → NPC responds
AFTER:  Player speaks → [50ms] → "Hmm..." → [1800ms] → Full response streams
                         ↑ Instant bark masks wait time
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
