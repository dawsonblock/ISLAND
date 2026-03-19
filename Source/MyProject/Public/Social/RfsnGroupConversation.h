// RFSN Group Conversation System
// Manages multi-NPC dialogue and NPC-to-NPC conversations
// Player can join or observe NPC conversations

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnGroupConversation.generated.h"

class URfsnNpcClientComponent;

/**
 * Participant in a group conversation
 */
USTRUCT(BlueprintType)
struct FRfsnConversationParticipant
{
	GENERATED_BODY()

	/** NPC ID */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString NpcId;

	/** Display name */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString DisplayName;

	/** Reference to NPC component */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	URfsnNpcClientComponent* NpcComponent = nullptr;

	/** Is this participant actively speaking? */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	bool bIsSpeaking = false;

	/** Has this participant contributed recently? */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	bool bHasContributed = false;

	/** Turn count this conversation */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	int32 TurnCount = 0;
};

/**
 * Dialogue line in group conversation
 */
USTRUCT(BlueprintType)
struct FRfsnGroupDialogueLine
{
	GENERATED_BODY()

	/** Speaker NPC ID */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString SpeakerId;

	/** Speaker display name */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString SpeakerName;

	/** The dialogue text */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString Text;

	/** Timestamp */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	float Timestamp = 0.0f;

	/** Is this from the player? */
	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	bool bIsPlayer = false;
};

/**
 * Conversation topic
 */
USTRUCT(BlueprintType)
struct FRfsnConversationTopic
{
	GENERATED_BODY()

	/** Topic identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Topic")
	FString TopicId;

	/** Topic display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Topic")
	FString DisplayName;

	/** Starter prompts for this topic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Topic")
	TArray<FString> StarterPrompts;

	/** How many exchanges on this topic */
	UPROPERTY(BlueprintReadWrite, Category = "Topic")
	int32 ExchangeCount = 0;

	/** Is topic exhausted? */
	bool IsExhausted() const { return ExchangeCount >= 3; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGroupDialogue, const FString&, SpeakerId, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantJoined, const FString&, NpcId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantLeft, const FString&, NpcId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);

/**
 * Group Conversation Component
 * Manages multi-NPC dialogue sessions
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnGroupConversation : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnGroupConversation();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum participants (excluding player) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation|Config")
	int32 MaxParticipants = 4;

	/** Time between NPC turns (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation|Config")
	float TurnDelay = 3.0f;

	/** Maximum conversation duration (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation|Config")
	float MaxDuration = 120.0f;

	/** Proximity radius for auto-joining */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation|Config")
	float JoinRadius = 400.0f;

	/** Available conversation topics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation|Config")
	TArray<FRfsnConversationTopic> AvailableTopics;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current participants */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	TArray<FRfsnConversationParticipant> Participants;

	/** Dialogue history */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	TArray<FRfsnGroupDialogueLine> DialogueHistory;

	/** Current topic */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	FString CurrentTopic;

	/** Is conversation active? */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	bool bConversationActive = false;

	/** Is player participating? */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	bool bPlayerParticipating = false;

	/** Current speaker index */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	int32 CurrentSpeakerIndex = 0;

	/** Conversation start time */
	UPROPERTY(BlueprintReadOnly, Category = "Conversation|State")
	float ConversationStartTime = 0.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Conversation|Events")
	FOnGroupDialogue OnGroupDialogue;

	UPROPERTY(BlueprintAssignable, Category = "Conversation|Events")
	FOnParticipantJoined OnParticipantJoined;

	UPROPERTY(BlueprintAssignable, Category = "Conversation|Events")
	FOnParticipantLeft OnParticipantLeft;

	UPROPERTY(BlueprintAssignable, Category = "Conversation|Events")
	FOnConversationEnded OnConversationEnded;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start a group conversation with specified NPCs */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	bool StartConversation(const TArray<FString>& NpcIds, const FString& Topic = TEXT(""));

	/** End the current conversation */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void EndConversation();

	/** Add participant to active conversation */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	bool AddParticipant(const FString& NpcId);

	/** Remove participant from conversation */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	bool RemoveParticipant(const FString& NpcId);

	/** Player joins the conversation */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void PlayerJoin();

	/** Player leaves the conversation */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void PlayerLeave();

	/** Player says something to the group */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void PlayerSpeak(const FString& Text);

	/** Trigger next NPC to speak */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void TriggerNextSpeaker();

	/** Get conversation context for LLM */
	UFUNCTION(BlueprintPure, Category = "Conversation")
	FString GetConversationContext() const;

	/** Get recent dialogue for LLM */
	UFUNCTION(BlueprintPure, Category = "Conversation")
	FString GetRecentDialogue(int32 LineCount = 5) const;

	/** Get participant names as string */
	UFUNCTION(BlueprintPure, Category = "Conversation")
	FString GetParticipantNames() const;

	/** Check if NPC is participating */
	UFUNCTION(BlueprintPure, Category = "Conversation")
	bool IsParticipating(const FString& NpcId) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Turn timer */
	float TurnTimer = 0.0f;

	/** Find NPC component by ID */
	URfsnNpcClientComponent* FindNpcComponent(const FString& NpcId) const;

	/** Select next speaker */
	int32 SelectNextSpeaker() const;

	/** Generate NPC response */
	void GenerateNpcResponse(int32 SpeakerIndex);

	/** Add line to history */
	void AddDialogueLine(const FString& SpeakerId, const FString& Name, const FString& Text, bool bIsPlayer = false);

	/** Check for conversation end conditions */
	bool ShouldEndConversation() const;
};
