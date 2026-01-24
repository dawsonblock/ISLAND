// RFSN Ambient Chatter Component
// Enables NPCs to have idle dialogue and react to world events

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnAmbientChatter.generated.h"

class URfsnNpcClientComponent;

UENUM(BlueprintType)
enum class ERfsnChatterTrigger : uint8
{
	/** Random timer-based */
	Idle,
	/** Player enters radius */
	PlayerNearby,
	/** Another NPC nearby */
	NpcNearby,
	/** Combat started */
	CombatStart,
	/** Low health */
	LowHealth,
	/** World event */
	WorldEvent
};

USTRUCT(BlueprintType)
struct FRfsnChatterLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	FString Line;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	ERfsnChatterTrigger Trigger = ERfsnChatterTrigger::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	float Weight = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatterTriggered, const FString&, Line, ERfsnChatterTrigger, Trigger);

/**
 * Component for NPC ambient dialogue and reactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnAmbientChatter : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnAmbientChatter();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	bool bEnabled = true;

	/** Minimum time between idle chatter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter|Timing")
	float MinIdleInterval = 30.0f;

	/** Maximum time between idle chatter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter|Timing")
	float MaxIdleInterval = 120.0f;

	/** Radius to detect player for nearby trigger */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	float PlayerDetectionRadius = 500.0f;

	/** Pre-defined chatter lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter|Lines")
	TArray<FRfsnChatterLine> ChatterLines;

	/** Use RFSN for generating chatter instead of predefined */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	bool bUseRfsnForChatter = false;

	/** Context for RFSN-generated chatter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	FString RfsnChatterContext = TEXT("idle observation");

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Chatter|Events")
	FOnChatterTriggered OnChatterTriggered;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Trigger chatter of specific type */
	UFUNCTION(BlueprintCallable, Category = "Chatter")
	void TriggerChatter(ERfsnChatterTrigger Trigger);

	/** Add a chatter line at runtime */
	UFUNCTION(BlueprintCallable, Category = "Chatter")
	void AddChatterLine(const FString& Line, ERfsnChatterTrigger Trigger, float Weight = 1.0f);

	/** Play a specific line */
	UFUNCTION(BlueprintCallable, Category = "Chatter")
	void SayLine(const FString& Line);

	/** Start idle chatter timer */
	UFUNCTION(BlueprintCallable, Category = "Chatter")
	void StartIdleChatter();

	/** Stop idle chatter timer */
	UFUNCTION(BlueprintCallable, Category = "Chatter")
	void StopIdleChatter();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	float IdleTimer = 0.0f;
	float NextIdleTime = 0.0f;
	bool bIdleChatterActive = false;

	FString SelectRandomLine(ERfsnChatterTrigger Trigger);
	void ResetIdleTimer();
	bool IsPlayerNearby() const;
};
