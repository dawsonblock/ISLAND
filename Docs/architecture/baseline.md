# ISLAND baseline

## Repo/runtime snapshot

- Unreal project: `MyProject.uproject`
- Current Unreal runtime module: `MyProject`
- Current PCH: `Source/MyProject/MyProjectPCH.h`
- Unreal Engine association in repo: `5.5`
- Variant source still lives inside the main module:
  - `Source/MyProject/Variant_Horror/*`
  - `Source/MyProject/Variant_Shooter/*`
- Python backend root: `RFSN_NPC_AI/Python`
- Python requirement in `pyproject.toml`: `>=3.12`
- Observed interpreter in this environment: `Python 3.12.3`

## Launch steps

### Unreal editor

From the repo root:

```bash
./launch_game.sh
```

The launcher expects `UE_PATH` to point at an Unreal installation if it cannot auto-discover one, then builds `MyProjectEditor` and opens `MyProject.uproject`.

### Python backend

Current backend entrypoint remains the flat orchestrator module:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

## Known failing Python tests

Observed with:

```bash
cd RFSN_NPC_AI/Python
python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py -q
```

Current failures:

- `tests/test_bandit_learner.py::TestStateActionBandit::test_persistence_save`
- `tests/test_bandit_learner.py::TestStateActionBandit::test_banned_all_actions_fallback`
- `tests/test_bandit_learner.py::TestIntegration::test_complete_workflow`
- `tests/test_fidelity.py::test_smart_sentence_splitting`

Observed summary: `4 failed, 27 passed`.

## Runtime data hygiene notes

Before this refactor pass, committed runtime data was present under:

- `RFSN_NPC_AI/Python/data/recordings/`
- `RFSN_NPC_AI/Python/data/audit/`
- `RFSN_NPC_AI/Python/data/policy/`
- `RFSN_NPC_AI/data/recordings/`
- `RFSN_NPC_AI/data/audit/`
- `RFSN_NPC_AI/web_chat_ui/backend/episodes/`

This pass redirects new Python-side runtime output toward ignored `var/` directories so recordings, audit logs, policy logs, and learning snapshots stop polluting diffs.
