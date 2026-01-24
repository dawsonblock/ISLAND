# ISLAND - Survival Horror Game

An Unreal Engine 5.5 survival horror game featuring AI-driven NPC dialogue and dynamic game pacing.

## Features

- **Subsystem-First Architecture** - Decoupled core systems from GameModes
- **Alert System** - Dynamic threat level that influences gameplay
- **Radio Tower Extraction** - Multi-stage objective system
- **Two Game Variants** - Horror (stealth) and Shooter (action)
- **🆕 RFSN AI Integration** - LLM-driven NPC dialogue and Director pacing

---

## RFSN NPC AI Integration

ISLAND integrates **RFSN (Reactive Finite State Network)** for intelligent NPC conversations and AI-driven game pacing.

### Key Components

| Component | Purpose |
|-----------|---------|
| `RfsnNpcClientComponent` | HTTP SSE client for NPC dialogue |
| `RfsnDirectorBridge` | AI pacing → IslandDirectorSubsystem |
| `RfsnDialogueManager` | Coordinates player-NPC conversations |
| `RfsnChoiceWidget` | Multiple-choice player responses |
| `RfsnConversationLog` | Dialogue history tracking |
| `RfsnQuestBridge` | Dialogue actions → Quest objectives |

### Quick Start

1. **Start RFSN Server:**

   ```bash
   cd RFSN_NPC_AI/Python
   source .venv/bin/activate
   pip install -r requirements.txt
   python -m uvicorn orchestrator:app --port 8000
   ```

2. **Build Project** in Unreal Engine 5.5

3. **Create Dialogue Key:**
   - Create `IA_Dialogue` Input Action
   - Assign to `DialogueAction` on your character

4. **Place RFSN NPCs:**
   - Use `RfsnSampleMerchant` or `RfsnSampleGuard`
   - Or add `URfsnNpcClientComponent` to any actor

5. **Test:**
   - Walk near NPC
   - Press dialogue key
   - Watch subtitles at bottom of screen!

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/dialogue/stream` | POST | Stream NPC dialogue (SSE) |
| `/api/director/control` | POST | Get pacing commands |
| `/api/health` | GET | Server health check |

---

## Project Structure

```
ISLAND/
├── Source/MyProject/
│   ├── Public/           # Headers
│   ├── Private/          # Implementations
│   ├── Variant_Horror/   # Horror game mode
│   └── Variant_Shooter/  # Shooter game mode
├── RFSN_NPC_AI/          # Python AI backend
│   └── Python/           # FastAPI server
├── SETUP_INSTRUCTIONS.md # Full setup guide
└── RFSN_BLUEPRINT_GUIDE.md # Blueprint integration
```

## Documentation

- [SETUP_INSTRUCTIONS.md](SETUP_INSTRUCTIONS.md) - Complete setup guide
- [RFSN_BLUEPRINT_GUIDE.md](RFSN_BLUEPRINT_GUIDE.md) - Blueprint NPC setup
- [ISLAND_SYSTEM_REFERENCE.md](ISLAND_SYSTEM_REFERENCE.md) - Architecture overview

## Requirements

- Unreal Engine 5.5
- Python 3.10+ (for RFSN server)
- macOS, Windows, or Linux

## License

MIT
