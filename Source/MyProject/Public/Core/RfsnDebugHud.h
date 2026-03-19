// RFSN Debug HUD Component
// Displays debug info about active RFSN state

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnDebugHud.generated.h"

class URfsnDialogueManager;
class URfsnNpcClientComponent;

/**
 * Component that displays RFSN debug information on screen.
 * Shows active NPC, last action, bandit key, and server status.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnDebugHud : public UActorComponent {
  GENERATED_BODY()

public:
  URfsnDebugHud();

  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** Enable debug display */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
  bool bEnabled = false;

  /** Screen position X */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
  float ScreenX = 10.0f;

  /** Screen position Y */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
  float ScreenY = 50.0f;

  // ─────────────────────────────────────────────────────────────
  // Debug State
  // ─────────────────────────────────────────────────────────────

  /** Last NPC action received */
  UPROPERTY(BlueprintReadOnly, Category = "Debug")
  FString LastNpcAction;

  /** Last bandit key */
  UPROPERTY(BlueprintReadOnly, Category = "Debug")
  FString LastBanditKey;

  /** Last player signal */
  UPROPERTY(BlueprintReadOnly, Category = "Debug")
  FString LastPlayerSignal;

  /** RFSN server status */
  UPROPERTY(BlueprintReadOnly, Category = "Debug")
  bool bServerConnected = false;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Toggle debug display */
  UFUNCTION(BlueprintCallable, Category = "Debug")
  void ToggleDebug() { bEnabled = !bEnabled; }

  /** Update from meta event */
  UFUNCTION(BlueprintCallable, Category = "Debug")
  void UpdateFromMeta(const FString &NpcAction, const FString &PlayerSignal,
                      const FString &BanditKey);

  /** Draw debug info (call from HUD DrawHUD) */
  void DrawDebugInfo(class AHUD *HUD);

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

private:
  FString GetActiveNpcName() const;
};
