// RFSN NPC Dialogue Trigger
// Triggers NPC dialogue when player interacts or enters proximity

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnNpcDialogueTrigger.generated.h"

class URfsnNpcClientComponent;
class URfsnDialogueWidget;
class USphereComponent;

UENUM(BlueprintType)
enum class ERfsnDialogueTriggerMode : uint8 {
  Interact,      // Trigger on player interaction (E key)
  Proximity,     // Trigger when player enters range
  ProximityOnce, // Trigger once when player enters range
  Manual         // Only trigger via Blueprint/code
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueTriggerActivated,
                                            const FString &, InitialPrompt);

/**
 * Component that manages when NPC dialogue is triggered.
 * Supports interaction-based and proximity-based triggers.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcDialogueTrigger : public UActorComponent {
  GENERATED_BODY()

public:
  URfsnNpcDialogueTrigger();

  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** How dialogue is triggered */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
  ERfsnDialogueTriggerMode TriggerMode = ERfsnDialogueTriggerMode::Interact;

  /** Proximity radius for proximity-based triggers */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
            meta = (ClampMin = "50.0", ClampMax = "2000.0"))
  float ProximityRadius = 300.0f;

  /** Default prompt sent when dialogue starts */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
  FString DefaultPrompt = TEXT("Hello");

  /** Cooldown between dialogue triggers (seconds) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
            meta = (ClampMin = "0.0"))
  float TriggerCooldown = 3.0f;

  /** Reference to the RFSN client (auto-found if not set) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
  TObjectPtr<URfsnNpcClientComponent> RfsnClient;

  /** Reference to dialogue widget (auto-created if not set) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
  TObjectPtr<URfsnDialogueWidget> DialogueWidget;

  // ─────────────────────────────────────────────────────────────
  // Events
  // ─────────────────────────────────────────────────────────────

  /** Called when dialogue is triggered */
  UPROPERTY(BlueprintAssignable, Category = "Trigger|Events")
  FOnDialogueTriggerActivated OnTriggerActivated;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Manually trigger dialogue with custom prompt */
  UFUNCTION(BlueprintCallable, Category = "Trigger")
  void TriggerDialogue(const FString &PlayerPrompt);

  /** Manually trigger dialogue with default prompt */
  UFUNCTION(BlueprintCallable, Category = "Trigger")
  void TriggerDefaultDialogue();

  /** Check if trigger is on cooldown */
  UFUNCTION(BlueprintPure, Category = "Trigger")
  bool IsOnCooldown() const;

  /** Check if player is in proximity */
  UFUNCTION(BlueprintPure, Category = "Trigger")
  bool IsPlayerInProximity() const;

  /** Called when player attempts interaction (bind to input) */
  UFUNCTION(BlueprintCallable, Category = "Trigger")
  void OnPlayerInteract();

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

private:
  bool bPlayerInProximity = false;
  bool bProximityTriggered = false;
  float LastTriggerTime = -999.0f;

  TObjectPtr<APawn> PlayerPawn;

  void FindComponents();
  void CheckProximity();
  bool CanTrigger() const;
};
