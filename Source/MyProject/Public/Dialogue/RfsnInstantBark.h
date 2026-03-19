// RFSN Instant Bark System - UE5 Component
// Plays pre-recorded barks immediately when action is determined
// Masks LLM latency per Gemini's recommendation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "RfsnInstantBark.generated.h"

class URfsnNpcClientComponent;
class URfsnVoiceRouter;

/**
 * Bark category - maps to NPC actions
 */
UENUM(BlueprintType)
enum class ERfsnBarkCategory : uint8
{
	Greet UMETA(DisplayName = "Greet"),
	Threaten UMETA(DisplayName = "Threaten"),
	Agree UMETA(DisplayName = "Agree"),
	Disagree UMETA(DisplayName = "Disagree"),
	Question UMETA(DisplayName = "Question"),
	Help UMETA(DisplayName = "Help"),
	Trade UMETA(DisplayName = "Trade"),
	Farewell UMETA(DisplayName = "Farewell"),
	Idle UMETA(DisplayName = "Idle"),
	Combat UMETA(DisplayName = "Combat"),
	Surprise UMETA(DisplayName = "Surprise"),
	Grateful UMETA(DisplayName = "Grateful")
};

/**
 * A single instant bark entry
 */
USTRUCT(BlueprintType)
struct FRfsnInstantBarkEntry
{
	GENERATED_BODY()

	/** Text of the bark (for subtitles) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	FString Text;

	/** Pre-recorded audio (optional - uses TTS if null) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	USoundBase* Audio = nullptr;

	/** Duration in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	int32 DurationMs = 500;
};

/**
 * Bark library for an NPC
 */
USTRUCT(BlueprintType)
struct FRfsnBarkLibrary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> GreetBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> ThreatenBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> AgreeBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> DisagreeBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> QuestionBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> HelpBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> TradeBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> FarewellBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> IdleBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> CombatBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> SurpriseBarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	TArray<FRfsnInstantBarkEntry> GratefulBarks;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInstantBarkPlayed, ERfsnBarkCategory, Category, const FString&,
                                             BarkText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBarkComplete);

/**
 * Instant Bark Component
 * Plays pre-recorded barks immediately when an action is received,
 * masking LLM generation latency.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnInstantBark : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnInstantBark();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Enable instant barks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InstantBark|Config")
	bool bEnabled = true;

	/** Auto-bind to RFSN client on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InstantBark|Config")
	bool bAutoBindToClient = true;

	/** Volume multiplier for bark audio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InstantBark|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float VolumeMultiplier = 1.0f;

	/** NPC-specific bark library */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InstantBark|Barks")
	FRfsnBarkLibrary BarkLibrary;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when bark plays */
	UPROPERTY(BlueprintAssignable, Category = "InstantBark|Events")
	FOnInstantBarkPlayed OnBarkPlayed;

	/** Called when bark completes */
	UPROPERTY(BlueprintAssignable, Category = "InstantBark|Events")
	FOnBarkComplete OnBarkComplete;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Play an instant bark for a category */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void PlayBark(ERfsnBarkCategory Category);

	/** Play bark from action string */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void PlayBarkFromAction(const FString& ActionName);

	/** Play bark from server-provided data */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void PlayServerBark(const FString& BarkText, int32 DurationMs);

	/** Map action name to category */
	UFUNCTION(BlueprintPure, Category = "InstantBark")
	static ERfsnBarkCategory ActionToCategory(const FString& ActionName);

	/** Get barks for category */
	UFUNCTION(BlueprintPure, Category = "InstantBark")
	TArray<FRfsnInstantBarkEntry> GetBarksForCategory(ERfsnBarkCategory Category) const;

	/** Stop current bark */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void StopBark();

	/** Is bark currently playing? */
	UFUNCTION(BlueprintPure, Category = "InstantBark")
	bool IsBarkPlaying() const;

	/** Bind to RFSN client */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void BindToRfsnClient(URfsnNpcClientComponent* Client);

	/** Setup default barks */
	UFUNCTION(BlueprintCallable, Category = "InstantBark")
	void SetupDefaultBarks();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Audio component for playback */
	UPROPERTY()
	UAudioComponent* AudioComponent = nullptr;

	/** Current bark index per category (round-robin) */
	TMap<ERfsnBarkCategory, int32> BarkIndices;

	/** Timer handle for bark completion */
	FTimerHandle BarkCompletionTimer;

	/** Currently playing */
	bool bIsPlaying = false;

	/** Handle metadata from RFSN client */
	UFUNCTION()
	void OnRfsnMetaReceived(const struct FRfsnDialogueMeta& Meta);

	/** Get next bark for category (round-robin) */
	FRfsnInstantBarkEntry GetNextBark(ERfsnBarkCategory Category);

	/** Handle audio completion */
	void OnAudioFinished();
};
