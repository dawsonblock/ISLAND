# RFSN Python backend

This directory now uses an `app/` package layout while keeping root-level compatibility shims for existing tests and launch commands.

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

Existing commands still work through shims:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

## Runtime data

Runtime output defaults to ignored `var/` directories rather than checked-in `data/` logs for:

- recordings
- audit logs
- policy logs
- learning snapshots/state

## Verified backend tests on this branch

- `python3 -m pytest tests/test_bandit_learner.py -q`
- `python3 -m pytest tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py -q`
- `python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py tests/test_streaming_fixes.py -q`
