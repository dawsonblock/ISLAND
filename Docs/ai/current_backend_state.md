# Current backend state

## Shape of the service today

- Backend root: `RFSN_NPC_AI/Python`
- Main API file: `orchestrator.py`
- Current API route used by the Unreal client: `/api/dialogue/stream`
- The codebase is still mostly flat modules at the Python root, with a smaller `learning/` package alongside it.

## Version/runtime facts

- Python requirement: `>=3.12` (`RFSN_NPC_AI/Python/pyproject.toml`)
- Observed interpreter in this environment: `Python 3.12.3`
- The backend is optional for the shipping game target and should degrade cleanly when unavailable.

## Launch steps

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

The Unreal client currently defaults to:

- `http://127.0.0.1:8000/api/dialogue/stream`

## Observed defects

Confirmed by test run:

- `bandit_learner.py` does not persist after `update()`, which breaks:
  - `test_persistence_save`
  - `test_complete_workflow`
- banned-action handling raises when all candidates are banned, which breaks:
  - `test_banned_all_actions_fallback`
- `streaming_engine.py` still damages punctuation/quotes in sentence splitting, which breaks:
  - `test_smart_sentence_splitting`

## Runtime data boundary

Prior to this hygiene pass, the service wrote runtime state under checked-in `data/` folders.

New default runtime outputs are being redirected toward ignored `var/` locations for:

- recordings
- audit logs
- policy logs
- learning snapshots/state

That keeps source control focused on code and intentional fixtures instead of live runtime residue.
