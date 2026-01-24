// RFSN Quest Bridge
// Connects RFSN NPC actions to IslandObjectiveSubsystem

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnQuestBridge.generated.h"

class UIslandObjectiveSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestProgressFromDialogue,
                                             const FString &, ObjectiveTag,
                                             float, ProgressDelta);

/**
 * Bridges RFSN dialogue actions to game objectives.
 * When NPCs take certain actions, objectives can progress.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnQuestBridge : public UActorComponent {
  GENERATED_BODY()

public:
  URfsnQuestBridge();

  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** Objective tag to update when NPC helps */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
  FString HelpObjectiveTag = TEXT("talk_to_npc");

  /** Objective tag for trade actions */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
  FString TradeObjectiveTag = TEXT("trade_with_merchant");

  /** Objective tag for gathering intel */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
  FString IntelObjectiveTag = TEXT("gather_intel");

  // ─────────────────────────────────────────────────────────────
  // Events
  // ─────────────────────────────────────────────────────────────

  UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
  FOnQuestProgressFromDialogue OnQuestProgress;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Bind to RFSN client for automatic quest updates */
  UFUNCTION(BlueprintCallable, Category = "Quest")
  void BindToRfsnClient(URfsnNpcClientComponent *Client);

  /** Manually trigger quest progress */
  UFUNCTION(BlueprintCallable, Category = "Quest")
  void TriggerQuestProgress(const FString &ObjectiveTag, float Progress = 1.0f);

protected:
  virtual void BeginPlay() override;

private:
  UFUNCTION()
  void OnRfsnNpcAction(ERfsnNpcAction Action);

  UIslandObjectiveSubsystem *GetObjectiveSubsystem() const;
};
