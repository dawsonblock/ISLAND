// RFSN Blueprint Function Library Implementation

#include "RfsnBlueprintLibrary.h"
#include "RfsnDialogueManager.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "EngineUtils.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────
// Dialogue Management
// ─────────────────────────────────────────────────────────────

bool URfsnBlueprintLibrary::StartDialogueWithNpc(const UObject* WorldContextObject, AActor* NpcActor)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	if (Manager && NpcActor)
	{
		return Manager->StartDialogue(NpcActor);
	}
	return false;
}

void URfsnBlueprintLibrary::EndDialogue(const UObject* WorldContextObject)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	if (Manager)
	{
		Manager->EndDialogue();
	}
}

void URfsnBlueprintLibrary::SendPlayerMessage(const UObject* WorldContextObject, const FString& Message)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	if (Manager)
	{
		Manager->SendPlayerMessage(Message);
	}
}

bool URfsnBlueprintLibrary::IsDialogueActive(const UObject* WorldContextObject)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	return Manager ? Manager->IsDialogueActive() : false;
}

AActor* URfsnBlueprintLibrary::GetActiveDialogueNpc(const UObject* WorldContextObject)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	return Manager ? Manager->GetActiveNpc() : nullptr;
}

// ─────────────────────────────────────────────────────────────
// NPC Discovery
// ─────────────────────────────────────────────────────────────

AActor* URfsnBlueprintLibrary::FindNearestRfsnNpc(const UObject* WorldContextObject, FVector Location,
                                                  float MaxDistance)
{
	URfsnDialogueManager* Manager = GetDialogueManager(WorldContextObject);
	return Manager ? Manager->FindNearestRfsnNpc(Location, MaxDistance) : nullptr;
}

TArray<AActor*> URfsnBlueprintLibrary::GetAllRfsnNpcs(const UObject* WorldContextObject)
{
	TArray<AActor*> RfsnNpcs;

	if (!WorldContextObject)
	{
		return RfsnNpcs;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return RfsnNpcs;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->FindComponentByClass<URfsnNpcClientComponent>())
		{
			RfsnNpcs.Add(Actor);
		}
	}

	return RfsnNpcs;
}

URfsnNpcClientComponent* URfsnBlueprintLibrary::GetRfsnClient(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<URfsnNpcClientComponent>() : nullptr;
}

// ─────────────────────────────────────────────────────────────
// NPC Configuration
// ─────────────────────────────────────────────────────────────

void URfsnBlueprintLibrary::SetNpcMood(AActor* NpcActor, const FString& Mood)
{
	URfsnNpcClientComponent* Client = GetRfsnClient(NpcActor);
	if (Client)
	{
		Client->Mood = Mood;
	}
}

void URfsnBlueprintLibrary::SetNpcRelationship(AActor* NpcActor, const FString& Relationship)
{
	URfsnNpcClientComponent* Client = GetRfsnClient(NpcActor);
	if (Client)
	{
		Client->Relationship = Relationship;
	}
}

void URfsnBlueprintLibrary::SetNpcAffinity(AActor* NpcActor, float Affinity)
{
	URfsnNpcClientComponent* Client = GetRfsnClient(NpcActor);
	if (Client)
	{
		Client->Affinity = FMath::Clamp(Affinity, -1.0f, 1.0f);
	}
}

// ─────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────

void URfsnBlueprintLibrary::CheckServerHealth(const UObject* WorldContextObject)
{
	// TODO: Implement async health check
	RFSN_LOG(TEXT("Server health check requested"));
}

FString URfsnBlueprintLibrary::GetRfsnServerUrl()
{
	// Could read from config
	return TEXT("http://localhost:8000");
}

FString URfsnBlueprintLibrary::ActionToString(ERfsnNpcAction Action)
{
	switch (Action)
	{
	case ERfsnNpcAction::Greet:
		return TEXT("Greet");
	case ERfsnNpcAction::Attack:
		return TEXT("Attack");
	case ERfsnNpcAction::Flee:
		return TEXT("Flee");
	case ERfsnNpcAction::Help:
		return TEXT("Help");
	case ERfsnNpcAction::Trade:
		return TEXT("Trade");
	case ERfsnNpcAction::Warn:
		return TEXT("Warn");
	case ERfsnNpcAction::Threaten:
		return TEXT("Threaten");
	case ERfsnNpcAction::Accept:
		return TEXT("Accept");
	case ERfsnNpcAction::Refuse:
		return TEXT("Refuse");
	case ERfsnNpcAction::Explain:
		return TEXT("Explain");
	case ERfsnNpcAction::Answer:
		return TEXT("Answer");
	case ERfsnNpcAction::Offer:
		return TEXT("Offer");
	default:
		return TEXT("Unknown");
	}
}

URfsnDialogueManager* URfsnBlueprintLibrary::GetDialogueManager(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	return World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;
}
