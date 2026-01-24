// RFSN Reputation HUD Component
// Displays faction reputation status for the player

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnReputationHud.generated.h"

class URfsnFactionSystem;

/**
 * Single faction reputation display data
 */
USTRUCT(BlueprintType)
struct FRfsnFactionDisplay
{
	GENERATED_BODY()

	/** Faction identifier */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	FString FactionId;

	/** Display name */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	FString DisplayName;

	/** Current reputation value (-100 to 100) */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	float Reputation = 0.0f;

	/** Reputation tier text */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	FString TierText;

	/** Color based on reputation */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	FLinearColor TierColor;

	/** Normalized 0-1 value for progress bars (maps -100..100 to 0..1) */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	float NormalizedValue = 0.5f;

	/** Icon/symbol (faction-specific) */
	UPROPERTY(BlueprintReadOnly, Category = "Faction")
	FString IconSymbol;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReputationDisplayUpdated, const TArray<FRfsnFactionDisplay>&, Factions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReputationChanged, const FString&, FactionId, float, NewReputation);

/**
 * Reputation HUD Component
 * Provides data for displaying faction reputation in the UI
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnReputationHud : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnReputationHud();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Factions to track (empty = all factions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation|Config")
	TArray<FString> TrackedFactions;

	/** Show notification on reputation change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation|Config")
	bool bShowChangeNotifications = true;

	/** Minimum reputation change to trigger notification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation|Config")
	float NotificationThreshold = 5.0f;

	/** How long to show the HUD after a change (0 = always visible) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation|Config")
	float AutoHideDelay = 5.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Whether HUD is currently visible */
	UPROPERTY(BlueprintReadOnly, Category = "Reputation|State")
	bool bIsVisible = false;

	/** Time remaining before auto-hide */
	UPROPERTY(BlueprintReadOnly, Category = "Reputation|State")
	float HideTimer = 0.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when faction display data updates */
	UPROPERTY(BlueprintAssignable, Category = "Reputation|Events")
	FOnReputationDisplayUpdated OnReputationDisplayUpdated;

	/** Called when a specific reputation changes significantly */
	UPROPERTY(BlueprintAssignable, Category = "Reputation|Events")
	FOnReputationChanged OnReputationChanged;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get all faction display data */
	UFUNCTION(BlueprintPure, Category = "Reputation")
	TArray<FRfsnFactionDisplay> GetAllFactionDisplayData() const { return CachedFactionData; }

	/** Get specific faction display data */
	UFUNCTION(BlueprintPure, Category = "Reputation")
	FRfsnFactionDisplay GetFactionDisplayData(const FString& FactionId) const;

	/** Force refresh of faction data */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void RefreshFactionData();

	/** Show the reputation HUD */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void ShowHud();

	/** Hide the reputation HUD */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void HideHud();

	/** Toggle HUD visibility */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void ToggleHud();

	/** Get tier color for reputation value */
	UFUNCTION(BlueprintPure, Category = "Reputation")
	static FLinearColor GetTierColor(float Reputation);

	/** Get tier text for reputation value */
	UFUNCTION(BlueprintPure, Category = "Reputation")
	static FString GetTierText(float Reputation);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Cached faction display data */
	UPROPERTY()
	TArray<FRfsnFactionDisplay> CachedFactionData;

	/** Previous reputation values for change detection */
	TMap<FString, float> PreviousReputations;

	/** Get faction icon symbol */
	static FString GetFactionIcon(const FString& FactionId);

	/** Get faction display name */
	static FString GetFactionDisplayName(const FString& FactionId);
};
