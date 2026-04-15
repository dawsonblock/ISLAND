# Test matrix

Generated: 2026-04-15

## Verified automated and scripted checks

| Area | Command | Result | Scope |
| --- | --- | --- | --- |
| Voice import gate | `cd RFSN_NPC_AI/Python && python3 -m py_compile app/voice/streaming_voice_system.py` | passed | Syntax gate for the voice import surface |
| Action prompt compatibility | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_action_prompts.py -q` | 10 passed | Dialogue prompt compatibility |
| NPC action bandit contract | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_npc_action_bandit.py -q` | 14 passed | Learning-layer contract checks |
| Dashboard route regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_dashboard_ui.py tests/test_operational.py -q` | 5 passed | Dashboard and operational route smoke tests |
| Targeted TTL regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_production.py::TestMemoryGovernance::test_memory_ttl_expiration -q` | passed | Confirms the repaired TTL admission behavior |
| Runtime-root targeted backend slice | `cd RFSN_NPC_AI/Python && RFSN_RUNTIME_ROOT="$(mktemp -d)" python3 -m pytest tests/test_runtime_paths.py tests/test_integration.py tests/test_memory_manager.py tests/test_memory_leaks.py -q` | 33 passed, 1 skipped | Confirms isolated runtime-root behavior and core backend regressions |
| Full Python backend | `cd RFSN_NPC_AI/Python && RFSN_RUNTIME_ROOT="$(mktemp -d)" python3 -m pytest --continue-on-collection-errors -q` | 261 passed, 1 skipped | Full backend suite in the verified environment under an isolated runtime root |
| Helper script backend path | `cd /Users/dawsonblock/Documents/ISLAND && ./scripts/ci-cd.sh test` | passed | Confirms the local helper resolves the repo root correctly |
| Docker build path | `cd /Users/dawsonblock/Documents/ISLAND && docker build -f RFSN_NPC_AI/Dockerfile -t island-backend:test-matrix RFSN_NPC_AI` | passed | Confirms the documented repo-root invocation uses the correct build context |

## Verified only on a provisioned local machine

| Area | Command | Result | Notes |
| --- | --- | --- | --- |
| Unreal editor compile | `cd /Users/dawsonblock/Documents/ISLAND && DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer SKIP_LAUNCH=1 ./launch_game.sh` | passed | Requires local Unreal installation, Mac platform support, and full Xcode |

## Not yet revalidated in the current proof pass

| Area | Status | Notes |
| --- | --- | --- |
| Editor map launch | pending | Requires local editor/runtime assets and a playable map pass |
| Asset redirect/reference validation | pending | Requires in-editor validation across the current content set |
| Full vertical-slice playthrough | pending | Must be verified interactively in Unreal |

## Current gate status

- Gate A - repo hygiene: satisfied
- Gate B - Unreal compile: satisfied on a provisioned local machine; still environment-dependent
- Gate C - gameplay ownership cleanup: source and config authority aligned; in-editor runtime validation still pending
- Gate D - dialogue reliability: backend test surface is green; Unreal runtime degradation path still needs local play validation
- Gate E - Python backend: satisfied in the verified environment
- Gate F - full vertical slice: still requires local map playthrough
