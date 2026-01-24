// RFSN Blueprint Function Library
// Static helper functions exposed to Blueprint for easy RFSN access

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnBlueprintLibrary.generated.h"

class URfsnDialogueManager;
class URfsnNpcClientComponent;

/**
 * Blueprint function library for RFSN integration.
 * Provides easy access to common RFSN operations.
 */
UCLASS()
class MYPROJECT_API URfsnBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Dialogue Management
	// ─────────────────────────────────────────────────────────────

	/** Start dialogue with an NPC actor */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue", meta = (WorldContext = "WorldContextObject"))
	static bool StartDialogueWithNpc(const UObject* WorldContextObject, AActor* NpcActor);

	/** End current dialogue */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue", meta = (WorldContext = "WorldContextObject"))
	static void EndDialogue(const UObject* WorldContextObject);

	/** Send player message to active NPC */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue", meta = (WorldContext = "WorldContextObject"))
	static void SendPlayerMessage(const UObject* WorldContextObject, const FString& Message);

	/** Check if dialogue is currently active */
	UFUNCTION(BlueprintPure, Category = "RFSN|Dialogue", meta = (WorldContext = "WorldContextObject"))
	static bool IsDialogueActive(const UObject* WorldContextObject);

	/** Get currently engaged NPC */
	UFUNCTION(BlueprintPure, Category = "RFSN|Dialogue", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetActiveDialogueNpc(const UObject* WorldContextObject);

	// ─────────────────────────────────────────────────────────────
	// NPC Discovery
	// ─────────────────────────────────────────────────────────────

	/** Find nearest NPC with RFSN client component */
	UFUNCTION(BlueprintCallable, Category = "RFSN|NPC", meta = (WorldContext = "WorldContextObject"))
	static AActor* FindNearestRfsnNpc(const UObject* WorldContextObject, FVector Location, float MaxDistance = 300.0f);

	/** Get all NPCs with RFSN client in world */
	UFUNCTION(BlueprintCallable, Category = "RFSN|NPC", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> GetAllRfsnNpcs(const UObject* WorldContextObject);

	/** Get RFSN client component from actor */
	UFUNCTION(BlueprintPure, Category = "RFSN|NPC")
	static URfsnNpcClientComponent* GetRfsnClient(AActor* Actor);

	// ─────────────────────────────────────────────────────────────
	// NPC Configuration
	// ─────────────────────────────────────────────────────────────

	/** Set NPC mood */
	UFUNCTION(BlueprintCallable, Category = "RFSN|NPC")
	static void SetNpcMood(AActor* NpcActor, const FString& Mood);

	/** Set NPC relationship */
	UFUNCTION(BlueprintCallable, Category = "RFSN|NPC")
	static void SetNpcRelationship(AActor* NpcActor, const FString& Relationship);

	/** Set NPC affinity */
	UFUNCTION(BlueprintCallable, Category = "RFSN|NPC")
	static void SetNpcAffinity(AActor* NpcActor, float Affinity);

	// ─────────────────────────────────────────────────────────────
	// Utilities
	// ─────────────────────────────────────────────────────────────

	/** Check if RFSN server is reachable */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Utility", meta = (WorldContext = "WorldContextObject"))
	static void CheckServerHealth(const UObject* WorldContextObject);

	/** Get RFSN server URL from config */
	UFUNCTION(BlueprintPure, Category = "RFSN|Utility")
	static FString GetRfsnServerUrl();

	/** Convert action enum to string */
	UFUNCTION(BlueprintPure, Category = "RFSN|Utility")
	static FString ActionToString(ERfsnNpcAction Action);

private:
	static URfsnDialogueManager* GetDialogueManager(const UObject* WorldContextObject);
};
