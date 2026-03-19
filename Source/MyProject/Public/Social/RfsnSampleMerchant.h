// RFSN Sample NPC - Merchant with trading dialogue
// Demonstrates RFSN client configuration for a friendly trader

#pragma once

#include "CoreMinimal.h"
#include "MyProjectCharacter.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnNpcDialogueTrigger.h"
#include "RfsnSampleMerchant.generated.h"

/**
 * Sample merchant NPC with RFSN dialogue.
 * Configured for friendly trading interactions.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ARfsnSampleMerchant : public AMyProjectCharacter {
  GENERATED_BODY()

public:
  ARfsnSampleMerchant();

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RFSN")
  TObjectPtr<URfsnNpcClientComponent> RfsnClient;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RFSN")
  TObjectPtr<URfsnNpcDialogueTrigger> DialogueTrigger;

  /** Merchant inventory items for trade dialogue */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
  TArray<FString> InventoryItems;

protected:
  virtual void BeginPlay() override;

  UFUNCTION()
  void OnRfsnAction(ERfsnNpcAction Action);

  UFUNCTION()
  void OnDialogueSentence(const FRfsnSentence &Sentence);
};
