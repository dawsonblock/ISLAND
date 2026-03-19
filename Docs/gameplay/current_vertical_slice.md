# Current vertical slice

## Core loop

The active playable loop in the repo is:

`spawn -> collect fuse/fuel/crank -> repair tower -> power/transmit -> survive pressure -> extract or die`

## Required map wiring

For the current slice to function, the map should provide:

- `IslandGameMode` as the active game mode
- one `IslandRadioTower`
- one `IslandExtractionZone`
- one `IslandAISpawnManager`
- three `IslandPickupActor` placements configured for:
  - fuse
  - fuel
  - crank

## Player-facing flow

1. Spawn into the island map.
2. Scavenge the three tower parts.
3. Return to the tower and install/repair it.
4. Power the tower and start transmission.
5. Survive cultist pressure during transmit.
6. Reach extraction once the extraction window opens.

## Current ownership notes

- `AIslandGameMode` already updates objective text and tracks tower/extraction actors, but startup validation is still implicit.
- `AIslandRadioTower` still mutates objective state directly during transmit, so objective ownership is duplicated.
- `AIslandAISpawnManager` responds to tower/director state and remains part of the primary shipping loop.

## Minimum placed-actor checklist

If the slice fails to start cleanly, verify:

- tower actor exists and is discoverable
- extraction actor exists and is discoverable
- spawn manager exists
- player pawn/controller classes still resolve from `AIslandGameMode`
- tutorial/HUD widgets are available for the opening prompt
