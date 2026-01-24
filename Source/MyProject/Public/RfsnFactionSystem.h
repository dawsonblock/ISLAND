// RFSN Faction System
// Manages group-based reputation and NPC relationships

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RfsnFactionSystem.generated.h"

USTRUCT(BlueprintType)
struct FRfsnFaction
{
	GENERATED_BODY()

	/** Unique faction ID */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString FactionId;

	/** Display name */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString DisplayName;

	/** Player reputation with this faction (-100 to 100) */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	float Reputation = 0.0f;

	/** Default mood for NPCs in this faction */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString DefaultMood = TEXT("Neutral");

	/** Other factions this faction is allied with */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	TArray<FString> Allies;

	/** Other factions this faction is hostile to */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	TArray<FString> Enemies;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFactionReputationChanged, const FString&, FactionId, float,
                                             NewReputation);

/**
 * Game Instance Subsystem for managing factions and group reputation.
 */
UCLASS()
class MYPROJECT_API URfsnFactionSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Faction|Events")
	FOnFactionReputationChanged OnFactionReputationChanged;

	// ─────────────────────────────────────────────────────────────
	// Subsystem Interface
	// ─────────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ─────────────────────────────────────────────────────────────
	// Faction Management
	// ─────────────────────────────────────────────────────────────

	/** Register a new faction */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void RegisterFaction(const FRfsnFaction& Faction);

	/** Get faction by ID */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool GetFaction(const FString& FactionId, FRfsnFaction& OutFaction);

	/** Get all factions */
	UFUNCTION(BlueprintPure, Category = "Faction")
	TArray<FRfsnFaction> GetAllFactions() const;

	// ─────────────────────────────────────────────────────────────
	// Reputation API
	// ─────────────────────────────────────────────────────────────

	/** Get player reputation with faction (-100 to 100) */
	UFUNCTION(BlueprintPure, Category = "Faction")
	float GetReputation(const FString& FactionId) const;

	/** Modify reputation by delta */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void ModifyReputation(const FString& FactionId, float Delta);

	/** Set reputation to specific value */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetReputation(const FString& FactionId, float Value);

	/** Get reputation tier (Hostile, Unfriendly, Neutral, Friendly, Allied) */
	UFUNCTION(BlueprintPure, Category = "Faction")
	FString GetReputationTier(const FString& FactionId) const;

	// ─────────────────────────────────────────────────────────────
	// Faction Relations
	// ─────────────────────────────────────────────────────────────

	/** Check if two factions are allied */
	UFUNCTION(BlueprintPure, Category = "Faction")
	bool AreFactionsAllied(const FString& FactionA, const FString& FactionB) const;

	/** Check if two factions are enemies */
	UFUNCTION(BlueprintPure, Category = "Faction")
	bool AreFactionsHostile(const FString& FactionA, const FString& FactionB) const;

	/** Get effective NPC affinity based on faction reputation */
	UFUNCTION(BlueprintPure, Category = "Faction")
	float GetNpcAffinityFromFaction(const FString& FactionId) const;

protected:
	UPROPERTY()
	TMap<FString, FRfsnFaction> Factions;

private:
	void CreateDefaultFactions();
};
