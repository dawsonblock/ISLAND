// RFSN NPC Needs System
// Simulates basic NPC needs (hunger, tiredness, social) that affect behavior
// Integrates with schedule system for natural behavior patterns

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcNeeds.generated.h"

/**
 * Individual need status
 */
USTRUCT(BlueprintType)
struct FRfsnNeed
{
	GENERATED_BODY()

	/** Current value (0 = depleted, 100 = fully satisfied) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0", ClampMax = "100"))
	float Value = 100.0f;

	/** How fast this need decays per game hour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0", ClampMax = "50"))
	float DecayRate = 5.0f;

	/** Threshold below which NPC seeks to satisfy this need */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0", ClampMax = "100"))
	float SeekThreshold = 30.0f;

	/** Critical threshold - NPC behavior is impaired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0", ClampMax = "100"))
	float CriticalThreshold = 15.0f;

	/** Is this need currently critical? */
	bool IsCritical() const { return Value <= CriticalThreshold; }

	/** Should NPC seek to satisfy this? */
	bool NeedsSeeking() const { return Value <= SeekThreshold; }

	/** Satisfy this need by amount */
	void Satisfy(float Amount) { Value = FMath::Clamp(Value + Amount, 0.0f, 100.0f); }

	/** Decay this need */
	void Decay(float Hours) { Value = FMath::Max(0.0f, Value - DecayRate * Hours); }
};

/**
 * NPC mood/state affected by needs
 */
UENUM(BlueprintType)
enum class ERfsnNeedState : uint8
{
	Content UMETA(DisplayName = "Content"),    // All needs satisfied
	Hungry UMETA(DisplayName = "Hungry"),      // Needs food
	Tired UMETA(DisplayName = "Tired"),        // Needs rest
	Lonely UMETA(DisplayName = "Lonely"),      // Needs interaction
	Stressed UMETA(DisplayName = "Stressed"),  // Multiple needs low
	Desperate UMETA(DisplayName = "Desperate") // Critical needs
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNeedStateChanged, ERfsnNeedState, NewState, ERfsnNeedState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedCritical, FName, NeedName);

/**
 * NPC Needs Component
 * Tracks and manages NPC physiological and psychological needs
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcNeeds : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcNeeds();

	// ─────────────────────────────────────────────────────────────
	// Needs Configuration
	// ─────────────────────────────────────────────────────────────

	/** Hunger - need for food */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|Config")
	FRfsnNeed Hunger;

	/** Energy - need for rest/sleep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|Config")
	FRfsnNeed Energy;

	/** Social - need for interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|Config")
	FRfsnNeed Social;

	/** Safety - feeling of security */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|Config")
	FRfsnNeed Safety;

	/** Purpose - need for meaningful activity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|Config")
	FRfsnNeed Purpose;

	// ─────────────────────────────────────────────────────────────
	// System Configuration
	// ─────────────────────────────────────────────────────────────

	/** Enable needs simulation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|System")
	bool bEnabled = true;

	/** Game seconds per real second (time scale) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs|System")
	float TimeScale = 60.0f; // 1 real minute = 1 game hour

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current overall need state */
	UPROPERTY(BlueprintReadOnly, Category = "Needs|State")
	ERfsnNeedState CurrentState = ERfsnNeedState::Content;

	/** Overall wellbeing (0-100 average of all needs) */
	UPROPERTY(BlueprintReadOnly, Category = "Needs|State")
	float OverallWellbeing = 100.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when need state changes */
	UPROPERTY(BlueprintAssignable, Category = "Needs|Events")
	FOnNeedStateChanged OnNeedStateChanged;

	/** Called when a need becomes critical */
	UPROPERTY(BlueprintAssignable, Category = "Needs|Events")
	FOnNeedCritical OnNeedCritical;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Satisfy hunger (e.g., ate food) */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void Feed(float Amount = 50.0f);

	/** Satisfy energy (e.g., slept) */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void Rest(float Amount = 50.0f);

	/** Satisfy social need (e.g., had conversation) */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void Socialize(float Amount = 30.0f);

	/** Boost safety feeling */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void FeelSafe(float Amount = 20.0f);

	/** Reduce safety (e.g., witnessed danger) */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void FeelThreatened(float Amount = 30.0f);

	/** Satisfy purpose (e.g., completed work) */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void Accomplish(float Amount = 25.0f);

	/** Get a specific need by name */
	UFUNCTION(BlueprintPure, Category = "Needs")
	float GetNeedValue(FName NeedName) const;

	/** Check if any need is critical */
	UFUNCTION(BlueprintPure, Category = "Needs")
	bool HasCriticalNeed() const;

	/** Get the most pressing need */
	UFUNCTION(BlueprintPure, Category = "Needs")
	FName GetMostPressingNeed() const;

	/** Get behavior modifier based on needs (-1 to 1) */
	UFUNCTION(BlueprintPure, Category = "Needs")
	float GetBehaviorModifier() const;

	/** Get dialogue tone modifier for LLM */
	UFUNCTION(BlueprintPure, Category = "Needs")
	FString GetNeedsToneModifier() const;

	/** Get needs context for LLM prompt */
	UFUNCTION(BlueprintPure, Category = "Needs")
	FString GetNeedsContext() const;

	/** Apply needs effect to emotion blend */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void ApplyToEmotionBlend();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Time accumulator for decay */
	float TimeAccumulator = 0.0f;

	/** Update needs based on elapsed time */
	void UpdateNeeds(float GameHours);

	/** Calculate overall state from needs */
	ERfsnNeedState CalculateState() const;

	/** Calculate wellbeing */
	void UpdateWellbeing();
};
