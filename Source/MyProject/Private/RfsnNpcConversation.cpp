// RFSN NPC Conversation Implementation

#include "RfsnNpcConversation.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/World.h"

void URfsnNpcConversation::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Start conversation tick timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ConversationTickHandle, this, &URfsnNpcConversation::TickConversations,
		                                  1.0f, // Check every second
		                                  true);
	}

	RFSN_LOG(TEXT("NPC Conversation system initialized"));
}

void URfsnNpcConversation::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConversationTickHandle);
	}

	// End all active conversations
	TArray<FString> ConvIds;
	ActiveConversations.GetKeys(ConvIds);
	for (const FString& Id : ConvIds)
	{
		EndConversation(Id);
	}

	Super::Deinitialize();
}

FString URfsnNpcConversation::StartDialogue(AActor* NpcA, AActor* NpcB, const FString& Topic)
{
	if (!NpcA || !NpcB)
	{
		RFSN_ERROR(TEXT("Cannot start dialogue - invalid NPCs"));
		return FString();
	}

	if (ActiveConversations.Num() >= MaxConcurrentConversations)
	{
		RFSN_WARNING(TEXT("Max concurrent conversations reached"));
		return FString();
	}

	// Check if either NPC is already in a conversation
	if (IsNpcInConversation(NpcA) || IsNpcInConversation(NpcB))
	{
		RFSN_WARNING(TEXT("One or both NPCs already in conversation"));
		return FString();
	}

	FRfsnNpcConversationSession Session;
	Session.ConversationId = GenerateConversationId();
	Session.Type = ERfsnConversationType::Dialogue;
	Session.Topic = Topic;
	Session.bActive = true;
	Session.MaxTurns = 6;

	// Add participants
	FRfsnNpcConversationParticipant ParticipantA;
	ParticipantA.NpcActor = NpcA;
	if (URfsnNpcClientComponent* ClientA = NpcA->FindComponentByClass<URfsnNpcClientComponent>())
	{
		ParticipantA.NpcName = ClientA->NpcName;
	}
	ParticipantA.Role = TEXT("speaker");
	Session.Participants.Add(ParticipantA);

	FRfsnNpcConversationParticipant ParticipantB;
	ParticipantB.NpcActor = NpcB;
	if (URfsnNpcClientComponent* ClientB = NpcB->FindComponentByClass<URfsnNpcClientComponent>())
	{
		ParticipantB.NpcName = ClientB->NpcName;
	}
	ParticipantB.Role = TEXT("listener");
	Session.Participants.Add(ParticipantB);

	ActiveConversations.Add(Session.ConversationId, Session);

	OnConversationStarted.Broadcast(Session.ConversationId, Session.Participants);
	RFSN_DIALOGUE_LOG(TEXT("Started NPC dialogue: %s and %s about '%s'"), *ParticipantA.NpcName, *ParticipantB.NpcName,
	                  *Topic);

	// Start first turn
	AdvanceConversation(ActiveConversations[Session.ConversationId]);

	return Session.ConversationId;
}

FString URfsnNpcConversation::StartGroupDiscussion(const TArray<AActor*>& Npcs, const FString& Topic)
{
	if (Npcs.Num() < 2)
	{
		RFSN_ERROR(TEXT("Need at least 2 NPCs for group discussion"));
		return FString();
	}

	FRfsnNpcConversationSession Session;
	Session.ConversationId = GenerateConversationId();
	Session.Type = ERfsnConversationType::GroupDiscussion;
	Session.Topic = Topic;
	Session.bActive = true;
	Session.MaxTurns = Npcs.Num() * 2; // Each gets 2 turns

	for (AActor* Npc : Npcs)
	{
		if (!Npc || IsNpcInConversation(Npc))
		{
			continue;
		}

		FRfsnNpcConversationParticipant Participant;
		Participant.NpcActor = Npc;
		if (URfsnNpcClientComponent* Client = Npc->FindComponentByClass<URfsnNpcClientComponent>())
		{
			Participant.NpcName = Client->NpcName;
		}
		Participant.Role = TEXT("participant");
		Session.Participants.Add(Participant);
	}

	if (Session.Participants.Num() < 2)
	{
		RFSN_ERROR(TEXT("Not enough available NPCs for discussion"));
		return FString();
	}

	ActiveConversations.Add(Session.ConversationId, Session);
	OnConversationStarted.Broadcast(Session.ConversationId, Session.Participants);

	AdvanceConversation(ActiveConversations[Session.ConversationId]);

	return Session.ConversationId;
}

void URfsnNpcConversation::Announce(AActor* Speaker, const FString& Message, float Radius)
{
	if (!Speaker)
	{
		return;
	}

	FString SpeakerName = TEXT("Unknown");
	if (URfsnNpcClientComponent* Client = Speaker->FindComponentByClass<URfsnNpcClientComponent>())
	{
		SpeakerName = Client->NpcName;
	}

	// Find nearby NPCs
	TArray<AActor*> NearbyNpcs = FindNearbyNpcs(Speaker, Radius);

	RFSN_DIALOGUE_LOG(TEXT("[%s] (Announcing) %s"), *SpeakerName, *Message);
	OnNpcSpoke.Broadcast(SpeakerName, Message, TEXT("announcement"));

	// Each nearby NPC could react
	for (AActor* Npc : NearbyNpcs)
	{
		if (Npc == Speaker)
		{
			continue;
		}

		// Optionally trigger reaction in nearby NPCs
		if (URfsnNpcClientComponent* Client = Npc->FindComponentByClass<URfsnNpcClientComponent>())
		{
			// Could send context to RFSN for reaction
			// Client->SendPlayerUtterance(FString::Printf(TEXT("[Heard announcement from %s]: %s"), *SpeakerName,
			// *Message));
		}
	}
}

bool URfsnNpcConversation::PlayerJoinConversation(const FString& ConversationId)
{
	if (!ActiveConversations.Contains(ConversationId))
	{
		return false;
	}

	FRfsnNpcConversationSession& Session = ActiveConversations[ConversationId];
	if (!Session.bPlayerCanJoin)
	{
		return false;
	}

	// End NPC-to-NPC conversation and switch to player dialogue
	EndConversation(ConversationId);

	// Player can now talk to the NPCs through normal dialogue system
	RFSN_LOG(TEXT("Player joined conversation %s"), *ConversationId);
	return true;
}

void URfsnNpcConversation::EndConversation(const FString& ConversationId)
{
	if (!ActiveConversations.Contains(ConversationId))
	{
		return;
	}

	FRfsnNpcConversationSession& Session = ActiveConversations[ConversationId];
	Session.bActive = false;

	OnConversationEnded.Broadcast(ConversationId);
	RFSN_LOG(TEXT("Ended conversation: %s after %d turns"), *ConversationId, Session.TotalTurns);

	ActiveConversations.Remove(ConversationId);
}

bool URfsnNpcConversation::GetNpcConversation(AActor* Npc, FRfsnNpcConversationSession& OutSession)
{
	for (auto& Pair : ActiveConversations)
	{
		for (const FRfsnNpcConversationParticipant& P : Pair.Value.Participants)
		{
			if (P.NpcActor.Get() == Npc)
			{
				OutSession = Pair.Value;
				return true;
			}
		}
	}
	return false;
}

bool URfsnNpcConversation::IsNpcInConversation(AActor* Npc) const
{
	for (const auto& Pair : ActiveConversations)
	{
		for (const FRfsnNpcConversationParticipant& P : Pair.Value.Participants)
		{
			if (P.NpcActor.Get() == Npc)
			{
				return true;
			}
		}
	}
	return false;
}

TArray<FRfsnNpcConversationSession> URfsnNpcConversation::GetActiveConversations() const
{
	TArray<FRfsnNpcConversationSession> Result;
	ActiveConversations.GenerateValueArray(Result);
	return Result;
}

TArray<AActor*> URfsnNpcConversation::FindNearbyNpcs(AActor* Origin, float Radius) const
{
	TArray<AActor*> Result;

	if (!Origin)
	{
		return Result;
	}

	FVector OriginLocation = Origin->GetActorLocation();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor != Origin && Actor->FindComponentByClass<URfsnNpcClientComponent>())
		{
			float Distance = FVector::Dist(OriginLocation, Actor->GetActorLocation());
			if (Distance <= Radius)
			{
				Result.Add(Actor);
			}
		}
	}

	return Result;
}

FString URfsnNpcConversation::GenerateConversationId()
{
	return FString::Printf(TEXT("conv_%d_%d"), FDateTime::Now().GetTicks(), FMath::RandRange(1000, 9999));
}

void URfsnNpcConversation::TickConversations()
{
	// This would be used for timed auto-advancement
	// Currently conversations advance via RFSN response callbacks
}

void URfsnNpcConversation::AdvanceConversation(FRfsnNpcConversationSession& Session)
{
	if (!Session.bActive || Session.TotalTurns >= Session.MaxTurns)
	{
		EndConversation(Session.ConversationId);
		return;
	}

	if (Session.Participants.Num() == 0)
	{
		return;
	}

	// Get current speaker
	FRfsnNpcConversationParticipant& CurrentSpeaker = Session.Participants[Session.CurrentSpeakerIndex];
	AActor* SpeakerActor = CurrentSpeaker.NpcActor.Get();

	if (!SpeakerActor)
	{
		EndConversation(Session.ConversationId);
		return;
	}

	// Build context for RFSN
	FString Context;
	if (Session.TotalTurns == 0)
	{
		// Opening line
		Context = FString::Printf(TEXT("You are starting a conversation about '%s'. Say something to begin."),
		                          *Session.Topic);
	}
	else
	{
		// Get the other participant(s)
		TArray<FString> OtherNames;
		for (int32 i = 0; i < Session.Participants.Num(); ++i)
		{
			if (i != Session.CurrentSpeakerIndex)
			{
				OtherNames.Add(Session.Participants[i].NpcName);
			}
		}
		FString Others = FString::Join(OtherNames, TEXT(" and "));

		Context = FString::Printf(TEXT("You are talking to %s about '%s'. Continue the conversation."), *Others,
		                          *Session.Topic);
	}

	SendNpcMessage(SpeakerActor, Context, Session.ConversationId);

	// Advance turn
	Session.TotalTurns++;
	CurrentSpeaker.TurnsTaken++;
	Session.CurrentSpeakerIndex = (Session.CurrentSpeakerIndex + 1) % Session.Participants.Num();
}

void URfsnNpcConversation::SendNpcMessage(AActor* Npc, const FString& Context, const FString& ConversationId)
{
	if (!Npc)
	{
		return;
	}

	URfsnNpcClientComponent* Client = Npc->FindComponentByClass<URfsnNpcClientComponent>();
	if (!Client)
	{
		return;
	}

	// Bind to receive response
	// Note: In production, would use a more robust callback system
	Client->OnSentenceReceived.AddDynamic(this, &URfsnNpcConversation::OnNpcResponseReceived);

	Client->SendPlayerUtterance(Context);
}

void URfsnNpcConversation::OnNpcResponseReceived(const FRfsnSentence& Sentence)
{
	// Find which conversation this belongs to and broadcast
	// In a full implementation, we'd track which NPC this came from
	if (!Sentence.Sentence.IsEmpty())
	{
		OnNpcSpoke.Broadcast(TEXT("NPC"), Sentence.Sentence, TEXT("active"));
	}
}
