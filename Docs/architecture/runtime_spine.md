# Runtime spine

## Shipping gameplay order

The intended runtime authority for the Island slice is:

`player action -> inventory / stealth / vitality -> tower state -> director alert -> spawn pressure -> extraction window -> run end`

## Ownership rules

### `AIslandGameMode`

`AIslandGameMode` is the run authority.

It owns:

- required actor discovery
- startup validation
- runtime actor binding
- objective updates
- extraction activation routing
- extraction success routing
- player death routing

### `AIslandRadioTower`

The tower is state-only.

It owns:

- state transitions
- repair progress
- timers
- pulse FX/audio
- transmit/extract window timing

It does **not** own objective text.

### `UIslandDirectorSubsystem`

The director stays numeric.

It owns:

- alert accumulation
- alert decay
- intensity transitions
- derived pacing values

It does **not** absorb UI, lore, dialogue orchestration, or social simulation.

### `AIslandAISpawnManager`

The spawn manager is a responder.

It reacts to:

- director intensity
- tower high-pressure states

It does **not** plan story beats or social behavior.

## Extraction / failure routing

- extraction success is emitted by `AIslandExtractionZone`
- `AIslandGameMode` ends the run on extraction success
- `AMyProjectCharacter` routes player death through `AIslandGameMode` when the Island runtime is active

## Optional layers around the spine

### Gameplay-critical optional dialogue seam

- `URfsnNpcClientComponent`
- one-shot dialogue triggers
- lightweight presentation components

### Optional social simulation

Everything under `Social/` is intentionally outside the first playable loop.

### Variants

- `Plugins/IslandVariantHorror/`
- `Plugins/IslandVariantShooter/`

These are isolated from the shipping module and should be treated as opt-in experiments.
