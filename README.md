# ISLAND

ISLAND is a single-player extraction survival game with optional NPC intelligence.

The shipping loop is intentionally narrow:

`spawn -> collect fuse/fuel/crank -> repair tower -> power/transmit -> survive cult pressure -> extract or die`

Python-backed RFSN dialogue is an optional enhancement, not a requirement for the playable slice.

## Current product boundary

### Core shipping path

These systems sit on the main runtime path:

- `AIslandGameMode`
- `AIslandRadioTower`
- `AIslandExtractionZone`
- `AIslandAISpawnManager`
- `UIslandDirectorSubsystem`
- `UIslandObjectiveSubsystem`
- `UIslandInventoryComponent`
- `UIslandVitalityComponent`
- `UIslandStealthComponent`
- `UIslandInteractorComponent`
- `ACultistCharacter`
- `ACultistAIController`
- minimal HUD/tutorial flow
- minimal dialogue bridge

### Optional enhancement path

These systems can enrich the slice, but do not own progression:

- `URfsnNpcClientComponent`
- `URfsnDialogueManager`
- `URfsnEmotionBlend`
- `URfsnVoiceRouter`
- `URfsnInstantBark`
- `URfsnNpcAwareness`
- `URfsnNpcNeeds`
- `URfsnTemporalMemory`

### Experimental / non-shipping

- social simulation systems under `Source/MyProject/Public|Private/Social/`
- variant plugins under `Plugins/IslandVariantHorror/` and `Plugins/IslandVariantShooter/`
- broader RFSN backend modules that are not required for the offline slice

## Required actors for the current slice

A playable Island map currently needs:

- `IslandGameMode` as the active game mode
- `IslandRadioTower`
- `IslandExtractionZone`
- `IslandAISpawnManager`
- three `IslandPickupActor` placements configured as:
  - fuse
  - fuel
  - crank

## Repo layout

```text
ISLAND/
├── Config/
├── Content/
├── Docs/
│   ├── architecture/
│   ├── gameplay/
│   ├── ai/
│   └── operations/
├── Plugins/
│   ├── IslandVariantHorror/
│   └── IslandVariantShooter/
├── RFSN_NPC_AI/
│   └── Python/
│       ├── app/
│       ├── scripts/
│       ├── tests/
│       └── README.md
├── Source/
│   └── MyProject/
└── README.md
```

## Running the project

### Unreal editor

```bash
./launch_game.sh
```

The launcher builds `MyProjectEditor` and opens `MyProject.uproject`. If autodiscovery fails, set `UE_PATH` before running it.

### Python service (optional)

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

The Unreal dialogue client defaults to `http://127.0.0.1:8000/api/dialogue/stream`.

## Offline mode

The Island slice is expected to remain playable with the backend down.

Current branch behavior:

- tower progression does not depend on the Python service
- the Unreal dialogue client falls back to a local bark when the backend is unavailable
- the NPC interaction path degrades instead of hard-failing the run

See `Docs/ai/offline_mode.md` for the current degraded-mode behavior.

## Tests

Verified on this branch:

- `python3 -m pytest tests/test_bandit_learner.py -q`
- `python3 -m pytest tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py -q`
- `python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py tests/test_streaming_fixes.py -q`

Current verified result:

- 57 targeted backend tests passed
- previously failing `test_bandit_learner.py` and `test_fidelity.py` defects are fixed on this branch

Not yet revalidated in this cloud environment:

- full Unreal compile/open gate (requires local Unreal installation)
- map launch and asset reference validation in-editor
- full Python suite beyond the targeted backend regression set above

## Key docs

- `Docs/architecture/baseline.md`
- `Docs/architecture/runtime_spine.md`
- `Docs/gameplay/vertical_slice.md`
- `Docs/ai/offline_mode.md`
- `Docs/ai/service_contract.md`
- `Docs/operations/test_matrix.md`
