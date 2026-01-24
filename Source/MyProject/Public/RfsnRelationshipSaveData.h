// RFSN Relationship Save Data
// Persists NPC relationship data across sessions

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RfsnRelationshipSaveData.generated.h"

USTRUCT(BlueprintType)
struct FRfsnNpcRelationship {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, Category = "Relationship")
  FString NpcId;

  UPROPERTY(BlueprintReadWrite, Category = "Relationship")
  float Affinity = 0.0f;

  UPROPERTY(BlueprintReadWrite, Category = "Relationship")
  FString Relationship = TEXT("Stranger");

  UPROPERTY(BlueprintReadWrite, Category = "Relationship")
  int32 InteractionCount = 0;

  UPROPERTY(BlueprintReadWrite, Category = "Relationship")
  FDateTime LastInteraction;
};

/**
 * SaveGame class for persisting NPC relationships.
 */
UCLASS()
class MYPROJECT_API URfsnRelationshipSaveData : public USaveGame {
  GENERATED_BODY()

public:
  UPROPERTY(BlueprintReadWrite, Category = "Save")
  TMap<FString, FRfsnNpcRelationship> NpcRelationships;

  UPROPERTY(BlueprintReadWrite, Category = "Save")
  FString PlayerName = TEXT("Player");

  UPROPERTY(BlueprintReadWrite, Category = "Save")
  FDateTime SaveTimestamp;

  /** Get relationship for NPC, creating if doesn't exist */
  UFUNCTION(BlueprintCallable, Category = "Save")
  FRfsnNpcRelationship &GetOrCreateRelationship(const FString &NpcId);

  /** Update relationship from RFSN client */
  UFUNCTION(BlueprintCallable, Category = "Save")
  void UpdateFromClient(const FString &NpcId, float Affinity,
                        const FString &Relationship);
};
