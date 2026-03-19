# Island vertical slice v1

## Goal

Deliver one believable, replayable run:

- player spawns on the beach
- player finds fuse, fuel, and crank
- player repairs the tower under pressure
- player powers and transmits
- cultists escalate during transmit
- extraction opens
- player escapes or dies
- optional NPC dialogue still works
- backend outage does not break the run

## Required runtime pieces

- `IslandGameMode`
- `IslandRadioTower`
- `IslandExtractionZone`
- `IslandAISpawnManager`
- three `IslandPickupActor` placements for tower parts
- a player pawn using `AMyProjectCharacter`

## Success criteria

### Win path

1. collect all three parts
2. install/repair tower
3. power tower
4. finish transmit
5. reach extraction and hold long enough to escape

### Failure path

- die to cult pressure
- starve if vitality collapses before extraction

## UI expectation

The first milestone only needs:

- current objective text
- extraction timer / hold progress
- vitality / stealth feedback
- one tutorial prompt at run start

## Non-goals for this slice

- variant gameplay rules
- social simulation owning objectives
- backend-required progression
- large quest/faction economies
