# Current backend state

## Canonical backend authority

- Backend root: `RFSN_NPC_AI/Python`
- Canonical FastAPI implementation: `app/api/main.py`
- Compatibility launch shim: `orchestrator.py`
- Compatibility wrapper: `main.py`
- Current Unreal client route: `/api/dialogue/stream`

`app/api/main.py` is the only canonical backend authority. `orchestrator.py` and `main.py` exist only to preserve historical imports and launch targets.

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

Canonical launch surface:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn app.api.main:app --host 127.0.0.1 --port 8000
```

Historical compatibility surface, still supported:

```bash
cd RFSN_NPC_AI/Python
python3 -m uvicorn orchestrator:app --host 127.0.0.1 --port 8000
```

## Current backend limits

The authority surface is singular in practice and the package-local import surface is now stable:

- `app/api/main.py` resolves runtime paths through the package-local `app.runtime_paths` surface and uses package-local bridges for legacy helper modules
- committed service assets such as `RFSN_NPC_AI/config.json`, dashboard files, and optional legacy `RFSN_NPC_AI/Models/` content stay under the service root
- runtime-created state such as config copies, API keys, durable memory, downloaded models, and recordings now defaults under `RFSN_NPC_AI/Python/var/` and can be relocated with `RFSN_RUNTIME_ROOT`

The remaining proof limits are narrower now:

- full backend teardown confidence still depends on runtime behavior beyond the current automated coverage, even though websocket/config watcher ownership is explicit
- full Unreal-integrated dialogue validation still requires local in-editor/runtime verification

## Runtime data boundary

Runtime state is being pushed toward ignored `var/` locations instead of checked-in `data/` folders for artifacts such as:

- recordings
- audit logs
- policy logs
- learning snapshots and state

That keeps source control focused on code, fixtures, and intentional test assets instead of live runtime residue.
