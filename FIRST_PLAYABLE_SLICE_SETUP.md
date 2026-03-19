# First Playable Slice Setup

This repo now supports the island run loop in C++:

beach spawn -> scavenge fuse/fuel/crank -> repair tower under pressure -> transmit -> survive hunt -> extract

Use this guide for the current setup. Older docs in the repo still describe earlier prototype behavior.

For a shorter in-editor pass/fail checklist after setup, use:

- [`EDITOR_VALIDATION_CHECKLIST.md`](./EDITOR_VALIDATION_CHECKLIST.md)

## 1. Build prerequisites

- Unreal Engine 5.7 installed locally
- `UE_PATH` exported to your engine install, for example:

```bash
export UE_PATH="/Users/Shared/Epic Games/UE_5.7"
```

You can set that automatically with:

```bash
./setup_environment.sh
source "${HOME}/.bashrc"  # or ~/.zshrc depending on your shell
```

Or just run:

```bash
./launch_game.sh
```

`launch_game.sh` now:
- resolves the repo path automatically
- uses `UE_PATH` when present
- searches common install locations when `UE_PATH` is missing
- supports both macOS and Linux build script paths

## 2. Runtime classes that must be active

The playable slice assumes these C++ classes are in use:

- **GameMode:** `IslandGameMode`
- **Default Pawn:** `MyProjectCharacter`
- **Player Controller:** `MyProjectPlayerController`
- **HUD:** `IslandHUD`

`IslandGameMode` now sets the pawn/controller/HUD classes in code, so the main thing you need is for the map to use `IslandGameMode`.

In Unreal Editor:

1. Open **Project Settings -> Maps & Modes**
2. Set **Default GameMode** to `IslandGameMode`
3. If the level overrides GameMode in **World Settings**, set that override to `IslandGameMode` too

## 3. Input setup required in the editor

The player code now exposes these input actions:

- `JumpAction`
- `MoveAction`
- `LookAction`
- `MouseLookAction`
- `SprintAction`
- `InteractAction`
- `FlashlightAction`
- `DialogueAction`

The slice only depends on these for the main loop:

- `MoveAction`
- `LookAction` / `MouseLookAction`
- `SprintAction`
- `InteractAction`

Recommended mapping:

- `InteractAction` -> `E`
- `SprintAction` -> `Left Shift`
- `FlashlightAction` -> `F`

Make sure the input mapping context assigned to `MyProjectPlayerController` includes those actions.

## 4. Level actors you must place

Place one of each:

### Radio objective
- `IslandRadioTower`

### Extraction
- `IslandExtractionZone`

### Pressure
- `IslandAISpawnManager`

The game mode will auto-find these actors if the level contains them.

## 5. Tower setup

`IslandRadioTower` expects required parts. By default it needs:

- `TowerFuse`
- `TowerFuel`
- `AntennaCrank`

You can change that in the actor details panel with `RequiredParts`, but for the first slice keep the defaults.

Recommended tower tuning:

- `RequiredRepairTime`: 4-6 seconds
- `RepairNoisePerSecond`: 8+
- `TransmitDurationSeconds`: 20-30 seconds
- `ExtractWindowSeconds`: 45-60 seconds

## 6. Pickups you must place

Place three `IslandPickupActor` instances and configure them like this:

1. Wreck or beach debris
   - `ItemType = TowerFuse`
   - `Quantity = 1`
   - `PickupPrompt = "Take Fuse"`

2. Hut, shed, or generator area
   - `ItemType = TowerFuel`
   - `Quantity = 1`
   - `PickupPrompt = "Take Fuel"`

3. Ritual site or lookout
   - `ItemType = AntennaCrank`
   - `Quantity = 1`
   - `PickupPrompt = "Take Crank"`

Important:
- the pickup mesh must block the player's interact trace on `Visibility`
- the player must be able to look directly at the pickup to get the prompt

## 7. Cultist spawn setup

Set up `IslandAISpawnManager`:

- `CultistClass` -> `CultistCharacter` or a Blueprint subclass of it
- `MaxAliveCultists` -> 4 to 6
- `SpawnRadius` -> 1800 to 2500
- `MinSpawnDistance` -> 700 to 1000

The cult runtime now supports:

- patrol
- suspicious state
- noise investigation
- local search sweeps
- chase
- melee attack
- tower guard behavior during transmission pressure

## 8. What to verify in PIE

Run this exact checklist:

1. Spawn as `MyProjectCharacter`
2. Walk to each pickup and confirm the interact prompt appears
3. Pick up fuse, fuel, and crank
4. Walk to tower and confirm the prompt changes from missing-parts text to install/repair
5. Start repair and confirm:
   - repair progress appears on the HUD
   - threat rises
   - cultists investigate the repair noise
6. Power the tower
7. Trigger transmission
8. Confirm:
   - objective text changes
   - cultists converge harder on the tower
   - extraction becomes active only after transmission completes
9. Reach extraction and hold inside the zone
10. Confirm run ends as a successful escape

Also verify the fail path:

1. Trigger cult pursuit
2. Let a cultist kill the player
3. Confirm the run ends as a failure and reloads

## 9. Common hookup failures

### No interact prompt
- `InteractAction` is not assigned in the player Blueprint/defaults
- pickup or tower mesh is not hittable by `Visibility`
- the player is not using `MyProjectCharacter`

### No movement input
- the map is not using `MyProjectPlayerController`
- the input mapping context is missing from the controller defaults

### No HUD
- the map is not using `IslandGameMode`
- a level override is replacing the HUD/GameMode

### Tower can be used without parts
- check the placed tower actor's `RequiredParts` array
- make sure you did not clear it in a Blueprint instance

### Extraction never works
- transmission must complete first
- the player must remain alive
- the level must contain an `IslandExtractionZone`

### No cultists spawn
- `IslandAISpawnManager` is missing from the map
- `CultistClass` is unset
- there is no reachable navmesh around the spawn origin

## 10. Recommended first Blueprint subclasses

Create these lightweight Blueprints for content hookup:

- `BP_IslandPlayer` from `MyProjectCharacter`
- `BP_Cultist` from `CultistCharacter`
- `BP_RadioTower` from `IslandRadioTower`
- `BP_Pickup_Fuse` / `BP_Pickup_Fuel` / `BP_Pickup_Crank` from `IslandPickupActor`

That gives you asset assignment without changing the runtime authority back out of C++.

### Recommended Blueprint hookups

#### `BP_IslandPlayer`
- assign the input actions used by the slice:
  - `MoveAction`
  - `LookAction`
  - `MouseLookAction`
  - `SprintAction`
  - `InteractAction`
  - optional: `FlashlightAction`
- assign first-person mesh / camera tuning as needed
- keep the island components created in C++ as the runtime authority

#### `BP_Cultist`
- assign skeletal mesh and anim blueprint
- tune movement speeds and attack values in defaults:
  - `PatrolMoveSpeed`
  - `InvestigateMoveSpeed`
  - `ChaseMoveSpeed`
  - `AttackRange`
  - `AttackRadius`
  - `AttackInterval`
- implement these Blueprint events for presentation:
  - `OnCultStateChanged`
  - `OnAttackStarted`
  - `OnAttackConnected`
- use those events for:
  - attack montage playback
  - hit reactions / bark audio
  - suspicious / chase / guard state VFX or posture changes

#### `BP_RadioTower`
- assign mesh, sound, and Niagara assets
- verify prompt visibility and collision
- keep `RequiredParts` aligned with the placed pickups

#### `BP_Pickup_*`
- assign a mesh that clearly communicates the item role
- keep collision hittable by `Visibility`
- customize prompts so the scavenging path reads cleanly in playtests

## 11. Combat feel tuning notes

The cult runtime now supports a more readable melee loop:

- slower patrol pace
- faster chase pace
- local search sweeps at investigation points
- short lunge when entering melee
- facing-sensitive melee confirmation
- Blueprint events for attack start and attack connect

Recommended first tuning pass for `BP_Cultist`:

- `AttackRange`: 140-180
- `AttackRadius`: 90-120
- `AttackInterval`: 1.0-1.4
- `AttackLungeDistance`: 180-240
- `AttackLungeStrength`: 350-500
- `LoseTargetDistance`: 1800-2400

If combat feels too sticky:
- lower `ChaseMoveSpeed`
- lower `AttackLungeStrength`
- raise `AttackInterval`

If combat feels too soft:
- raise `InvestigateMoveSpeed`
- raise `AttackRadius` slightly
- lower `AttackInterval`
