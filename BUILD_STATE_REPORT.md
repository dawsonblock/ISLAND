# Build State Report

Generated: 2026-04-14
Local Unreal validation host: macOS
Repo: ISLAND

## Current verified state

This report tracks the current verified build surface only. Earlier reproduced failures were fixed during the repair pass and are preserved in git history rather than mixed into the current-state report.

- Python backend: green. `258 passed, 1 skipped`.
- Docker build path: green from the repo root when `RFSN_NPC_AI` is used as the build context.
- Local helper script: green for `test`; repo root resolution is fixed.
- Unreal editor compile on the provisioned local machine: green when full Xcode is selected.
- CI truth surface: hard gates and advisory checks are now labeled separately instead of silently masking failures.

## Verified commands

| Area | Command | Result | Notes |
| --- | --- | --- | --- |
| Targeted TTL regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_production.py::TestMemoryGovernance::test_memory_ttl_expiration -q` | passed | Confirms TTL now starts when memory admission completes |
| Python backend | `cd RFSN_NPC_AI/Python && python3 -m pytest --continue-on-collection-errors -q` | passed | `258 passed, 1 skipped, 4 warnings` |
| Local helper script | `cd /Users/dawsonblock/Documents/ISLAND && ./scripts/ci-cd.sh test` | passed | Backend suite completed and coverage XML generated |
| Docker from repo root | `cd /Users/dawsonblock/Documents/ISLAND && docker build -f RFSN_NPC_AI/Dockerfile -t island-backend:build-analysis-fixed RFSN_NPC_AI` | passed | Confirms docs, CI, and helper scripts now use the correct build context |
| Unreal editor compile | `cd /Users/dawsonblock/Documents/ISLAND && DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer SKIP_LAUNCH=1 ./launch_game.sh` | passed | Full editor target compiled successfully on a machine with Unreal and full Xcode installed |

## Remaining limits

- Full Unreal validation is still environment-dependent and cannot be proven in a generic cloud runner.
- Editor map launch, asset redirect checks, and a full playable-slice pass still require a local Unreal session.
- Backend teardown proof is improved but not complete; explicit shutdown ownership around websocket and config watcher cleanup is still being tightened.

## Surface verdict

| Surface | Verdict | Reason |
| --- | --- | --- |
| Python backend tests | green | Full backend suite revalidated successfully |
| Docker image recipe and invocation path | green | Dockerfile and repo-root invocation agree on `RFSN_NPC_AI` as the build context |
| Local helper scripts | green | `scripts/ci-cd.sh` now resolves the repo root correctly |
| Unreal local compile | green on provisioned macOS | Verified locally with full Xcode and a Mac-capable Unreal install |
| CI proof surface | green with advisories | Hard gates fail normally; style and proof-doc checks are explicitly advisory |
| Full playable slice | pending local validation | Requires in-editor/runtime verification |

## Current build verdict

The repaired repo is materially healthier than the earlier failure snapshot. The remaining gaps are no longer repo-layout or TTL regressions; they are the environment-dependent Unreal checks and the final backend teardown hardening pass.
