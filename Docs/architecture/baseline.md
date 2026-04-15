# ISLAND baseline

This document captures the repo state at the start of the runtime-boundary
refactor. It is intentionally historical, not a description of the current
branch head after subsequent cleanup.

## Repo/runtime snapshot

- Unreal project: `MyProject.uproject`
- Current Unreal runtime module: `MyProject`
- Current PCH: `Source/MyProject/MyProjectPCH.h`
- Unreal Engine association in repo: `5.5`
- At baseline, variant source still lived inside the main module:
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

At baseline, the backend launch command had already moved to the package-local canonical module:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn app.api.main:app --host 127.0.0.1 --port 8000
```

## Known failing Python tests

Observed with:

```bash
cd RFSN_NPC_AI/Python
python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py -q
```

Baseline failures:

- `tests/test_bandit_learner.py::TestStateActionBandit::test_persistence_save`
- `tests/test_bandit_learner.py::TestStateActionBandit::test_banned_all_actions_fallback`
- `tests/test_bandit_learner.py::TestIntegration::test_complete_workflow`
- `tests/test_fidelity.py::test_smart_sentence_splitting`

Observed summary: `4 failed, 27 passed`.

## Runtime data hygiene notes

Before this refactor pass, committed runtime data was present under multiple repo-relative runtime output directories, including old Python-side recording, audit, policy, and episode folders.

The refactor redirected new Python-side runtime output toward ignored `var/`
directories so recordings, audit logs, policy logs, and learning snapshots stop
polluting diffs.
