// RFSN Group Conversation Implementation

#include "RfsnGroupConversation.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

URfsnGroupConversation::URfsnGroupConversation()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}

void URfsnGroupConversation::BeginPlay()
{
	Super::BeginPlay();

	// Setup default topics if empty
	if (AvailableTopics.Num() == 0)
	{
		AvailableTopics.Add({TEXT("weather"), TEXT("Weather"), {TEXT("Have you noticed the weather lately?")}});
		AvailableTopics.Add({TEXT("rumors"), TEXT("Rumors"), {TEXT("Have you heard any interesting news?")}});
		AvailableTopics.Add({TEXT("survival"), TEXT("Survival"), {TEXT("How are supplies holding up?")}});
		AvailableTopics.Add({TEXT("stories"), TEXT("Stories"), {TEXT("Remember the old days?")}});
	}

	RFSN_LOG(TEXT("GroupConversation initialized"));
}

void URfsnGroupConversation::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bConversationActive)
	{
		return;
	}

	// Check end conditions
	if (ShouldEndConversation())
	{
		EndConversation();
		return;
	}

	// Auto-progress conversation
	TurnTimer += DeltaTime;
	if (TurnTimer >= TurnDelay && Participants.Num() > 0)
	{
		TurnTimer = 0.0f;
		TriggerNextSpeaker();
	}
}

bool URfsnGroupConversation::StartConversation(const TArray<FString>& NpcIds, const FString& Topic)
{
	if (bConversationActive || NpcIds.Num() < 2)
	{
		return false;
	}

	Participants.Empty();
	DialogueHistory.Empty();

	for (const FString& NpcId : NpcIds)
	{
		if (Participants.Num() >= MaxParticipants)
		{
			break;
		}

		URfsnNpcClientComponent* NpcComp = FindNpcComponent(NpcId);
		if (NpcComp)
		{
			FRfsnConversationParticipant Participant;
			Participant.NpcId = NpcId;
			Participant.DisplayName = NpcComp->NpcName;
			Participant.NpcComponent = NpcComp;
			Participants.Add(Participant);
		}
	}

	if (Participants.Num() < 2)
	{
		return false;
	}

	bConversationActive = true;
	CurrentSpeakerIndex = 0;
	TurnTimer = 0.0f;
	ConversationStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// Set topic
	if (!Topic.IsEmpty())
	{
		CurrentTopic = Topic;
	}
	else if (AvailableTopics.Num() > 0)
	{
		CurrentTopic = AvailableTopics[FMath::RandRange(0, AvailableTopics.Num() - 1)].TopicId;
	}

	RFSN_LOG(TEXT("Group conversation started with %d participants, topic: %s"), Participants.Num(), *CurrentTopic);

	// First speaker starts
	TriggerNextSpeaker();
	return true;
}

void URfsnGroupConversation::EndConversation()
{
	if (!bConversationActive)
	{
		return;
	}

	bConversationActive = false;
	bPlayerParticipating = false;

	OnConversationEnded.Broadcast();
	RFSN_LOG(TEXT("Group conversation ended after %d exchanges"), DialogueHistory.Num());
}

bool URfsnGroupConversation::AddParticipant(const FString& NpcId)
{
	if (!bConversationActive || IsParticipating(NpcId) || Participants.Num() >= MaxParticipants)
	{
		return false;
	}

	URfsnNpcClientComponent* NpcComp = FindNpcComponent(NpcId);
	if (!NpcComp)
	{
		return false;
	}

	FRfsnConversationParticipant Participant;
	Participant.NpcId = NpcId;
	Participant.DisplayName = NpcComp->NpcName;
	Participant.NpcComponent = NpcComp;
	Participants.Add(Participant);

	OnParticipantJoined.Broadcast(NpcId);
	RFSN_LOG(TEXT("%s joined group conversation"), *NpcId);
	return true;
}

bool URfsnGroupConversation::RemoveParticipant(const FString& NpcId)
{
	int32 Index =
	    Participants.IndexOfByPredicate([&NpcId](const FRfsnConversationParticipant& P) { return P.NpcId == NpcId; });

	if (Index == INDEX_NONE)
	{
		return false;
	}

	Participants.RemoveAt(Index);
	OnParticipantLeft.Broadcast(NpcId);

	// End if not enough participants
	if (Participants.Num() < 2 && !bPlayerParticipating)
	{
		EndConversation();
	}

	return true;
}

void URfsnGroupConversation::PlayerJoin()
{
	if (!bConversationActive)
	{
		return;
	}

	bPlayerParticipating = true;
	RFSN_LOG(TEXT("Player joined group conversation"));
}

void URfsnGroupConversation::PlayerLeave()
{
	bPlayerParticipating = false;
	RFSN_LOG(TEXT("Player left group conversation"));

	if (Participants.Num() < 2)
	{
		EndConversation();
	}
}

void URfsnGroupConversation::PlayerSpeak(const FString& Text)
{
	if (!bConversationActive || !bPlayerParticipating)
	{
		return;
	}

	AddDialogueLine(TEXT("player"), TEXT("You"), Text, true);
	TurnTimer = 0.0f; // Reset timer for NPC response
}

void URfsnGroupConversation::TriggerNextSpeaker()
{
	if (!bConversationActive || Participants.Num() == 0)
	{
		return;
	}

	int32 NextSpeaker = SelectNextSpeaker();
	if (NextSpeaker != INDEX_NONE)
	{
		CurrentSpeakerIndex = NextSpeaker;
		GenerateNpcResponse(NextSpeaker);
	}
}

FString URfsnGroupConversation::GetConversationContext() const
{
	FString Context = FString::Printf(TEXT("Group conversation between: %s. "), *GetParticipantNames());
	Context += FString::Printf(TEXT("Topic: %s. "), *CurrentTopic);

	if (bPlayerParticipating)
	{
		Context += TEXT("Player is part of the conversation. ");
	}
	else
	{
		Context += TEXT("NPCs are talking among themselves. ");
	}

	if (DialogueHistory.Num() > 0)
	{
		Context += FString::Printf(TEXT("%d exchanges so far. "), DialogueHistory.Num());
	}

	return Context;
}

FString URfsnGroupConversation::GetRecentDialogue(int32 LineCount) const
{
	FString Dialogue;
	int32 Start = FMath::Max(0, DialogueHistory.Num() - LineCount);

	for (int32 i = Start; i < DialogueHistory.Num(); ++i)
	{
		const FRfsnGroupDialogueLine& Line = DialogueHistory[i];
		Dialogue += FString::Printf(TEXT("%s: %s\n"), *Line.SpeakerName, *Line.Text);
	}

	return Dialogue;
}

FString URfsnGroupConversation::GetParticipantNames() const
{
	TArray<FString> Names;
	for (const FRfsnConversationParticipant& P : Participants)
	{
		Names.Add(P.DisplayName);
	}
	if (bPlayerParticipating)
	{
		Names.Add(TEXT("Player"));
	}
	return FString::Join(Names, TEXT(", "));
}

bool URfsnGroupConversation::IsParticipating(const FString& NpcId) const
{
	return Participants.ContainsByPredicate([&NpcId](const FRfsnConversationParticipant& P)
	                                        { return P.NpcId == NpcId; });
}

URfsnNpcClientComponent* URfsnGroupConversation::FindNpcComponent(const FString& NpcId) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		URfsnNpcClientComponent* Comp = (*It)->FindComponentByClass<URfsnNpcClientComponent>();
		if (Comp && Comp->NpcId == NpcId)
		{
			return Comp;
		}
	}

	return nullptr;
}

int32 URfsnGroupConversation::SelectNextSpeaker() const
{
	if (Participants.Num() == 0)
	{
		return INDEX_NONE;
	}

	// Find participant who hasn't spoken recently
	int32 Best = -1;
	int32 LowestTurns = INT_MAX;

	for (int32 i = 0; i < Participants.Num(); ++i)
	{
		if (i != CurrentSpeakerIndex && Participants[i].TurnCount < LowestTurns)
		{
			LowestTurns = Participants[i].TurnCount;
			Best = i;
		}
	}

	if (Best == -1 && Participants.Num() > 0)
	{
		// Just pick next
		Best = (CurrentSpeakerIndex + 1) % Participants.Num();
	}

	return Best;
}

void URfsnGroupConversation::GenerateNpcResponse(int32 SpeakerIndex)
{
	if (SpeakerIndex < 0 || SpeakerIndex >= Participants.Num())
	{
		return;
	}

	FRfsnConversationParticipant& Speaker = Participants[SpeakerIndex];
	Speaker.TurnCount++;
	Speaker.bHasContributed = true;
	Speaker.bIsSpeaking = true;

	// Generate response via NPC component (would integrate with LLM)
	FString Response;
	if (Speaker.NpcComponent)
	{
		// Create context-aware prompt
		FString Prompt = FString::Printf(
		    TEXT("You are %s in a group conversation about %s with %s. Say something brief and natural."),
		    *Speaker.DisplayName, *CurrentTopic, *GetParticipantNames());

		// For now, use placeholder
		TArray<FString> Responses = {TEXT("I agree with that."),
		                             TEXT("That's an interesting point."),
		                             TEXT("I hadn't thought about it that way."),
		                             TEXT("What do you think we should do about it?"),
		                             TEXT("I've been thinking the same thing."),
		                             TEXT("Things have been different lately."),
		                             TEXT("We should be careful."),
		                             TEXT("I hope things improve soon.")};
		Response = Responses[FMath::RandRange(0, Responses.Num() - 1)];
	}
	else
	{
		Response = TEXT("...");
	}

	AddDialogueLine(Speaker.NpcId, Speaker.DisplayName, Response);
	Speaker.bIsSpeaking = false;
}

void URfsnGroupConversation::AddDialogueLine(const FString& SpeakerId, const FString& Name, const FString& Text,
                                             bool bIsPlayer)
{
	FRfsnGroupDialogueLine Line;
	Line.SpeakerId = SpeakerId;
	Line.SpeakerName = Name;
	Line.Text = Text;
	Line.bIsPlayer = bIsPlayer;
	Line.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	DialogueHistory.Add(Line);
	OnGroupDialogue.Broadcast(SpeakerId, Text);

	RFSN_LOG(TEXT("[Group] %s: %s"), *Name, *Text);
}

bool URfsnGroupConversation::ShouldEndConversation() const
{
	// Time limit
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - ConversationStartTime > MaxDuration)
	{
		return true;
	}

	// Not enough participants
	if (Participants.Num() < 2 && !bPlayerParticipating)
	{
		return true;
	}

	// Too many exchanges
	if (DialogueHistory.Num() > 20)
	{
		return true;
	}

	return false;
}
