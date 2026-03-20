# Test matrix

## Verified in this cloud branch

| Area | Command | Result |
|---|---|---|
| Voice import gate | `cd RFSN_NPC_AI/Python && python3 -m py_compile app/voice/streaming_voice_system.py` | passed |
| Action prompt compatibility | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_action_prompts.py -q` | 10 passed |
| NPC action bandit contract | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_npc_action_bandit.py -q` | 14 passed |
| Dashboard route regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_dashboard_ui.py tests/test_operational.py -q` | 5 passed |
| Full Python backend | `cd RFSN_NPC_AI/Python && python3 -m pytest --continue-on-collection-errors -q` | 258 passed, 1 skipped |

## Not verified here

| Area | Status | Notes |
|---|---|---|
| Unreal compile | not run | requires Unreal build environment |
| Editor map launch | not run | requires local editor/runtime assets |
| Asset redirect/reference validation | not run | variant content folders were not present in this repo snapshot |
| Full Python suite | not run | targeted regression suites were prioritized |

## Current gate status

- Gate A - repo hygiene: satisfied on this branch
- Gate B - Unreal structure only: source tree and include wiring changed, but full compile still needs local verification
- Gate C - gameplay ownership cleanup: code path updated; in-editor validation still pending
- Gate D - dialogue reliability: targeted backend/client code updated; Unreal runtime validation still pending
- Gate E - Python backend: full suite green in this cloud environment
- Gate F - full vertical slice: still requires local map playthrough
