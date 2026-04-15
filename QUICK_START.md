# ISLAND Quick Start

This repo now has a concrete first playable slice:

## Slice Loop

spawn on beach -> scavenge tower parts -> repair under cult pressure -> transmit -> survive convergence -> extract

For the full current hookup guide, read:

- [`FIRST_PLAYABLE_SLICE_SETUP.md`](./FIRST_PLAYABLE_SLICE_SETUP.md)
- [`EDITOR_VALIDATION_CHECKLIST.md`](./EDITOR_VALIDATION_CHECKLIST.md)

## Fast path

### 1. Set your Unreal path

```bash
./setup_environment.sh
source "${HOME}/.bashrc"  # or ~/.zshrc depending on your shell
```

Or set it manually:

```bash
export UE_PATH="/Users/Shared/Epic Games/UE_5.7"
```

### 2. Build and open the project

```bash
./launch_game.sh
```

On macOS, the launcher now prefers `/Applications/Xcode.app/Contents/Developer` automatically when Command Line Tools are active. If the build still fails, install full Xcode and open it once to finish setup.

### 3. Use the island runtime

Make sure the active map uses:

- `IslandGameMode`

`MyProjectGameMode` remains in the repo as a legacy prototype surface and should not be used for the current playable slice.

The game mode now sets:

- `MyProjectCharacter` as the default pawn
- `MyProjectPlayerController` as the player controller
- `IslandHUD` as the HUD

### 4. Place the required actors in the level

- `IslandRadioTower`
- `IslandExtractionZone`
- `IslandAISpawnManager`
- three `IslandPickupActor` instances configured as:
  - `TowerFuse`
  - `TowerFuel`
  - `AntennaCrank`

### 5. Verify the loop in PIE

1. collect fuse, fuel, and crank
2. interact with the tower to install parts and begin timed repair
3. survive cult investigation during repair
4. power the tower
5. transmit the distress signal
6. survive the pressure spike
7. extract after transmission completes

## Controls the slice expects

- `InteractAction` -> recommended: `E`
- `SprintAction` -> recommended: `Left Shift`
- `FlashlightAction` -> recommended: `F`

Make sure the input mapping context on `MyProjectPlayerController` includes those actions.

## Main troubleshooting

### No interact prompt

- the player is not using `MyProjectCharacter`
- `InteractAction` is not assigned
- the pickup/tower mesh is not blocking `Visibility`

### No cultists

- `IslandAISpawnManager` is missing
- `CultistClass` is not assigned
- there is no navmesh around the spawn area

### Extraction never activates

- transmission did not complete
- the level is overriding `IslandGameMode`
- the level was switched back to legacy `MyProjectGameMode`
- `IslandExtractionZone` is missing

### Build script cannot find Unreal

- confirm `echo $UE_PATH`
- or rerun `./setup_environment.sh`

For the exact setup guide, use `FIRST_PLAYABLE_SLICE_SETUP.md`.
For the quick pass/fail editor checklist, use `EDITOR_VALIDATION_CHECKLIST.md`.
