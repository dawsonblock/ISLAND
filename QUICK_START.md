# Island Survival System - Quick Start Guide

## 🚀 Quick Setup (5 minutes)

### 1. Set Up Environment (One-time)

Run the setup script:
```bash
cd "/Users/dawsonblock/Documents/Unreal Projects/MyProject"
./setup_environment.sh
source ~/.zshrc
```

Or manually add to `~/.zshrc`:
```bash
export UE_PATH="/Users/Shared/Epic Games/UE_5.3"
```

### 2. Build the Project

In VS Code:
- Press `Cmd+Shift+B` to build
- OR run task: "UE: Build Editor (macOS)"

Expected output: "Build succeeded" or similar

### 3. Add Life State to Your Character

Open your player character header (e.g., `MyProjectCharacter.h`):

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

### 4. Add Interaction Component

In your character header:
```cpp
#include "IslandInteractorComponent.h"

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island")
    TObjectPtr<UIslandInteractorComponent> Interactor;
```

In your character constructor (`.cpp`):
```cpp
Interactor = CreateDefaultSubobject<UIslandInteractorComponent>(TEXT("Interactor"));
```

In your input setup:
```cpp
void AMyProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    // Add your interact binding
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMyProjectCharacter::OnInteract);
}

void AMyProjectCharacter::OnInteract()
{
    if (Interactor)
    {
        Interactor->TryInteract();
    }
}
```

### 5. Rebuild After Character Changes

```bash
# In VS Code
Cmd+Shift+B
```

### 6. Configure in Unreal Editor

1. Open Unreal Editor
2. **Set GameMode**:
   - Edit → Project Settings → Maps & Modes
   - Default GameMode → `IslandGameMode`

3. **Place Actors**:
   - Place Actors panel → search "IslandRadioTower"
   - Drag into level
   - Place Actors panel → search "IslandExtractionZone"
   - Drag into level

4. **Test**:
   - Press Play
   - Look at tower
   - See "[E] Power Radio" prompt
   - Press E to interact

---

## 📋 What Got Created

### Core Systems
- ✅ **Alert System** - Tracks player threat level
- ✅ **Director Subsystem** - Manages alert decay
- ✅ **Objective Subsystem** - For AI targeting
- ✅ **Radio Tower** - Multi-state objective with interaction
- ✅ **Extraction Zone** - Win condition with timer
- ✅ **Run Management** - Save/load, stats tracking
- ✅ **GameMode** - Wires everything together
- ✅ **HUD** - Shows alert, tower state, extraction timer
- ✅ **Interaction System** - Press E to use objects

### Files Created (20 files)
- 10 header files (`.h`) in `Source/MyProject/Public/`
- 8 implementation files (`.cpp`) in `Source/MyProject/Private/`
- 1 build configuration (`.vscode/tasks.json`)
- 1 Build.cs update

---

## 🎮 How It Works

### The Loop
1. **Early Game**: Survive, alert is low
2. **Mid Game**: Alert rises, can power tower
3. **Tower Phase**: Interact to power, then transmit
4. **Transmission**: 30s of pulses, attracts enemies
5. **Extract Window**: 60s to reach extraction zone
6. **Win/Lose**: Extract successfully or die trying

### Key Mechanics
- **Alert Level**: 0-100%, gates tower usage
- **Tower States**: Unpowered → Powered → Transmitting → ExtractWindow → Cooldown
- **Extraction**: Hold in zone for 3 seconds
- **Eligibility**: Must be alive and not downed

---

## 🔧 VS Code Workflow

### Build Commands
```bash
# Default build (Cmd+Shift+B)
Cmd+Shift+B

# Clean build
Terminal → Run Task → "UE: Clean Build"

# Generate project files (after adding files)
Terminal → Run Task → "UE: Generate Project Files"
```

### Common Tasks
- **Add new C++ class**: Create in `Source/MyProject/Public/` or `Private/`
- **After adding files**: Run "Generate Project Files" task
- **Build errors**: Try clean build, then rebuild
- **IntelliSense issues**: Regenerate project files

---

## 🐛 Troubleshooting

### Build Fails: "Cannot find UE_PATH"
```bash
# Check if set
echo $UE_PATH

# If empty, run setup script again
./setup_environment.sh
source ~/.zshrc

# Restart VS Code
```

### Build Fails: Missing includes
```bash
# Regenerate project files
Tools → Refresh Visual Studio Code Project (in Unreal Editor)
# Then rebuild
```

### HUD Not Showing
- Verify GameMode is `IslandGameMode`
- Check Project Settings → Maps & Modes
- Ensure HUD is enabled in PlayerController

### Can't Interact with Tower
- Character must have `IslandInteractorComponent`
- Input must be bound to call `TryInteract()`
- Tower must have collision enabled
- Must be within 350 units of tower

### Extraction Not Working
- Tower must complete transmission (30s)
- Extraction zone must be placed in level
- Character must implement `IslandLifeStateInterface`
- Must stand in zone for 3 seconds

---

## 📚 Documentation Files

1. **SETUP_INSTRUCTIONS.md** - Detailed setup guide
2. **ISLAND_SYSTEM_REFERENCE.md** - Complete system architecture
3. **This file** - Quick start guide

---

## 🎯 Next Steps

### Essential
- [ ] Implement interface on your character
- [ ] Bind interact input
- [ ] Test basic loop (power → transmit → extract)

### Polish
- [ ] Add visual feedback to tower (lights, particles)
- [ ] Add sound effects for pulses
- [ ] Style the HUD (UMG widgets)
- [ ] Add extraction progress indicator

### Gameplay
- [ ] AI enemies that respond to alert
- [ ] Loot system
- [ ] Health/stamina
- [ ] Multiple towers/extraction points

---

## 💡 Tips

- **Alert Management**: Keep alert low early game
- **Timing**: Plan extraction route before transmitting
- **Safety**: Clear extraction zone area before transmitting
- **Testing**: Use HUD to monitor all systems

---

## ✨ Features Summary

| Feature | Status | Location |
|---------|--------|----------|
| Alert System | ✅ | IslandDirectorSubsystem |
| Radio Tower | ✅ | IslandRadioTower |
| Extraction | ✅ | IslandExtractionZone |
| Interaction | ✅ | IslandInteractorComponent |
| HUD | ✅ | IslandHUD |
| Run Tracking | ✅ | IslandGameInstanceSubsystem |
| Save System | ✅ | IslandRunSaveGame |
| Life State | ✅ | IslandLifeStateInterface |

---

**You're all set! Build the project and start testing.** 🎉
