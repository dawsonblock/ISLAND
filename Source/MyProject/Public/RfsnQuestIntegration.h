// RFSN Quest Integration System
// Connects NPC dialogue with quest state
// Enables quest-aware conversations and dynamic objective updates

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnQuestIntegration.generated.h"

/**
 * Quest status
 */
UENUM(BlueprintType)
enum class ERfsnQuestStatus : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),     // NPC doesn't know about it
	Available UMETA(DisplayName = "Available"), // Can be started
	Active UMETA(DisplayName = "Active"),       // In progress
	Completed UMETA(DisplayName = "Completed"), // Finished successfully
	Failed UMETA(DisplayName = "Failed"),       // Failed
	Abandoned UMETA(DisplayName = "Abandoned")  // Player abandoned
};

/**
 * Quest objective data
 */
USTRUCT(BlueprintType)
struct FRfsnQuestObjective
{
	GENERATED_BODY()

	/** Objective identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString ObjectiveId;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString Description;

	/** Current progress */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 CurrentProgress = 0;

	/** Required progress */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 RequiredProgress = 1;

	/** Is this optional? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bOptional = false;

	/** Is completed? */
	bool IsComplete() const { return CurrentProgress >= RequiredProgress; }
};

/**
 * Quest data
 */
USTRUCT(BlueprintType)
struct FRfsnQuest
{
	GENERATED_BODY()

	/** Quest identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString QuestId;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString DisplayName;

	/** Short description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString Description;

	/** Quest giver NPC ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString QuestGiverNpcId;

	/** Current status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	ERfsnQuestStatus Status = ERfsnQuestStatus::Unknown;

	/** Objectives */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FRfsnQuestObjective> Objectives;

	/** Dialogue hint for when quest is available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString AvailableHint;

	/** Dialogue hint for when quest is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString ActiveHint;

	/** Dialogue hint for when quest is complete */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString CompletionHint;

	/** Check if all required objectives are complete */
	bool IsComplete() const
	{
		for (const FRfsnQuestObjective& Obj : Objectives)
		{
			if (!Obj.bOptional && !Obj.IsComplete())
			{
				return false;
			}
		}
		return true;
	}
};

/**
 * NPC's knowledge about a quest
 */
USTRUCT(BlueprintType)
struct FRfsnNpcQuestKnowledge
{
	GENERATED_BODY()

	/** Quest ID */
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	FString QuestId;

	/** Is this NPC the quest giver? */
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	bool bIsQuestGiver = false;

	/** Is this NPC a quest target? */
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	bool bIsQuestTarget = false;

	/** Can provide info about this quest? */
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	bool bCanProvideInfo = false;

	/** Custom dialogue hint */
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	FString DialogueHint;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestStatusChanged, const FString&, QuestId, ERfsnQuestStatus,
                                             NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectiveProgress, const FString&, QuestId, const FString&,
                                             ObjectiveId);

/**
 * Quest Integration Component
 * Connects NPC dialogue with quest system
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnQuestIntegration : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnQuestIntegration();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Quests this NPC offers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Config")
	TArray<FRfsnQuest> OfferedQuests;

	/** Quests this NPC knows about */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Config")
	TArray<FRfsnNpcQuestKnowledge> QuestKnowledge;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestStatusChanged OnQuestStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnObjectiveProgress OnObjectiveProgress;

	// ─────────────────────────────────────────────────────────────
	// API - Quest Management
	// ─────────────────────────────────────────────────────────────

	/** Start a quest */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool StartQuest(const FString& QuestId);

	/** Complete a quest */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CompleteQuest(const FString& QuestId);

	/** Fail a quest */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool FailQuest(const FString& QuestId);

	/** Update objective progress */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool UpdateObjective(const FString& QuestId, const FString& ObjectiveId, int32 Progress = 1);

	/** Get quest status */
	UFUNCTION(BlueprintPure, Category = "Quest")
	ERfsnQuestStatus GetQuestStatus(const FString& QuestId) const;

	/** Get quest by ID */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FRfsnQuest GetQuest(const FString& QuestId) const;

	/** Get available quests from this NPC */
	UFUNCTION(BlueprintPure, Category = "Quest")
	TArray<FRfsnQuest> GetAvailableQuests() const;

	/** Get active quests from this NPC */
	UFUNCTION(BlueprintPure, Category = "Quest")
	TArray<FRfsnQuest> GetActiveQuests() const;

	// ─────────────────────────────────────────────────────────────
	// API - Dialogue Integration
	// ─────────────────────────────────────────────────────────────

	/** Get quest context for LLM prompt */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FString GetQuestContext() const;

	/** Get dialogue hint for a specific quest */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FString GetQuestDialogueHint(const FString& QuestId) const;

	/** Check if NPC has quests to offer */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasAvailableQuests() const;

	/** Check if NPC has active quests to discuss */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasActiveQuests() const;

	/** Check if NPC knows about a quest */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool KnowsAboutQuest(const FString& QuestId) const;

	/** Get information NPC can share about a quest */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FString GetQuestInfo(const FString& QuestId) const;

	/** Parse player dialogue for quest-related keywords */
	UFUNCTION(BlueprintPure, Category = "Quest")
	TArray<FString> DetectQuestTopics(const FString& PlayerDialogue) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Find quest in offered quests */
	FRfsnQuest* FindQuest(const FString& QuestId);
	const FRfsnQuest* FindQuest(const FString& QuestId) const;

	/** Find NPC knowledge about quest */
	const FRfsnNpcQuestKnowledge* FindQuestKnowledge(const FString& QuestId) const;
};
