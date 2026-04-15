# Build State Report

Generated: 2026-04-14
Host: macOS
Repo: ISLAND

## Updated status after fixes

The repo-side build issues identified in the initial analysis have now been fixed and revalidated.

- Python backend: green. `258 passed, 1 skipped`.
- Docker build path: green from the repo root when `RFSN_NPC_AI` is used as the build context.
- Local helper script: green for `test`; repo root resolution is fixed.
- Unreal editor build on this machine: green when full Xcode is selected. `launch_game.sh` now auto-selects `/Applications/Xcode.app/Contents/Developer` when available.

Remaining non-fatal issues:

- FastAPI deprecation warnings remain in `app/api/main.py` because the app still uses `@app.on_event(...)`.
- Python teardown emits a `multiprocess.resource_tracker` `AttributeError` after tests complete, but it does not fail the suite.
- GitHub workflow validation warns about secret references in `.github/workflows/docker-push.yml`; those are expected if the secrets are not defined in the current repo configuration.

## Revalidation after fixes

| Area | Command | Result | Notes |
| --- | --- | --- | --- |
| Targeted TTL regression | `cd RFSN_NPC_AI/Python && python3 -m pytest tests/test_production.py::TestMemoryGovernance::test_memory_ttl_expiration -q` | passed | Confirms TTL now starts when memory admission completes |
| Python backend | `cd RFSN_NPC_AI/Python && python3 -m pytest --continue-on-collection-errors -q` | passed | `258 passed, 1 skipped, 4 warnings` |
| Local helper script | `cd /Users/dawsonblock/Documents/ISLAND && ./scripts/ci-cd.sh test` | passed | Backend suite completed and coverage XML generated |
| Docker from repo root | `cd /Users/dawsonblock/Documents/ISLAND && docker build -f RFSN_NPC_AI/Dockerfile -t island-backend:build-analysis-fixed RFSN_NPC_AI` | passed | Confirms docs/CI/script path is now valid |
| Unreal editor compile | `cd /Users/dawsonblock/Documents/ISLAND && DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer SKIP_LAUNCH=1 ./launch_game.sh` | passed | Full editor target compiled successfully on this machine |

The historical failure analysis below is retained for traceability but is superseded by the updated status above.

## Executive summary

The build is not green end to end.

- The Python backend is close to green, but the full test suite currently fails with 1 failing test, 257 passing tests, and 1 skipped test.
- The Docker image itself is buildable, but the documented and automated build commands are using the wrong build context, so the current CI Docker path is effectively broken.
- The Unreal editor target did not reach project compilation on this machine because the local Unreal/macOS toolchain setup is incomplete.
- The local CI/CD helper script is also broken because it computes the repository root incorrectly.

## Commands executed

| Area | Command | Result | Notes |
| --- | --- | --- | --- |
| Python backend | `cd RFSN_NPC_AI/Python && python3 -m pytest --continue-on-collection-errors -q` | failed | `1 failed, 257 passed, 1 skipped` in `62.04s` |
| Docker via documented repo-root path | `cd /Users/dawsonblock/Documents/ISLAND && docker build -f RFSN_NPC_AI/Dockerfile -t island-backend:build-analysis .` | failed | Docker context does not contain `Python/`, `config.json`, or `Dashboard/` at the paths expected by the Dockerfile |
| Docker via corrected service-root path | `cd RFSN_NPC_AI && docker build -f Dockerfile -t island-backend:build-analysis-local .` | passed | Confirms the Dockerfile itself is viable when invoked from the expected context |
| Unreal editor compile | `cd /Users/dawsonblock/Documents/ISLAND && SKIP_LAUNCH=1 ./launch_game.sh` | failed | UnrealBuildTool stopped before C++ compile because the Mac platform/toolchain setup is incomplete |
| Local helper script | `cd /Users/dawsonblock/Documents/ISLAND && ./scripts/ci-cd.sh test` | failed | Script resolves `REPO_ROOT` to the parent of the repo |

## Detailed findings

### 1. Python backend status: mostly healthy, but not green

Observed result:

- `1 failed, 257 passed, 1 skipped, 4 warnings`

Failing test:

- `RFSN_NPC_AI/Python/tests/test_production.py::TestMemoryGovernance::test_memory_ttl_expiration`

Failure details:

- The test expects a newly added memory with `ttl_seconds=0.1` to still be visible on the first `query_memories()` call.
- In practice, `query_memories()` returns an empty list immediately after admission.

Why it is failing:

- `GovernedMemory.is_expired()` in `RFSN_NPC_AI/Python/app/memory/memory_governance.py` treats TTL with sub-second precision.
- `query_memories()` filters expired memories by default.
- The admission path is not cheap: semantic model initialization and indexing happen before the first query in this test path.
- With a TTL of only `0.1` seconds, the memory is already expired by the time the first query runs.

Interpretation:

- This is a real correctness gap in the current build signal, even if the root fix may end up being in the test, the implementation, or both.
- At minimum, the test is too timing-sensitive for the current admission path.

Secondary Python observations:

- `RFSN_NPC_AI/Python/app/api/main.py` emits FastAPI deprecation warnings because it still uses `@app.on_event("startup")` and `@app.on_event("shutdown")`.
- These warnings are not currently breaking the build, but they are technical debt and will matter on future FastAPI upgrades.

### 2. Docker status: image build is valid, automation is wrong

What failed:

- The repo-root build command used in automation and docs fails immediately on `COPY` steps in `RFSN_NPC_AI/Dockerfile`.

Why it failed:

- The Dockerfile assumes the build context is `RFSN_NPC_AI/`.
- It copies relative paths such as:
  - `Python/requirements-core.txt`
  - `Python/`
  - `config.json`
  - `Dashboard/`
- Those paths exist when the build context is `RFSN_NPC_AI/`, but not when the build context is the repository root.

What passed:

- `cd RFSN_NPC_AI && docker build -f Dockerfile -t island-backend:build-analysis-local .`

What this means:

- The container recipe itself is usable.
- The broken part is the automation and documentation around it.

Affected automation/docs:

- `.github/workflows/build-test.yml` uses `context: .` with `file: RFSN_NPC_AI/Dockerfile`
- `scripts/ci-cd.sh` builds from the repo root
- `CICD.md` documents the repo-root build command
- `DEPLOYMENT.md` documents the repo-root build command
- `GIT_TESTING_GUIDE.md` documents the repo-root build command
- `GIT_QUICK_REFERENCE.md` documents the repo-root build command

Practical impact:

- The Docker job in CI is likely red or fragile until the build context and Dockerfile assumptions are made consistent.

### 3. Unreal status: blocked by local machine setup, not yet a project compile verdict

Observed result from `./launch_game.sh`:

- UnrealBuildTool starts correctly.
- The build halts with: `Platform Mac is not a valid platform to build.`

Environment evidence:

- `UE_PATH` is set to `/Users/Shared/Epic Games/UE_5.7`
- `xcode-select -p` points to `/Library/Developer/CommandLineTools`
- `xcodebuild` is unavailable because the active developer directory is not a full Xcode install
- `/Users/Shared/Epic Games/UE_5.7/Engine/Platforms/Mac` does not exist

Interpretation:

- This machine is not currently in a state where it can prove or disprove whether the project C++ code compiles on Mac.
- The current blocker is environment/platform support, not a confirmed gameplay module compile error.

Project-level note discovered during build:

- `Source/MyProject/MyProject.Build.cs` still uses the obsolete `bEnforceIWYU` flag on UE 5.7.
- That produced a warning, not the fatal error.

### 4. Local build helper status: broken path resolution

Observed result:

- `./scripts/ci-cd.sh test` fails trying to `cd` into `/Users/dawsonblock/Documents/RFSN_NPC_AI/Python`

Why it failed:

- `scripts/ci-cd.sh` computes `REPO_ROOT` as `SCRIPT_DIR/../..`
- Since the script lives in `ISLAND/scripts/`, that resolves to the parent of the repo instead of the repo root

Practical impact:

- The helper script currently cannot run its documented backend test path locally.
- The same root issue likely affects its Docker and Compose subcommands too.

### 5. Documentation drift is now part of the build problem

Several docs currently claim or imply that the full build is in a healthier state than the repository actually demonstrates today.

Specific drift observed:

- `PROJECT_EXECUTION_SUMMARY.md` states the Docker image builds successfully, but the repo-root command path currently fails.
- `Docs/operations/test_matrix.md` says full backend is green in the referenced environment, but this machine currently reproduces a failing production test.
- Multiple how-to docs still recommend the broken repo-root Docker build command.

This matters because it increases the cost of diagnosing failures. The docs currently point contributors toward commands that do not match the repository layout.

## Current build verdict by surface

| Surface | Verdict | Reason |
| --- | --- | --- |
| Python backend tests | yellow | Nearly green, but blocked by one deterministic failing test |
| Docker image recipe | green | Builds successfully when invoked from `RFSN_NPC_AI/` |
| Docker automation and CI path | red | Current context/path assumptions are inconsistent |
| Unreal local compile on this machine | blocked | Requires full Xcode selection and Mac platform support in the UE install |
| Local helper scripts | red | Broken repo root calculation |
| Documentation accuracy | yellow | Several docs no longer match the verified build path |

## Highest-priority next actions

1. Fix the Docker build context mismatch first.
   - Either change CI/scripts/docs to build from `RFSN_NPC_AI/`, or rewrite the Dockerfile `COPY` paths to intentionally support repo-root context.
   - Right now the same mismatch appears in automation, helper scripts, and documentation.

2. Fix or re-specify the TTL expiration behavior in memory governance.
   - Decide whether sub-second TTLs are supported.
   - Then align `test_memory_ttl_expiration` and `MemoryGovernance` with that decision.

3. Repair the helper script root calculation.
   - `scripts/ci-cd.sh` should resolve the repo root to `ISLAND/`, not its parent directory.

4. Repair the local Unreal Mac toolchain before drawing conclusions about the C++ game build.
   - Install/select full Xcode.
   - Ensure the UE installation includes Mac platform support.
   - Re-run `SKIP_LAUNCH=1 ./launch_game.sh` after that.

5. Update build-facing docs after the above fixes.
   - The current docs are ahead of the verified state in several places.

## Bottom line

The original findings above explain the failure state that was reproduced at the start of this analysis. For the current verified state, use the `Updated status after fixes` and `Revalidation after fixes` sections at the top of this report.
