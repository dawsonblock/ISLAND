# Island Survival System - File Structure

> **Note**
> This reference predates the current first playable slice runtime.
> For the live hookup and runtime expectations, use
> [`FIRST_PLAYABLE_SLICE_SETUP.md`](./FIRST_PLAYABLE_SLICE_SETUP.md).

## Created Files

### VS Code Configuration
```
.vscode/
  └── tasks.json                              # Build tasks for macOS
```

### Public Headers (Source/MyProject/Public/)
```
Public/
  ├── IslandLifeStateInterface.h              # Interface for character life state
  ├── IslandDirectorSubsystem.h               # World subsystem for alert management
  ├── IslandObjectiveSubsystem.h              # World subsystem for AI objectives
  ├── IslandRadioTower.h                      # Radio tower actor
  ├── IslandExtractionZone.h                  # Extraction zone actor
  ├── IslandRunSaveGame.h                     # Save game data structure
  ├── IslandGameInstanceSubsystem.h           # GameInstance subsystem for run management
  ├── IslandGameMode.h                        # GameMode that wires everything together
  ├── IslandInteractableInterface.h           # Interface for interactable objects
  ├── IslandInteractorComponent.h             # Component for player interaction
  └── IslandHUD.h                             # HUD for displaying game state
```

### Private Implementation (Source/MyProject/Private/)
```
Private/
  ├── IslandDirectorSubsystem.cpp
  ├── IslandObjectiveSubsystem.cpp
  ├── IslandRadioTower.cpp
  ├── IslandExtractionZone.cpp
  ├── IslandGameInstanceSubsystem.cpp
  ├── IslandGameMode.cpp
  ├── IslandInteractorComponent.cpp
  └── IslandHUD.cpp
```

### Modified Files
```
Source/MyProject/MyProject.Build.cs          # Updated with required dependencies
```

## System Architecture

### Subsystems

**UIslandDirectorSubsystem** (WorldSubsystem)
- Manages alert level (0-100%)
- Alert decays over time
- Gates tower activation based on alert thresholds

**UIslandObjectiveSubsystem** (WorldSubsystem)
- Tracks active objective location
- Used by AI to prioritize targets

**UIslandGameInstanceSubsystem** (GameInstanceSubsystem)
- Tracks run time, seed, stats
- Handles save/load of progression
- Manages run start/end and level restart

### Actors

**AIslandRadioTower**
- States: NeedsParts → Broken/Repairing → Unpowered → Powered → Transmitting → ExtractWindow → Cooldown
- Implements IIslandInteractableInterface
- Consumes real tower parts from the player inventory
- Runs a timed, noisy repair sequence
- Sends alert pulses during transmission
- Broadcasts state change events

**AIslandExtractionZone**
- BoxComponent for overlap detection
- Only active during extract window
- Requires 3-second hold to extract
- Checks player eligibility via IIslandLifeStateInterface

**AIslandGameMode**
- Auto-finds tower and extraction actors
- Binds tower state to extraction activation
- Starts run on BeginPlay
- Updates run timer every frame

**AIslandHUD**
- Displays alert level
- Shows tower state
- Shows required tower parts and repair progress
- Shows player stealth noise / visibility
- Shows extraction timer
- Shows interact prompts

### Components

**UIslandInteractorComponent**
- Line traces from camera
- Detects IIslandInteractableInterface
- Shows interaction prompts
- Handles interaction input

### Interfaces

**IIslandLifeStateInterface**
- IsDowned() - checks if character is downed
- IsDead() - checks if character is dead
- Must be implemented by player character

**IIslandInteractableInterface**
- CanInteract() - checks if interaction is allowed
- Interact() - performs the interaction
- GetInteractPrompt() - returns UI text

## Data Flow

### Game Loop
```
1. Run Start
   └─> IslandGameInstanceSubsystem::StartRun()
       └─> Generate seed, reset timer

2. Gameplay
   ├─> Player scavenges fuse, fuel, and crank
   ├─> Player movement and tower work create noise / alert
   └─> Cultists investigate, search, chase, and attack

3. Tower Interaction
   ├─> Player presses E → IslandInteractorComponent::TryInteract()
   ├─> Tower consumes required parts
   ├─> Timed repair completes → State: Unpowered
   ├─> Player powers tower → State: Powered
   └─> Player transmits → StartTransmit()

4. Transmission Phase
   ├─> Tower sends pulses every 3s
   ├─> Each pulse adds alert and noise pressure
   ├─> Objective becomes active at tower/extraction locations
   └─> After 30s → ExtractWindow state

5. Extraction Phase
   ├─> Extraction zone activates only after transmission completes
   ├─> Player enters zone
   ├─> Hold for 3 seconds
   └─> Success → EndRun(true)

6. Run End
   └─> IslandGameInstanceSubsystem::EndRun()
       ├─> Save stats
       ├─> Unlock clues
       └─> Restart level
```

### Alert System
```
IslandDirectorSubsystem
  ├─> AlertLevel (0-100)
  ├─> MinAlertForTower (30)
  ├─> MinAlertForTransmit (50)
  ├─> AlertDecayRate (5/sec)
  └─> AddAlert(Amount)
```

### Tower State Machine
```
NeedsParts
  └─> [Player has fuse/fuel/crank] → Broken

Broken
  └─> [Player Interact] → Repairing

Repairing
  ├─> [Timed repair completes] → Unpowered
  └─> [Player leaves / dies] → Broken

Unpowered
  └─> [Player Interact] → Powered

Powered
  └─> [Player Interact] → Transmitting

Transmitting
  ├─> Sends pulses (every 3s)
  ├─> Objective active + cult convergence pressure
  └─> [After 30s] → ExtractWindow

ExtractWindow
  ├─> Extraction zone active
  └─> [After 60s] → Cooldown

Cooldown
  └─> [Manual reset or timer]
```

## Dependencies (MyProject.Build.cs)

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "AIModule",              // For AI behavior
    "StateTreeModule",       // Existing
    "GameplayStateTreeModule", // Existing
    "GameplayTasks",         // Required for subsystems
    "NavigationSystem",      // For AI navigation
    "UMG",                   // For UI
    "Slate",                 // For HUD
    "SlateCore"              // For HUD
});
```

## Usage Examples

### Adding the Interface to Your Character

```cpp
// MyProjectCharacter.h
#include "IslandLifeStateInterface.h"

UCLASS()
class AMyProjectCharacter : public ACharacter, public IIslandLifeStateInterface
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDowned = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDead = false;
    
    virtual bool IsDowned_Implementation() const override { return bDowned; }
    virtual bool IsDead_Implementation() const override { return bDead; }
};
```

### Adding Interaction to Your Character

```cpp
// MyProjectCharacter.h
#include "IslandInteractorComponent.h"

UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
TObjectPtr<UIslandInteractorComponent> Interactor;

// MyProjectCharacter.cpp - Constructor
Interactor = CreateDefaultSubobject<UIslandInteractorComponent>(TEXT("Interactor"));

// Input binding
void AMyProjectCharacter::OnInteract()
{
    if (Interactor)
        Interactor->TryInteract();
}
```

### Checking Alert Level in Blueprints

```
Get World Subsystem (Island Director Subsystem)
  └─> Get Alert Level
```

### Triggering Death

```cpp
void AMyProjectCharacter::Die()
{
    bDead = true;
    
    if (UGameInstance* GI = GetGameInstance())
    {
        if (auto* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
        {
            Run->EndRun(false); // false = death/failure
        }
    }
}
```

## Testing Checklist

- [ ] Project compiles successfully
- [ ] IslandGameMode is set as default game mode
- [ ] Radio tower placed in level
- [ ] Extraction zone placed in level
- [ ] AI spawn manager placed in level
- [ ] Fuse, fuel, and crank pickups placed in level
- [ ] Player can collect all three parts
- [ ] Player can look at tower and see the correct prompt
- [ ] Pressing E after collecting parts starts timed repair
- [ ] Repair can complete into the unpowered state
- [ ] Pressing E after repair powers the tower
- [ ] Pressing E again starts transmission
- [ ] HUD shows alert level
- [ ] HUD shows tower state, repair progress, and parts state
- [ ] Transmission pulses increase alert
- [ ] After transmission, extraction zone activates
- [ ] HUD shows extraction timer
- [ ] Cultists patrol, investigate, chase, and attack
- [ ] Standing in zone for 3s ends run
- [ ] Player death ends the run
- [ ] Level restarts after extraction
