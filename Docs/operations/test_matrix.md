# Test matrix

## Verified in this cloud branch

| Area | Command | Result |
|---|---|---|
| Baseline failure capture | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py -q` | observed 4 failures before defect fixes |
| Learner contract | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_bandit_learner.py -q` | 28 passed |
| Tokenizer/fidelity | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py -q` | 10 passed |
| Expanded backend regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_bandit_learner.py tests/test_fidelity.py tests/test_tokenizer_boundaries.py tests/test_flush_semantics.py tests/test_streaming_fixes.py -q` | 57 passed |
| Python syntax after package move | targeted `py_compile` over changed files | passed |

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
- Gate E - Python backend: targeted confirmed defects fixed and verified
- Gate F - full vertical slice: still requires local map playthrough
