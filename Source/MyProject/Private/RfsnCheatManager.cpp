// RFSN Console Commands Implementation

#include "RfsnCheatManager.h"
#include "RfsnDialogueManager.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnDebugHud.h"
#include "RfsnConversationLog.h"
#include "RfsnBlueprintLibrary.h"
#include "RfsnLogging.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void URfsnCheatManager::RfsnDebug()
{
	// Find debug HUD component on player
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn())
	{
		RFSN_WARNING("RfsnDebug: No player controller");
		return;
	}

	URfsnDebugHud* DebugHud = PC->GetPawn()->FindComponentByClass<URfsnDebugHud>();
	if (DebugHud)
	{
		DebugHud->ToggleDebug();
		RFSN_LOG("RFSN Debug HUD: %s", DebugHud->bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
	}
	else
	{
		RFSN_WARNING("RfsnDebug: No RfsnDebugHud component on player");
	}
}

void URfsnCheatManager::RfsnTalk()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	UWorld* World = GetWorld();
	URfsnDialogueManager* Manager = World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;
	if (!Manager)
	{
		RFSN_WARNING("RfsnTalk: No dialogue manager");
		return;
	}

	FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	AActor* NearestNpc = Manager->FindNearestRfsnNpc(PlayerLoc, 500.0f);

	if (NearestNpc)
	{
		Manager->StartDialogue(NearestNpc);
		Manager->SendPlayerMessage(TEXT("Hello!"));
		RFSN_LOG("Started dialogue with: %s", *NearestNpc->GetName());
	}
	else
	{
		RFSN_WARNING("RfsnTalk: No RFSN NPC in range");
	}
}

void URfsnCheatManager::RfsnSay(const FString& Message)
{
	UWorld* World = GetWorld();
	URfsnDialogueManager* Manager = World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;
	if (Manager && Manager->IsDialogueActive())
	{
		Manager->SendPlayerMessage(Message);
		RFSN_LOG("Sent: %s", *Message);
	}
	else
	{
		RFSN_WARNING("RfsnSay: No active dialogue");
	}
}

void URfsnCheatManager::RfsnEndDialogue()
{
	UWorld* World = GetWorld();
	URfsnDialogueManager* Manager = World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;
	if (Manager)
	{
		Manager->EndDialogue();
		RFSN_LOG("Dialogue ended");
	}
}

void URfsnCheatManager::RfsnPingServer()
{
	RFSN_LOG("Pinging RFSN server at %s...", *URfsnBlueprintLibrary::GetRfsnServerUrl());
	// TODO: Implement async ping
	RFSN_LOG("(Async ping not yet implemented)");
}

void URfsnCheatManager::RfsnListNpcs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		URfsnNpcClientComponent* Client = Actor ? Actor->FindComponentByClass<URfsnNpcClientComponent>() : nullptr;
		if (Client)
		{
			RFSN_LOG("[%d] %s - Name: %s, Mood: %s, Affinity: %.2f", Count++, *Actor->GetName(), *Client->NpcName,
			         *Client->Mood, Client->Affinity);
		}
	}
	RFSN_LOG("Found %d RFSN NPCs", Count);
}

void URfsnCheatManager::RfsnSetMood(const FString& NpcName, const FString& Mood)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		URfsnNpcClientComponent* Client = Actor ? Actor->FindComponentByClass<URfsnNpcClientComponent>() : nullptr;
		if (Client && Client->NpcName.Contains(NpcName))
		{
			Client->Mood = Mood;
			RFSN_LOG("Set %s mood to: %s", *Client->NpcName, *Mood);
			return;
		}
	}
	RFSN_WARNING("RfsnSetMood: NPC '%s' not found", *NpcName);
}

void URfsnCheatManager::RfsnSpawnNpc(const FString& NpcType)
{
	RFSN_LOG("RfsnSpawnNpc: Not yet implemented - spawn %s", *NpcType);
	// Would need to spawn Blueprint actor dynamically
}

void URfsnCheatManager::RfsnMockMode()
{
	bMockModeEnabled = !bMockModeEnabled;
	RFSN_LOG("RFSN Mock Mode: %s", bMockModeEnabled ? TEXT("Enabled") : TEXT("Disabled"));
	// TODO: Configure all RFSN clients to use mock responses
}

void URfsnCheatManager::RfsnDumpLog()
{
	// Find conversation log on player
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	URfsnConversationLog* Log = PC->GetPawn()->FindComponentByClass<URfsnConversationLog>();
	if (!Log)
	{
		RFSN_WARNING("RfsnDumpLog: No conversation log component");
		return;
	}

	const TArray<FRfsnConversationEntry>& Entries = Log->GetEntries();
	RFSN_LOG("=== Conversation Log (%d entries) ===", Entries.Num());
	for (const FRfsnConversationEntry& Entry : Entries)
	{
		RFSN_LOG("[%s] %s: %s", Entry.bIsPlayer ? TEXT("PLAYER") : TEXT("NPC"), *Entry.Speaker, *Entry.Message);
	}
}
