// RFSN Procedural Backstory Generator
// Generates unique NPC backstories on-demand using LLM
// Integrates with faction system and temporal memory

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnBackstoryGenerator.generated.h"

class URfsnNpcClientComponent;
class URfsnFactionSystem;
class URfsnTemporalMemory;

/**
 * A single element of an NPC's backstory
 */
USTRUCT(BlueprintType)
struct FRfsnBackstoryElement
{
	GENERATED_BODY()

	/** Type of element (childhood, trauma, skill, secret, relationship, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory")
	FString ElementType;

	/** Generated description text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory")
	FString Description;

	/** How central this is to personality (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Importance = 0.5f;

	/** Tags for memory matching and dialogue context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory")
	TArray<FString> Tags;

	/** Whether this element should be shared in dialogue readily */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory")
	bool bPublicKnowledge = true;
};

/**
 * Complete NPC backstory structure
 */
USTRUCT(BlueprintType)
struct FRfsnNpcBackstory
{
	GENERATED_BODY()

	/** NPC identifier */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString NpcId;

	/** Two-paragraph summary of the NPC's history */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString Summary;

	/** Current occupation or role */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString Occupation;

	/** How they came to join their faction */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString FactionHistory;

	/** What the NPC wants most */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString PersonalGoal;

	/** What the NPC fears most */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString Fear;

	/** Hidden truth or shame */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString SecretOrShame;

	/** Personality quirk or distinguishing trait */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString DistinguishingTrait;

	/** Detailed backstory elements */
	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	TArray<FRfsnBackstoryElement> Elements;

	/** Generation version for cache invalidation */
	UPROPERTY(BlueprintReadOnly, Category = "Backstory")
	int32 GenerationVersion = 1;

	/** When the backstory was generated */
	UPROPERTY(BlueprintReadOnly, Category = "Backstory")
	FDateTime GeneratedAt;

	/** Check if backstory has been generated */
	bool IsValid() const { return !Summary.IsEmpty(); }
};

/**
 * Backstory generation request parameters
 */
USTRUCT(BlueprintType)
struct FRfsnBackstoryRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString NpcId;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString NpcName;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString FactionId;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	TArray<FString> PersonalityTraits;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString Hint;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString CurrentMood;

	UPROPERTY(BlueprintReadWrite, Category = "Backstory")
	FString Occupation;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackstoryGenerated, const FRfsnNpcBackstory&, Backstory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackstoryError, const FString&, Error);

/**
 * Procedural backstory generation component
 * Generates NPC backstories on first interaction using LLM
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnBackstoryGenerator : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnBackstoryGenerator();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Personality traits to seed backstory generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config")
	TArray<FString> PersonalityTraits;

	/** Optional designer hint for backstory direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config", meta = (MultiLine = true))
	FString BackstoryHint;

	/** NPC's occupation (if blank, will be generated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config")
	FString DefaultOccupation;

	/** RFSN Backstory generation endpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config")
	FString BackstoryEndpoint = TEXT("http://127.0.0.1:8000/api/backstory/generate");

	/** Should load saved backstory on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config")
	bool bLoadOnBeginPlay = true;

	/** Should save backstory after generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstory|Config")
	bool bSaveAfterGeneration = true;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Cached backstory (generated or loaded) */
	UPROPERTY(BlueprintReadOnly, Category = "Backstory|State")
	FRfsnNpcBackstory CachedBackstory;

	/** Whether generation is in progress */
	UPROPERTY(BlueprintReadOnly, Category = "Backstory|State")
	bool bIsGenerating = false;

	/** Whether this NPC has had first interaction */
	UPROPERTY(BlueprintReadOnly, Category = "Backstory|State")
	bool bHasInteracted = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when backstory generation completes */
	UPROPERTY(BlueprintAssignable, Category = "Backstory|Events")
	FOnBackstoryGenerated OnBackstoryGenerated;

	/** Called on generation error */
	UPROPERTY(BlueprintAssignable, Category = "Backstory|Events")
	FOnBackstoryError OnBackstoryError;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Generate backstory asynchronously. Called automatically on first interaction. */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	void GenerateBackstory();

	/** Mark that first interaction has occurred (triggers generation if no backstory) */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	void OnFirstInteraction();

	/** Check if backstory exists */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	bool HasBackstory() const { return CachedBackstory.IsValid(); }

	/** Get dialogue context string for LLM prompts */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	FString GetDialogueContext() const;

	/** Get short context (1-2 sentences) for quick prompts */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	FString GetShortContext() const;

	/** Get a specific element by type */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	FString GetElementByType(const FString& Type) const;

	/** Get elements by tag */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	TArray<FRfsnBackstoryElement> GetElementsByTag(const FString& Tag) const;

	/** Save backstory to disk */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	void SaveBackstory();

	/** Load backstory from disk */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	bool LoadBackstory();

	/** Clear cached backstory (forces regeneration on next interaction) */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	void ClearBackstory();

	/** Check if save exists for this NPC */
	UFUNCTION(BlueprintPure, Category = "Backstory")
	bool DoesSaveExist() const;

	// ─────────────────────────────────────────────────────────────
	// Integration
	// ─────────────────────────────────────────────────────────────

	/** Seed temporal memory with backstory-relevant memories */
	UFUNCTION(BlueprintCallable, Category = "Backstory")
	void SeedTemporalMemory(URfsnTemporalMemory* Memory);

protected:
	virtual void BeginPlay() override;

private:
	/** Reference to sibling RFSN client */
	UPROPERTY()
	URfsnNpcClientComponent* RfsnClient;

	/** Build request JSON for LLM */
	FString BuildRequestJson(const FRfsnBackstoryRequest& Request) const;

	/** Parse response JSON into backstory */
	bool ParseBackstoryResponse(const FString& JsonResponse, FRfsnNpcBackstory& OutBackstory);

	/** Generate fallback backstory if LLM fails */
	FRfsnNpcBackstory GenerateFallbackBackstory() const;

	/** Get save slot name for this NPC */
	FString GetSaveSlotName() const;

	/** HTTP request callback */
	void OnBackstoryRequestComplete(bool bSuccess, const FString& Response);
};
