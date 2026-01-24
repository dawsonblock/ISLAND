// RFSN NPC Awareness System
// Detection and perception for stealth gameplay
// Includes sight, sound, and investigation behaviors

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcAwareness.generated.h"

/**
 * Awareness level of NPC toward a target
 */
UENUM(BlueprintType)
enum class ERfsnAwarenessLevel : uint8
{
	Unaware UMETA(DisplayName = "Unaware"),             // No detection
	Suspicious UMETA(DisplayName = "Suspicious"),       // Heard/saw something
	Investigating UMETA(DisplayName = "Investigating"), // Actively searching
	Alerted UMETA(DisplayName = "Alerted"),             // Fully aware of target
	Hostile UMETA(DisplayName = "Hostile")              // Attacking
};

/**
 * Sensory detection event
 */
USTRUCT(BlueprintType)
struct FRfsnDetectionEvent
{
	GENERATED_BODY()

	/** Location of detection */
	UPROPERTY(BlueprintReadWrite, Category = "Detection")
	FVector Location = FVector::ZeroVector;

	/** Strength of detection (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Detection")
	float Strength = 0.0f;

	/** Type of detection */
	UPROPERTY(BlueprintReadWrite, Category = "Detection")
	FString Type = TEXT("Visual");

	/** Detected actor (if known) */
	UPROPERTY(BlueprintReadWrite, Category = "Detection")
	AActor* DetectedActor = nullptr;

	/** Time when detected */
	UPROPERTY(BlueprintReadWrite, Category = "Detection")
	float Timestamp = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAwarenessChanged, ERfsnAwarenessLevel, NewLevel, ERfsnAwarenessLevel,
                                             OldLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetDetected, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSuspiciousSound, FVector, Location);

/**
 * NPC Awareness Component
 * Handles detection, investigation, and alertness
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcAwareness : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcAwareness();

	// ─────────────────────────────────────────────────────────────
	// Sight Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum sight distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Sight")
	float SightRange = 1500.0f;

	/** Field of view in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Sight",
	          meta = (ClampMin = "10", ClampMax = "180"))
	float FieldOfView = 90.0f;

	/** Peripheral vision range (lower detection in this zone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Sight")
	float PeripheralFOV = 140.0f;

	/** Does darkness affect sight? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Sight")
	bool bAffectedByDarkness = true;

	// ─────────────────────────────────────────────────────────────
	// Hearing Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum hearing distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Hearing")
	float HearingRange = 800.0f;

	/** Sensitivity to sounds (multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Hearing")
	float HearingSensitivity = 1.0f;

	// ─────────────────────────────────────────────────────────────
	// Detection Configuration
	// ─────────────────────────────────────────────────────────────

	/** How fast awareness builds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float AwarenessGainRate = 0.5f;

	/** How fast awareness decays when no detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float AwarenessDecayRate = 0.2f;

	/** Threshold to become suspicious */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float SuspiciousThreshold = 0.3f;

	/** Threshold to start investigating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float InvestigateThreshold = 0.6f;

	/** Threshold to become fully alerted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float AlertedThreshold = 0.9f;

	/** How long to stay alerted after losing sight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness|Detection")
	float AlertDuration = 10.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current awareness level */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	ERfsnAwarenessLevel CurrentAwareness = ERfsnAwarenessLevel::Unaware;

	/** Current awareness value (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	float AwarenessValue = 0.0f;

	/** Currently tracked target */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	AActor* CurrentTarget = nullptr;

	/** Last known target location */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	FVector LastKnownLocation = FVector::ZeroVector;

	/** Time since last detection */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	float TimeSinceDetection = 0.0f;

	/** Is currently seeing target? */
	UPROPERTY(BlueprintReadOnly, Category = "Awareness|State")
	bool bCanSeeTarget = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when awareness level changes */
	UPROPERTY(BlueprintAssignable, Category = "Awareness|Events")
	FOnAwarenessChanged OnAwarenessChanged;

	/** Called when target is first fully detected */
	UPROPERTY(BlueprintAssignable, Category = "Awareness|Events")
	FOnTargetDetected OnTargetDetected;

	/** Called when suspicious sound is heard */
	UPROPERTY(BlueprintAssignable, Category = "Awareness|Events")
	FOnSuspiciousSound OnSuspiciousSound;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Check if can see a specific actor */
	UFUNCTION(BlueprintCallable, Category = "Awareness")
	bool CanSeeActor(AActor* Target);

	/** Check if can hear a sound at location */
	UFUNCTION(BlueprintCallable, Category = "Awareness")
	bool CanHearSound(FVector SoundLocation, float SoundLoudness = 1.0f);

	/** Report a sound for this NPC to potentially hear */
	UFUNCTION(BlueprintCallable, Category = "Awareness")
	void ReportSound(FVector SoundLocation, float Loudness, AActor* Source = nullptr);

	/** Immediately alert to target */
	UFUNCTION(BlueprintCallable, Category = "Awareness")
	void AlertToTarget(AActor* Target);

	/** Reset awareness to unaware */
	UFUNCTION(BlueprintCallable, Category = "Awareness")
	void ResetAwareness();

	/** Get investigation location */
	UFUNCTION(BlueprintPure, Category = "Awareness")
	FVector GetInvestigationLocation() const;

	/** Is NPC hostile? */
	UFUNCTION(BlueprintPure, Category = "Awareness")
	bool IsHostile() const { return CurrentAwareness == ERfsnAwarenessLevel::Hostile; }

	/** Is NPC alerted or above? */
	UFUNCTION(BlueprintPure, Category = "Awareness")
	bool IsAlerted() const { return CurrentAwareness >= ERfsnAwarenessLevel::Alerted; }

	/** Get awareness context for LLM */
	UFUNCTION(BlueprintPure, Category = "Awareness")
	FString GetAwarenessContext() const;

	/** Get awareness as string */
	UFUNCTION(BlueprintPure, Category = "Awareness")
	static FString AwarenessToString(ERfsnAwarenessLevel Level);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Recent detection events */
	TArray<FRfsnDetectionEvent> RecentEvents;

	/** Update visual detection */
	void UpdateVisualDetection(float DeltaTime);

	/** Update awareness level from value */
	void UpdateAwarenessLevel();

	/** Calculate visibility of target */
	float CalculateVisibility(AActor* Target) const;

	/** Line trace for visibility */
	bool HasLineOfSight(AActor* Target) const;

	/** Check if location is in FOV */
	bool IsInFieldOfView(FVector Location) const;
};
