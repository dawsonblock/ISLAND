// RFSN Replicated Dialogue Implementation

#include "RfsnReplicatedDialogue.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

URfsnReplicatedDialogue::URfsnReplicatedDialogue()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URfsnReplicatedDialogue::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URfsnReplicatedDialogue, CurrentSentence);
	DOREPLIFETIME(URfsnReplicatedDialogue, CurrentAction);
	DOREPLIFETIME(URfsnReplicatedDialogue, bDialogueActive);
}

void URfsnReplicatedDialogue::BeginPlay()
{
	Super::BeginPlay();

	// Cache RFSN client
	CachedClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>();

	// Bind to local RFSN events (server only)
	if (GetOwner()->HasAuthority() && CachedClient.IsValid())
	{
		CachedClient->OnSentenceReceived.AddDynamic(this, &URfsnReplicatedDialogue::OnLocalSentenceReceived);
		CachedClient->OnNpcActionReceived.AddDynamic(this, &URfsnReplicatedDialogue::OnLocalActionReceived);
	}
}

void URfsnReplicatedDialogue::ServerStartDialogue(const FString& PlayerUtterance)
{
	if (!GetOwner()->HasAuthority())
	{
		// Client requesting - send to server
		ServerRequestDialogue(PlayerUtterance);
		return;
	}

	// Server - initiate RFSN dialogue
	if (CachedClient.IsValid())
	{
		bDialogueActive = true;
		CachedClient->SendPlayerUtterance(PlayerUtterance);
	}
}

void URfsnReplicatedDialogue::OnRep_CurrentSentence()
{
	// Called on clients when sentence changes
	if (!CurrentSentence.IsEmpty())
	{
		RFSN_DIALOGUE_LOG(TEXT("[Replicated] %s"), *CurrentSentence);
		// Trigger local HUD display etc.
	}
}

void URfsnReplicatedDialogue::MulticastShowSentence_Implementation(const FString& Sentence)
{
	CurrentSentence = Sentence;
	RFSN_DIALOGUE_LOG(TEXT("[Multicast] %s"), *Sentence);
}

void URfsnReplicatedDialogue::MulticastNpcAction_Implementation(ERfsnNpcAction Action)
{
	CurrentAction = Action;
}

void URfsnReplicatedDialogue::ServerRequestDialogue_Implementation(const FString& PlayerUtterance)
{
	// Server receives request from client
	ServerStartDialogue(PlayerUtterance);
}

void URfsnReplicatedDialogue::OnLocalSentenceReceived(const FRfsnSentence& Sentence)
{
	// Server received sentence from RFSN - replicate to clients
	if (bReplicateDialogue)
	{
		MulticastShowSentence(Sentence.Sentence);
	}
}

void URfsnReplicatedDialogue::OnLocalActionReceived(ERfsnNpcAction Action)
{
	// Server received action from RFSN - replicate to clients
	if (bReplicateDialogue)
	{
		MulticastNpcAction(Action);
	}
}
