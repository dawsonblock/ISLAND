# Island Survival System - Setup Instructions

## Prerequisites

1. **Set the UE_PATH environment variable**

   Open Terminal and run:
   ```bash
   echo 'export UE_PATH="/Users/Shared/Epic Games/UE_5.3"' >> ~/.zshrc
   source ~/.zshrc
   ```
   
   *Replace UE_5.3 with your actual Unreal Engine version folder name*

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

1. **Open your project in Unreal Editor**

2. **Set the GameMode**
   - Project Settings → Maps & Modes
   - Default GameMode → `IslandGameMode`

3. **Place actors in your level**
   - Add `IslandRadioTower` actor
   - Add `IslandExtractionZone` actor
   - Position them appropriately

4. **Configure actors**
   - Select the Radio Tower
   - In Details panel, adjust:
     - Transmit Duration (default: 30s)
     - Extract Window (default: 60s)
     - Cooldown (default: 120s)
     - Pulse Interval (default: 3s)
   
   - Select the Extraction Zone
   - In Details panel, adjust:
     - Hold Time (default: 3s)
     - Box extent (collision volume size)

5. **Test the system**
   - Press Play in Editor
   - Press E while looking at the radio tower
   - Follow the HUD prompts

## How the System Works

1. **Alert System**: Actions increase alert level, which decays over time
2. **Radio Tower**:
   - Unpowered → Power On (requires minimum alert)
   - Powered → Transmit Signal (requires higher alert)
   - Transmitting → Sends pulses, increases alert, attracts enemies
   - Extract Window → Extraction zone becomes active
   - Cooldown → Tower resets

3. **Extraction**: 
   - Only active during Extract Window
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
   - Check that HUDClass is assigned in GameMode constructor

2. **Can't interact with tower**
   - Make sure your character has IslandInteractorComponent
   - Verify interact input is bound
   - Check that tower has collision enabled

3. **Extraction not working**
   - Ensure extraction zone is placed in level
   - Check that tower state reaches ExtractWindow
   - Verify your character implements IslandLifeStateInterface

## Next Steps

- Add AI enemies that respond to alert/objective system
- Create visual feedback for tower states (lights, particles)
- Add sound effects for pulses and state changes
- Implement stamina/health systems
- Add loot and progression systems
