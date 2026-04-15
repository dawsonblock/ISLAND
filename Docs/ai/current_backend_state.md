# Current backend state

## Canonical backend authority

- Backend root: `RFSN_NPC_AI/Python`
- Canonical FastAPI implementation: `app/api/main.py`
- Compatibility launch shim: `orchestrator.py`
- Current Unreal client route: `/api/dialogue/stream`

`orchestrator.py` is no longer the real implementation surface. It exists to preserve the historical launch target and currently re-exports the FastAPI app from `app.api.main`.

## Package shape

The active backend is package-first now:

- `app/api/` owns the HTTP entrypoint and lifecycle wiring
- `app/dialogue/`, `app/voice/`, `app/memory/`, `app/learning/`, and `app/telemetry/` hold the main runtime subsystems
- a small set of root-level modules still exists for compatibility and shared helpers

That means the backend is not "mostly flat" anymore, even though a few top-level modules are still imported by the canonical entrypoint.

## Runtime facts

- Python requirement: `>=3.12` (`RFSN_NPC_AI/Python/pyproject.toml`)
- The backend remains optional for the playable Island slice and should degrade cleanly when unavailable
- The Unreal client default remains `http://127.0.0.1:8000/api/dialogue/stream`

## Supported launch commands

Historical launch surface, still supported:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

Equivalent direct entrypoint to the canonical module:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn app.api.main:app --host 127.0.0.1 --port 8000
```

## Remaining cleanup edges

The authority surface is singular in practice, but two proof items are still being tightened:

- `app/api/main.py` still imports a few root-level helpers directly (`runtime_paths`, `model_manager`, `latency_optimizations`)
- shutdown ownership around websocket/config watcher cleanup is being tightened so teardown claims stay fully defensible

## Runtime data boundary

Runtime state is being pushed toward ignored `var/` locations instead of checked-in `data/` folders for artifacts such as:

- recordings
- audit logs
- policy logs
- learning snapshots and state

That keeps source control focused on code, fixtures, and intentional test assets instead of live runtime residue.
