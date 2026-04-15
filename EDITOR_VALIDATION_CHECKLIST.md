# Editor Validation Checklist

Use this after building the first playable slice in Unreal Editor.

## Before pressing Play

### Map and mode
- [ ] The active map uses `IslandGameMode`
- [ ] `World Settings` does not override the map back to `MyProjectGameMode` or another legacy prototype mode
- [ ] `GlobalDefaultGameMode` still points at `IslandGameMode`

### Required level actors
- [ ] `IslandRadioTower` is placed
- [ ] `IslandExtractionZone` is placed
- [ ] `IslandAISpawnManager` is placed
- [ ] at least 3 `IslandPickupActor` instances are placed

### Pickup configuration
- [ ] one pickup uses `TowerFuse`
- [ ] one pickup uses `TowerFuel`
- [ ] one pickup uses `AntennaCrank`
- [ ] pickup meshes block `Visibility`
- [ ] pickup prompts are set to readable text

### Cult setup
- [ ] `IslandAISpawnManager.CultistClass` is assigned
- [ ] the cult class is `CultistCharacter` or a Blueprint subclass
- [ ] navmesh exists around the player path, tower area, and likely spawn ring
- [ ] if using attack montages, `AttackMontage` is assigned on the cult Blueprint
- [ ] if using anim notifies, the impact notify calls `TriggerPendingAttackImpact()`

### Player setup
- [ ] the runtime pawn is `MyProjectCharacter` or a Blueprint subclass of it
- [ ] the input mapping context includes `InteractAction`
- [ ] the input mapping context includes `SprintAction`
- [ ] optional: `FlashlightAction` is mapped

### HUD setup
- [ ] if using UMG HUD, `IslandGameMode.IslandHUDWidgetClass` is assigned
- [ ] the widget is a subclass of `IslandHUDWidget`
- [ ] the widget reads from `CurrentState`

### Tower setup
- [ ] `RequiredParts` includes fuse, fuel, and crank
- [ ] `RequiredRepairTime` is tuned to a visible timed repair
- [ ] `TransmitDurationSeconds` and `ExtractWindowSeconds` are set to testable values

## In PIE: interaction and scavenging

- [ ] the HUD appears immediately
- [ ] objective text appears
- [ ] looking at a pickup shows an interact prompt
- [ ] collecting a pickup updates the tower parts readout on the HUD
- [ ] looking at the tower before all parts are found does **not** allow a free repair

## In PIE: tower repair loop

- [ ] after all parts are collected, the tower prompt changes appropriately
- [ ] starting repair shows repair progress on the HUD
- [ ] threat/alert increases during repair
- [ ] repair cancels if the player leaves the tower
- [ ] repair cancels if the player dies
- [ ] repair completes into the unpowered state

## In PIE: cult behavior

- [ ] sprinting causes investigation pressure
- [ ] tower repair noise pulls cultists toward the tower
- [ ] cultists patrol when idle
- [ ] cultists investigate noise
- [ ] cultists search locally after reaching an investigation point
- [ ] cultists chase once they have solid awareness
- [ ] cultists can melee-hit the player
- [ ] cultists guarding the tower stay near it during transmission pressure

## In PIE: transmission and extraction

- [ ] powering the tower works only after repair
- [ ] transmission begins from the powered state
- [ ] objective text updates during transmission
- [ ] transmission pulses raise pressure
- [ ] extraction does **not** activate before transmission completes
- [ ] extraction activates after transmission completes
- [ ] holding in extraction ends the run in success

## In PIE: fail path

- [ ] cult damage can kill the player
- [ ] death ends the run in failure
- [ ] the level reloads after death

## Recommended final spot checks

- [ ] `BP_Cultist` has mesh / anim / audio assigned
- [ ] `BP_Cultist` implements hit / miss / death presentation events if desired
- [ ] `BP_IslandPlayer` has the expected input assets assigned
- [ ] `BP_RadioTower` has tower visuals/audio assigned
- [ ] `BP_IslandHUDWidget` is assigned if using the UMG path
- [ ] there is no accidental dialogue-only critical path blocking the run
