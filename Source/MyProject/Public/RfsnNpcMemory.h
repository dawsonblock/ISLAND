// RFSN NPC Memory System
// Persistent memory of conversations and interactions with players
// Allows NPCs to reference past discussions and relationship history

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcMemory.generated.h"

/**
 * Type of memorable interaction
 */
UENUM(BlueprintType)
enum class ERfsnMemoryType : uint8
{
	Conversation UMETA(DisplayName = "Conversation"), // Had a dialogue
	Trade UMETA(DisplayName = "Trade"),               // Made a deal
	Gift UMETA(DisplayName = "Gift"),                 // Received a gift
	Favor UMETA(DisplayName = "Favor"),               // Did a favor
	Insult UMETA(DisplayName = "Insult"),             // Was insulted
	Combat UMETA(DisplayName = "Combat"),             // Fought together/against
	Quest UMETA(DisplayName = "Quest"),               // Quest related
	Promise UMETA(DisplayName = "Promise"),           // Made a promise
	Betrayal UMETA(DisplayName = "Betrayal"),         // Was betrayed
	FirstMeeting UMETA(DisplayName = "First Meeting") // Initial encounter
};

/**
 * Single memory entry
 */
USTRUCT(BlueprintType)
struct FRfsnMemoryEntry
{
	GENERATED_BODY()

	/** Unique memory ID */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	FGuid MemoryId;

	/** Memory type */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	ERfsnMemoryType Type = ERfsnMemoryType::Conversation;

	/** Short summary of what happened */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	FString Summary;

	/** Key topics discussed or involved */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	TArray<FString> Topics;

	/** Emotional impact (-1 to 1) */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	float EmotionalImpact = 0.0f;

	/** Memory importance (affects forgetting) */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	float Importance = 0.5f;

	/** When this happened (game time) */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	float GameTimeWhenOccurred = 0.0f;

	/** Real world timestamp */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	FDateTime RealTimeWhenOccurred;

	/** How many times has NPC thought about this? (reinforcement) */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	int32 ReinforcementCount = 1;

	/** Memory strength (decays over time) */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	float Strength = 1.0f;

	/** Associated player/NPC ID */
	UPROPERTY(BlueprintReadWrite, Category = "Memory")
	FString AssociatedEntityId;

	/** Constructor */
	FRfsnMemoryEntry()
	{
		MemoryId = FGuid::NewGuid();
		RealTimeWhenOccurred = FDateTime::Now();
	}
};

/**
 * Conversation snapshot
 */
USTRUCT(BlueprintType)
struct FRfsnConversationSnapshot
{
	GENERATED_BODY()

	/** When conversation started */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FDateTime StartTime;

	/** Player statements (most recent) */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	TArray<FString> PlayerStatements;

	/** NPC responses (most recent) */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	TArray<FString> NpcResponses;

	/** Topics detected in conversation */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	TArray<FString> DetectedTopics;

	/** Overall sentiment of conversation */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	float OverallSentiment = 0.0f;

	/** Max lines to keep per conversation */
	int32 MaxLinesPerSide = 5;

	void AddPlayerStatement(const FString& Statement)
	{
		PlayerStatements.Add(Statement);
		if (PlayerStatements.Num() > MaxLinesPerSide)
		{
			PlayerStatements.RemoveAt(0);
		}
	}

	void AddNpcResponse(const FString& Response)
	{
		NpcResponses.Add(Response);
		if (NpcResponses.Num() > MaxLinesPerSide)
		{
			NpcResponses.RemoveAt(0);
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryCreated, const FRfsnMemoryEntry&, Memory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryRecalled, const FRfsnMemoryEntry&, Memory);

/**
 * NPC Memory Component
 * Stores and manages memories of past interactions
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcMemory : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcMemory();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum memories to retain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Config")
	int32 MaxMemories = 50;

	/** Memory decay rate per game hour (0 = never forget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Config")
	float MemoryDecayRate = 0.01f;

	/** Minimum strength before memory is forgotten */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Config")
	float ForgetThreshold = 0.1f;

	/** Auto-save memories? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Config")
	bool bAutoSave = true;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** All stored memories */
	UPROPERTY(BlueprintReadOnly, Category = "Memory|State")
	TArray<FRfsnMemoryEntry> Memories;

	/** Current conversation (if in dialogue) */
	UPROPERTY(BlueprintReadOnly, Category = "Memory|State")
	FRfsnConversationSnapshot CurrentConversation;

	/** Is NPC in conversation? */
	UPROPERTY(BlueprintReadOnly, Category = "Memory|State")
	bool bInConversation = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when new memory is created */
	UPROPERTY(BlueprintAssignable, Category = "Memory|Events")
	FOnMemoryCreated OnMemoryCreated;

	/** Called when a memory is recalled */
	UPROPERTY(BlueprintAssignable, Category = "Memory|Events")
	FOnMemoryRecalled OnMemoryRecalled;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Create a new memory */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	FGuid CreateMemory(ERfsnMemoryType Type, const FString& Summary, float EmotionalImpact = 0.0f,
	                   float Importance = 0.5f);

	/** Add topic to a memory */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void AddTopicToMemory(const FGuid& MemoryId, const FString& Topic);

	/** Recall memories by topic */
	UFUNCTION(BlueprintPure, Category = "Memory")
	TArray<FRfsnMemoryEntry> RecallByTopic(const FString& Topic) const;

	/** Recall memories by type */
	UFUNCTION(BlueprintPure, Category = "Memory")
	TArray<FRfsnMemoryEntry> RecallByType(ERfsnMemoryType Type) const;

	/** Get most recent memories */
	UFUNCTION(BlueprintPure, Category = "Memory")
	TArray<FRfsnMemoryEntry> GetRecentMemories(int32 Count = 5) const;

	/** Get strongest memories */
	UFUNCTION(BlueprintPure, Category = "Memory")
	TArray<FRfsnMemoryEntry> GetStrongestMemories(int32 Count = 5) const;

	/** Find memory by ID */
	UFUNCTION(BlueprintPure, Category = "Memory")
	FRfsnMemoryEntry FindMemory(const FGuid& MemoryId) const;

	/** Reinforce a memory (makes it stronger) */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void ReinforceMemory(const FGuid& MemoryId);

	/** Start tracking a conversation */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void StartConversation();

	/** Record player statement in current conversation */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void RecordPlayerStatement(const FString& Statement);

	/** Record NPC response in current conversation */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void RecordNpcResponse(const FString& Response);

	/** End conversation and create memory from it */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	FGuid EndConversation();

	/** Get memory context for LLM prompt */
	UFUNCTION(BlueprintPure, Category = "Memory")
	FString GetMemoryContext(int32 MaxMemories = 3) const;

	/** Get conversation history for LLM prompt */
	UFUNCTION(BlueprintPure, Category = "Memory")
	FString GetConversationHistory() const;

	/** Check if NPC has met player before */
	UFUNCTION(BlueprintPure, Category = "Memory")
	bool HasMetPlayer() const;

	/** Decay all memories (call periodically) */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void DecayMemories(float GameHoursElapsed);

	/** Save memories to disk */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void SaveMemories();

	/** Load memories from disk */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	bool LoadMemories();

protected:
	virtual void BeginPlay() override;

private:
	/** Get save path for this NPC's memories */
	FString GetSavePath() const;

	/** Detect topics from text */
	TArray<FString> DetectTopics(const FString& Text) const;

	/** Trim old memories if over limit */
	void TrimMemories();
};
