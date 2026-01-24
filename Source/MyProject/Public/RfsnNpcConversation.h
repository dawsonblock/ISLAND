// RFSN NPC Conversation System
// Manages multi-NPC conversations and inter-NPC communication

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnNpcConversation.generated.h"

UENUM(BlueprintType)
enum class ERfsnConversationType : uint8
{
	/** Two NPCs talking */
	Dialogue,
	/** Multiple NPCs in group discussion */
	GroupDiscussion,
	/** NPC announcing to others */
	Announcement,
	/** NPCs gossiping about player */
	Gossip
};

USTRUCT(BlueprintType)
struct FRfsnNpcConversationParticipant
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	TWeakObjectPtr<AActor> NpcActor;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString NpcName;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString Role; // "speaker", "listener", "moderator"

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	int32 TurnsTaken = 0;
};

USTRUCT(BlueprintType)
struct FRfsnNpcConversationSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString ConversationId;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	ERfsnConversationType Type = ERfsnConversationType::Dialogue;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	TArray<FRfsnNpcConversationParticipant> Participants;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	FString Topic;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	int32 CurrentSpeakerIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	int32 TotalTurns = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	int32 MaxTurns = 6;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	bool bPlayerCanJoin = true;

	UPROPERTY(BlueprintReadWrite, Category = "Conversation")
	bool bActive = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNpcConversationStarted, const FString&, ConversationId,
                                             const TArray<FRfsnNpcConversationParticipant>&, Participants);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNpcSpokeInConversation, const FString&, NpcName, const FString&,
                                               Sentence, const FString&, ConversationId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNpcConversationEnded, const FString&, ConversationId);

/**
 * World Subsystem for managing NPC-to-NPC conversations.
 */
UCLASS()
class MYPROJECT_API URfsnNpcConversation : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "NPC Conversation|Events")
	FOnNpcConversationStarted OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category = "NPC Conversation|Events")
	FOnNpcSpokeInConversation OnNpcSpoke;

	UPROPERTY(BlueprintAssignable, Category = "NPC Conversation|Events")
	FOnNpcConversationEnded OnConversationEnded;

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Delay between NPC turns (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Conversation")
	float TurnDelay = 2.0f;

	/** Maximum concurrent conversations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Conversation")
	int32 MaxConcurrentConversations = 3;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start a conversation between two NPCs */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	FString StartDialogue(AActor* NpcA, AActor* NpcB, const FString& Topic);

	/** Start a group discussion */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	FString StartGroupDiscussion(const TArray<AActor*>& Npcs, const FString& Topic);

	/** Make an NPC announce to nearby NPCs */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	void Announce(AActor* Speaker, const FString& Message, float Radius = 1000.0f);

	/** Player joins an ongoing conversation */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	bool PlayerJoinConversation(const FString& ConversationId);

	/** End a conversation */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	void EndConversation(const FString& ConversationId);

	/** Get active conversation for NPC */
	UFUNCTION(BlueprintPure, Category = "NPC Conversation")
	bool GetNpcConversation(AActor* Npc, FRfsnNpcConversationSession& OutSession);

	/** Is NPC in a conversation */
	UFUNCTION(BlueprintPure, Category = "NPC Conversation")
	bool IsNpcInConversation(AActor* Npc) const;

	/** Get all active conversations */
	UFUNCTION(BlueprintPure, Category = "NPC Conversation")
	TArray<FRfsnNpcConversationSession> GetActiveConversations() const;

	/** Find nearby NPCs that can converse */
	UFUNCTION(BlueprintCallable, Category = "NPC Conversation")
	TArray<AActor*> FindNearbyNpcs(AActor* Origin, float Radius) const;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TMap<FString, FRfsnNpcConversationSession> ActiveConversations;

	FTimerHandle ConversationTickHandle;

	FString GenerateConversationId();
	void TickConversations();
	void AdvanceConversation(FRfsnNpcConversationSession& Session);
	void SendNpcMessage(AActor* Npc, const FString& Context, const FString& ConversationId);

	UFUNCTION()
	void OnNpcResponseReceived(const FRfsnSentence& Sentence);
};
