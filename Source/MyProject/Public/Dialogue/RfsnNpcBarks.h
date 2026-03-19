// RFSN NPC Barks System
// Context-aware one-liner comments and reactions
// Triggered by world events, player actions, or idle states

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcBarks.generated.h"

/**
 * Bark trigger context
 */
UENUM(BlueprintType)
enum class ERfsnBarkTrigger : uint8
{
	Idle UMETA(DisplayName = "Idle"),                // Random idle chatter
	PlayerNear UMETA(DisplayName = "Player Near"),   // Player approaches
	PlayerLeave UMETA(DisplayName = "Player Leave"), // Player walks away
	Combat UMETA(DisplayName = "Combat"),            // Combat situation
	Danger UMETA(DisplayName = "Danger"),            // Threat detected
	Discovery UMETA(DisplayName = "Discovery"),      // Found something
	Weather UMETA(DisplayName = "Weather"),          // Weather comment
	TimeOfDay UMETA(DisplayName = "Time of Day"),    // Morning, night, etc
	QuestRelated UMETA(DisplayName = "Quest"),       // Quest progress
	Trade UMETA(DisplayName = "Trade"),              // Trading interaction
	Greeting UMETA(DisplayName = "Greeting"),        // Hello to player
	Farewell UMETA(DisplayName = "Farewell"),        // Goodbye
	Pain UMETA(DisplayName = "Pain"),                // Damage taken
	Victory UMETA(DisplayName = "Victory"),          // Enemy defeated
	Frustrated UMETA(DisplayName = "Frustrated"),    // Something annoying
	Custom UMETA(DisplayName = "Custom")             // Custom trigger
};

/**
 * Single bark entry
 */
USTRUCT(BlueprintType)
struct FRfsnBark
{
	GENERATED_BODY()

	/** Trigger for this bark */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	ERfsnBarkTrigger Trigger = ERfsnBarkTrigger::Idle;

	/** The bark text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	FString Text;

	/** Custom trigger tag (for Custom trigger) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	FString CustomTag;

	/** Priority (higher = more likely to play) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Priority = 5;

	/** Cooldown between uses (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	float Cooldown = 30.0f;

	/** Last time this bark was used */
	float LastUsedTime = -1000.0f;

	/** Can use this bark? */
	bool IsAvailable(float CurrentTime) const { return CurrentTime - LastUsedTime >= Cooldown; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBarkTriggered, ERfsnBarkTrigger, Trigger, const FString&, Text);

/**
 * NPC Barks Component
 * Manages context-aware one-liners
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcBarks : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcBarks();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** All available barks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config")
	TArray<FRfsnBark> Barks;

	/** Global cooldown between any bark */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config")
	float GlobalCooldown = 5.0f;

	/** Chance to bark on trigger (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config", meta = (ClampMin = "0", ClampMax = "1"))
	float BarkChance = 0.6f;

	/** Enable idle barks? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config")
	bool bEnableIdleBarks = true;

	/** Idle bark interval (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config")
	float IdleBarkInterval = 60.0f;

	/** Max hearing distance for barks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Config")
	float HearingRange = 500.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Last bark time */
	UPROPERTY(BlueprintReadOnly, Category = "Barks|State")
	float LastBarkTime = 0.0f;

	/** Current bark (if speaking) */
	UPROPERTY(BlueprintReadOnly, Category = "Barks|State")
	FString CurrentBark;

	/** Is currently barking? */
	UPROPERTY(BlueprintReadOnly, Category = "Barks|State")
	bool bIsBarking = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Barks|Events")
	FOnBarkTriggered OnBarkTriggered;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Try to trigger a bark of specific type */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	bool TryBark(ERfsnBarkTrigger Trigger, bool bForce = false);

	/** Trigger a specific bark by text */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void SayBark(const FString& Text);

	/** Trigger custom bark by tag */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	bool TryCustomBark(const FString& CustomTag);

	/** Add a bark dynamically */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void AddBark(ERfsnBarkTrigger Trigger, const FString& Text, int32 Priority = 5, float Cooldown = 30.0f);

	/** Clear all barks of a type */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void ClearBarks(ERfsnBarkTrigger Trigger);

	/** Get random available bark for trigger */
	UFUNCTION(BlueprintPure, Category = "Barks")
	FString GetRandomBark(ERfsnBarkTrigger Trigger) const;

	/** Check if can bark (cooldown) */
	UFUNCTION(BlueprintPure, Category = "Barks")
	bool CanBark() const;

	/** Is player in hearing range? */
	UFUNCTION(BlueprintPure, Category = "Barks")
	bool IsPlayerInRange() const;

	/** Setup default barks */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void SetupDefaultBarks();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Idle bark timer */
	float IdleTimer = 0.0f;

	/** Select best bark from available */
	FRfsnBark* SelectBark(ERfsnBarkTrigger Trigger);
};
