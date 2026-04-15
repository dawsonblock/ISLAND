# Island Survival System - Setup Instructions

> **Note**
> For the current first playable island loop setup, start with
> [`FIRST_PLAYABLE_SLICE_SETUP.md`](./FIRST_PLAYABLE_SLICE_SETUP.md).
> This file still contains older prototype-oriented instructions.

## Prerequisites

1. **Set the UE_PATH environment variable**

   Open Terminal and run:

   ```bash
   echo 'export UE_PATH="/Users/Shared/Epic Games/UE_5.7"' >> ~/.zshrc
   source ~/.zshrc
   ```

   *Replace UE_5.7 with your actual Unreal Engine version folder name*

2. **Verify the environment variable**

   ```bash
   echo $UE_PATH
   ```

   Should print your Unreal Engine path.

## Building from VS Code

1. **Generate project files** (first time only)
   - Open Unreal Editor
   - Tools → Refresh Visual Studio Code Project
   - OR run task: `UE: Generate Project Files`

2. **Build the project**
   - Press `Cmd+Shift+B` (default build)
   - OR Terminal → Run Task → `UE: Build Editor (macOS)`

3. **Clean build** (if needed)
   - Terminal → Run Task → `UE: Clean Build`

## Implementing the Life State Interface

You need to add the interface to your player character. Choose the option that matches your setup:

### Option A: Using bool flags (bDowned, bDead)

In your character header (e.g., MyProjectCharacter.h):

```cpp
#include "IslandLifeStateInterface.h"

UCLASS()
class MYPROJECT_API AMyProjectCharacter : public ACharacter, public IIslandLifeStateInterface
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Life")
    bool bDowned = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Life")
    bool bDead = false;

    virtual bool IsDowned_Implementation() const override { return bDowned; }
    virtual bool IsDead_Implementation() const override { return bDead; }
};
```

### Option B: Using an enum

```cpp
UENUM(BlueprintType)
enum class ELifeState : uint8
{
    Alive,
    Downed,
    Dead
};

#include "IslandLifeStateInterface.h"

UCLASS()
class MYPROJECT_API AMyProjectCharacter : public ACharacter, public IIslandLifeStateInterface
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Life")
    ELifeState LifeState = ELifeState::Alive;

    virtual bool IsDowned_Implementation() const override { return LifeState == ELifeState::Downed; }
    virtual bool IsDead_Implementation() const override { return LifeState == ELifeState::Dead; }
};
```

## Adding Interact Capability to Your Character

In your character header:

```cpp
#include "IslandInteractorComponent.h"

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island")
TObjectPtr<UIslandInteractorComponent> Interactor;
```

In your character constructor:

```cpp
Interactor = CreateDefaultSubobject<UIslandInteractorComponent>(TEXT("Interactor"));
```

### Binding Interact Input

If using Enhanced Input, add this action:

```cpp
void AMyProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Assuming you have an IA_Interact input action
        EnhancedInput->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::OnInteract);
    }
}

void AMyProjectCharacter::OnInteract()
{
    if (Interactor)
    {
        Interactor->TryInteract();
    }
}
```

## Setting Up in Unreal Editor

### Current playable-slice differences

The current island loop is:

1. wake on the beach
2. scavenge `TowerFuse`, `TowerFuel`, and `AntennaCrank`
3. install parts and complete a timed tower repair
4. power the repaired tower
5. transmit the distress signal
6. survive the converging cult pressure
7. extract after transmission completes

### Editor setup

1. **Open your project in Unreal Editor.**

2. **Set the GameMode.**
Project Settings → Maps & Modes.
Default GameMode → `IslandGameMode`.
Do not use `MyProjectGameMode`; it remains only as a legacy prototype surface.

3. **Place actors in your level.**
Add `IslandRadioTower`, `IslandExtractionZone`, and `IslandAISpawnManager`.
Add three `IslandPickupActor` instances for fuse, fuel, and crank, then position them appropriately.

4. **Configure actors.**
For the Radio Tower, adjust Required Parts, Required Repair Time, Repair Noise Per Second, Transmit Duration, Extract Window, Cooldown, and Pulse Interval.
For the Extraction Zone, adjust Hold Time and box extent.
For the AI Spawn Manager, assign `CultistClass` to `CultistCharacter` or a Blueprint subclass.

5. **Test the system.**
Press Play in Editor, collect the three tower parts, return to the tower to start repair, and follow the HUD prompts through transmit and extraction.

## How the System Works

1. **Alert System**: Actions increase alert level, which decays over time
2. **Radio Tower**:
   - Needs Parts → waits for fuse, fuel, and crank
   - Broken/Repairing → timed repair with noise and pressure
   - Unpowered → ready to power on after repair
   - Powered → can transmit distress signal
   - Transmitting → sends pulses, increases pressure, attracts cultists
   - Extract Window → extraction zone becomes active
   - Cooldown → post-window state

3. **Extraction**:
   - Only active after transmission completes
   - Stand in zone for 3 seconds
   - Must be alive and not downed
   - Successfully extracting ends the run (win)

4. **Run Tracking**:
   - Saves total runs, escapes, best time
   - Unlocks clues based on outcomes
   - Restarts level on run end

## Troubleshooting

### Build Errors

1. **"Cannot find UE_PATH"**
   - Make sure you set the environment variable
   - Restart VS Code after setting it

2. **"Missing include files"**
   - Run: Tools → Refresh Visual Studio Code Project
   - Rebuild the project

3. **Linker errors**
   - Check that MyProject.Build.cs has all dependencies
   - Clean build and rebuild

### Runtime Issues

1. **HUD not showing**
   - Verify GameMode is set to IslandGameMode
   - Verify the map was not switched back to legacy `MyProjectGameMode`
   - Check that HUDClass is assigned in GameMode constructor

2. **Can't interact with tower**
   - Make sure your character has IslandInteractorComponent
   - Verify interact input is bound
   - Check that tower has collision enabled
   - Check that the required pickups have been collected first

3. **Extraction not working**
   - Ensure extraction zone is placed in level
   - Check that transmission completed first
   - Verify your character implements IslandLifeStateInterface

## Next Steps

- Tune `BP_Cultist` animations/audio using cult state and attack events
- Create visual feedback for tower states (lights, particles)
- Add sound effects for pulses and state changes
- Expand Blueprint polish around the first playable slice
- Add more level dressing and encounter layouts

---

## RFSN NPC AI Integration

The project includes **RFSN (Reactive Finite State Network)** for LLM-driven NPC dialogue and Director pacing.

### RFSN Prerequisites

1. **Python 3.10+** with virtual environment
2. **RFSN dependencies** installed

### Starting RFSN Server

```bash
cd RFSN_NPC_AI/Python
python -m venv .venv
source .venv/bin/activate  # Mac/Linux
pip install -r requirements.txt
python -m uvicorn orchestrator:app --port 8000
```

### RFSN Components

| Component | Description |
| --------- | ----------- |
| `RfsnNpcClientComponent` | HTTP SSE client for NPC dialogue |
| `RfsnDirectorBridge` | Connects RFSN → IslandDirectorSubsystem |
| `RfsnDialogueWidget` | Typewriter dialogue display |
| `RfsnNpcDialogueTrigger` | Proximity/interact dialogue triggers |
| `RfsnTtsAudioComponent` | TTS audio playback |
| `RfsnPlayerInputWidget` | Player dialogue text input |

### Using RFSN in NPCs

1. **Add component to NPC:**

```cpp
#include "RfsnNpcClientComponent.h"

// In header
UPROPERTY(VisibleAnywhere)
TObjectPtr<URfsnNpcClientComponent> RfsnClient;

// In constructor
RfsnClient = CreateDefaultSubobject<URfsnNpcClientComponent>(TEXT("RfsnClient"));
```

1. **Configure in BeginPlay:**

```cpp
RfsnClient->NpcName = TEXT("Guard");
RfsnClient->Mood = TEXT("Neutral");
RfsnClient->OnNpcActionReceived.AddDynamic(this, &AMyNPC::OnRfsnAction);
```

1. **Trigger dialogue:**

```cpp
RfsnClient->SendPlayerUtterance(TEXT("Hello there!"));
```

### Director Integration

Add `URfsnDirectorBridge` to influence game pacing:

```cpp
UPROPERTY(VisibleAnywhere)
TObjectPtr<URfsnDirectorBridge> DirectorBridge;

// Enable auto-polling
DirectorBridge->bAutoPolling = true;
DirectorBridge->PollInterval = 10.0f;
```

### API Endpoints

| Endpoint | Method | Description |
| -------- | ------ | ----------- |
| `/api/dialogue/stream` | POST | Stream NPC dialogue (SSE) |
| `/api/director/control` | POST | Get pacing commands |
| `/api/health` | GET | Server health check |
