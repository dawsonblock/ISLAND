// RFSN Relationship Decay System
// Time-based decay and maintenance of NPC relationships
// Relationships weaken without interaction, can be boosted by gifts/favors

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnRelationshipDecay.generated.h"

class URfsnNpcClientComponent;

/**
 * Relationship standing level
 */
UENUM(BlueprintType)
enum class ERfsnRelationshipTier : uint8
{
	Hostile UMETA(DisplayName = "Hostile"),       // -100 to -60
	Unfriendly UMETA(DisplayName = "Unfriendly"), // -60 to -20
	Neutral UMETA(DisplayName = "Neutral"),       // -20 to 20
	Friendly UMETA(DisplayName = "Friendly"),     // 20 to 60
	Trusted UMETA(DisplayName = "Trusted"),       // 60 to 100
	BestFriend UMETA(DisplayName = "Best Friend") // 100+ (no decay)
};

/**
 * Relationship bonus event
 */
USTRUCT(BlueprintType)
struct FRfsnRelationshipBonus
{
	GENERATED_BODY()

	/** Reason for bonus */
	UPROPERTY(BlueprintReadWrite, Category = "Relationship")
	FString Reason;

	/** Amount added */
	UPROPERTY(BlueprintReadWrite, Category = "Relationship")
	float Amount = 0.0f;

	/** When applied */
	UPROPERTY(BlueprintReadWrite, Category = "Relationship")
	FDateTime Timestamp;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRelationshipDecayed, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRelationshipTierChanged, ERfsnRelationshipTier, OldTier,
                                             ERfsnRelationshipTier, NewTier);

/**
 * Relationship Decay Component
 * Manages relationship degradation over time
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnRelationshipDecay : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnRelationshipDecay();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Base decay rate per game day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float DecayRatePerDay = 2.0f;

	/** Decay starts after this many game hours without interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float DecayGracePeriodHours = 48.0f;

	/** Positive relationships decay slower */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float PositiveDecayMultiplier = 0.5f;

	/** Negative relationships decay faster toward neutral */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float NegativeDecayMultiplier = 1.5f;

	/** Minimum value (floor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float MinValue = -100.0f;

	/** Maximum value (ceiling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float MaxValue = 100.0f;

	/** Value at which relationship becomes "locked" (best friend, no decay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay|Config")
	float LockThreshold = 100.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current relationship value */
	UPROPERTY(BlueprintReadOnly, Category = "Decay|State")
	float CurrentValue = 0.0f;

	/** Current tier */
	UPROPERTY(BlueprintReadOnly, Category = "Decay|State")
	ERfsnRelationshipTier CurrentTier = ERfsnRelationshipTier::Neutral;

	/** Game hours since last interaction */
	UPROPERTY(BlueprintReadOnly, Category = "Decay|State")
	float HoursSinceInteraction = 0.0f;

	/** Is relationship locked (best friend)? */
	UPROPERTY(BlueprintReadOnly, Category = "Decay|State")
	bool bIsLocked = false;

	/** Recent bonuses */
	UPROPERTY(BlueprintReadOnly, Category = "Decay|State")
	TArray<FRfsnRelationshipBonus> RecentBonuses;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Decay|Events")
	FOnRelationshipDecayed OnRelationshipDecayed;

	UPROPERTY(BlueprintAssignable, Category = "Decay|Events")
	FOnRelationshipTierChanged OnTierChanged;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Modify relationship value */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void ModifyRelationship(float Amount, const FString& Reason = TEXT(""));

	/** Record interaction (resets grace period) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void RecordInteraction();

	/** Give a gift (positive bonus) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void GiveGift(float Value = 10.0f);

	/** Do a favor (larger positive bonus) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void DoFavor(float Value = 25.0f);

	/** Insult (negative change) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void Insult(float Value = -15.0f);

	/** Betray (major negative change) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void Betray(float Value = -50.0f);

	/** Tick decay (call with game hours elapsed) */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void TickDecay(float GameHoursElapsed);

	/** Get current tier */
	UFUNCTION(BlueprintPure, Category = "Decay")
	ERfsnRelationshipTier GetTier() const { return CurrentTier; }

	/** Get tier from value */
	UFUNCTION(BlueprintPure, Category = "Decay")
	static ERfsnRelationshipTier ValueToTier(float Value);

	/** Get tier as string */
	UFUNCTION(BlueprintPure, Category = "Decay")
	static FString TierToString(ERfsnRelationshipTier Tier);

	/** Get relationship context for LLM */
	UFUNCTION(BlueprintPure, Category = "Decay")
	FString GetRelationshipContext() const;

	/** Get decay warning if applicable */
	UFUNCTION(BlueprintPure, Category = "Decay")
	FString GetDecayWarning() const;

	/** Set initial value */
	UFUNCTION(BlueprintCallable, Category = "Decay")
	void SetInitialValue(float Value);

protected:
	virtual void BeginPlay() override;

private:
	/** Update tier from current value */
	void UpdateTier();

	/** Calculate effective decay rate */
	float CalculateDecayRate() const;
};
