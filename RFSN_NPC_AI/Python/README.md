# RFSN Python backend

This directory now uses an `app/` package layout while keeping root-level compatibility shims for older tests and launch commands.

## Layout

```text
Python/
├── app/
│   ├── api/
│   ├── dialogue/
│   ├── learning/
│   ├── memory/
│   ├── safety/
│   ├── telemetry/
│   └── voice/
├── scripts/
├── tests/
├── runtime_paths.py
└── README.md
```

## Entrypoints

Canonical launch command:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn app.api.main:app --host 127.0.0.1 --port 8000
```

Compatibility launch surface, still supported:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

## Runtime data

Runtime output defaults to ignored `var/` directories rather than checked-in `data/` logs for:

- runtime config copies
- API keys
- conversation memory and backups
- downloaded runtime models
- recordings
- audit logs
- policy logs
- learning snapshots/state

## Verified backend tests on this branch

- `python3 -m py_compile app/voice/streaming_voice_system.py`
- `python3 -m pytest tests/test_action_prompts.py -q`
- `python3 -m pytest tests/test_npc_action_bandit.py -q`
- `python3 -m pytest tests/test_dashboard_ui.py tests/test_operational.py -q`
- `python3 -m pytest -q`

Current validated state in this cloud environment:

- the packaged `app/` layout and legacy root shims both import cleanly
- the voice streaming import blocker is fixed
- the legacy action prompt contract is restored on top of the packaged prompt module
- the NPC action bandit contract is aligned with the current world model and tests
- full backend result: 261 passed, 1 skipped
