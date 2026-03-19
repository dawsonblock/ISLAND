// RFSN Relationship Manager Subsystem
// Handles saving and loading NPC relationships across sessions

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RfsnRelationshipSaveData.h"
#include "RfsnRelationshipManager.generated.h"

class URfsnNpcClientComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRelationshipChanged, const FString&, NpcId, const FRfsnNpcRelationship&,
                                             Relationship);

/**
 * Game Instance Subsystem for managing NPC relationships.
 * Persists relationships to disk and syncs with RFSN clients.
 */
UCLASS()
class MYPROJECT_API URfsnRelationshipManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Save slot name for relationship data */
	UPROPERTY(BlueprintReadWrite, Category = "RFSN|Persistence")
	FString SaveSlotName = TEXT("RfsnRelationships");

	/** User index for save */
	UPROPERTY(BlueprintReadWrite, Category = "RFSN|Persistence")
	int32 SaveUserIndex = 0;

	/** Auto-save after each relationship change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Persistence")
	bool bAutoSave = true;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRelationshipChanged OnRelationshipChanged;

	// ─────────────────────────────────────────────────────────────
	// Subsystem Interface
	// ─────────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ─────────────────────────────────────────────────────────────
	// Relationship API
	// ─────────────────────────────────────────────────────────────

	/** Get relationship for NPC by ID */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	FRfsnNpcRelationship GetRelationship(const FString& NpcId);

	/** Update relationship from NPC client */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	void UpdateRelationship(const FString& NpcId, float Affinity, const FString& Relationship);

	/** Register NPC client - syncs its state with saved data */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	void RegisterNpcClient(URfsnNpcClientComponent* Client);

	/** Unregister NPC client */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	void UnregisterNpcClient(URfsnNpcClientComponent* Client);

	/** Modify affinity by delta (clamped -1 to 1) */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	void ModifyAffinity(const FString& NpcId, float Delta);

	/** Set relationship type (Friend, Enemy, Stranger, etc.) */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	void SetRelationshipType(const FString& NpcId, const FString& RelationshipType);

	// ─────────────────────────────────────────────────────────────
	// Save/Load API
	// ─────────────────────────────────────────────────────────────

	/** Save all relationships to disk */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Persistence")
	bool SaveRelationships();

	/** Load relationships from disk */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Persistence")
	bool LoadRelationships();

	/** Check if save exists */
	UFUNCTION(BlueprintPure, Category = "RFSN|Persistence")
	bool DoesSaveExist() const;

	/** Clear all saved relationships */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Persistence")
	void ClearSavedRelationships();

	/** Get all NPC IDs with saved relationships */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Relationships")
	TArray<FString> GetAllNpcIds() const;

protected:
	UPROPERTY()
	TObjectPtr<URfsnRelationshipSaveData> SaveData;

	UPROPERTY()
	TArray<TWeakObjectPtr<URfsnNpcClientComponent>> RegisteredClients;

private:
	void SyncClientFromSaveData(URfsnNpcClientComponent* Client);
	void SyncSaveDataFromClient(URfsnNpcClientComponent* Client);
};
