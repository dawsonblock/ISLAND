// RFSN Relationship Manager Implementation

#include "RfsnRelationshipManager.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnRelationshipSaveData.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"

void URfsnRelationshipManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create save data object
	SaveData = NewObject<URfsnRelationshipSaveData>(this);

	// Try to load existing save
	if (DoesSaveExist())
	{
		LoadRelationships();
		RFSN_LOG("Loaded %d NPC relationships from save", SaveData->NpcRelationships.Num());
	}
	else
	{
		RFSN_LOG("No saved relationships found, starting fresh");
	}
}

void URfsnRelationshipManager::Deinitialize()
{
	// Auto-save on shutdown
	if (SaveData && SaveData->NpcRelationships.Num() > 0)
	{
		SaveRelationships();
		RFSN_LOG("Auto-saved %d relationships on shutdown", SaveData->NpcRelationships.Num());
	}

	Super::Deinitialize();
}

FRfsnNpcRelationship URfsnRelationshipManager::GetRelationship(const FString& NpcId)
{
	if (!SaveData)
	{
		return FRfsnNpcRelationship();
	}
	return SaveData->GetOrCreateRelationship(NpcId);
}

void URfsnRelationshipManager::UpdateRelationship(const FString& NpcId, float Affinity, const FString& Relationship)
{
	if (!SaveData)
	{
		return;
	}

	SaveData->UpdateFromClient(NpcId, Affinity, Relationship);

	// Broadcast change
	FRfsnNpcRelationship& Rel = SaveData->GetOrCreateRelationship(NpcId);
	OnRelationshipChanged.Broadcast(NpcId, Rel);

	RFSN_DIALOGUE_LOG("Relationship updated: %s - Affinity: %.2f, Type: %s", *NpcId, Affinity, *Relationship);

	// Auto-save if enabled
	if (bAutoSave)
	{
		SaveRelationships();
	}
}

void URfsnRelationshipManager::RegisterNpcClient(URfsnNpcClientComponent* Client)
{
	if (!Client)
	{
		return;
	}

	// Add to tracked clients
	RegisteredClients.Add(Client);

	// Sync client state from saved data
	SyncClientFromSaveData(Client);

	RFSN_LOG("Registered NPC client: %s", *Client->NpcId);
}

void URfsnRelationshipManager::UnregisterNpcClient(URfsnNpcClientComponent* Client)
{
	if (!Client)
	{
		return;
	}

	// Save current state before unregistering
	SyncSaveDataFromClient(Client);

	// Remove from tracked
	RegisteredClients.RemoveAll([Client](const TWeakObjectPtr<URfsnNpcClientComponent>& Ptr)
	                            { return Ptr.Get() == Client; });

	RFSN_LOG("Unregistered NPC client: %s", *Client->NpcId);
}

void URfsnRelationshipManager::ModifyAffinity(const FString& NpcId, float Delta)
{
	if (!SaveData)
	{
		return;
	}

	FRfsnNpcRelationship& Rel = SaveData->GetOrCreateRelationship(NpcId);
	Rel.Affinity = FMath::Clamp(Rel.Affinity + Delta, -1.0f, 1.0f);
	Rel.LastInteraction = FDateTime::Now();
	Rel.InteractionCount++;

	// Update any registered clients
	for (auto& WeakClient : RegisteredClients)
	{
		if (URfsnNpcClientComponent* Client = WeakClient.Get())
		{
			if (Client->NpcId == NpcId)
			{
				Client->Affinity = Rel.Affinity;
			}
		}
	}

	OnRelationshipChanged.Broadcast(NpcId, Rel);

	if (bAutoSave)
	{
		SaveRelationships();
	}
}

void URfsnRelationshipManager::SetRelationshipType(const FString& NpcId, const FString& RelationshipType)
{
	if (!SaveData)
	{
		return;
	}

	FRfsnNpcRelationship& Rel = SaveData->GetOrCreateRelationship(NpcId);
	Rel.Relationship = RelationshipType;
	Rel.LastInteraction = FDateTime::Now();

	// Update any registered clients
	for (auto& WeakClient : RegisteredClients)
	{
		if (URfsnNpcClientComponent* Client = WeakClient.Get())
		{
			if (Client->NpcId == NpcId)
			{
				Client->Relationship = RelationshipType;
			}
		}
	}

	OnRelationshipChanged.Broadcast(NpcId, Rel);

	if (bAutoSave)
	{
		SaveRelationships();
	}
}

bool URfsnRelationshipManager::SaveRelationships()
{
	if (!SaveData)
	{
		RFSN_ERROR("Cannot save - no save data object");
		return false;
	}

	// Update timestamp
	SaveData->SaveTimestamp = FDateTime::Now();

	// Sync all registered clients to save data
	for (auto& WeakClient : RegisteredClients)
	{
		if (URfsnNpcClientComponent* Client = WeakClient.Get())
		{
			SyncSaveDataFromClient(Client);
		}
	}

	// Save to disk
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, SaveUserIndex);

	if (bSuccess)
	{
		RFSN_LOG("Saved %d relationships to slot '%s'", SaveData->NpcRelationships.Num(), *SaveSlotName);
	}
	else
	{
		RFSN_ERROR("Failed to save relationships");
	}

	return bSuccess;
}

bool URfsnRelationshipManager::LoadRelationships()
{
	if (!DoesSaveExist())
	{
		RFSN_WARNING("No save file found at slot '%s'", *SaveSlotName);
		return false;
	}

	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	URfsnRelationshipSaveData* LoadedData = Cast<URfsnRelationshipSaveData>(LoadedGame);

	if (!LoadedData)
	{
		RFSN_ERROR("Failed to load or cast save data");
		return false;
	}

	// Copy loaded data
	SaveData->NpcRelationships = LoadedData->NpcRelationships;
	SaveData->PlayerName = LoadedData->PlayerName;
	SaveData->SaveTimestamp = LoadedData->SaveTimestamp;

	// Sync all registered clients
	for (auto& WeakClient : RegisteredClients)
	{
		if (URfsnNpcClientComponent* Client = WeakClient.Get())
		{
			SyncClientFromSaveData(Client);
		}
	}

	RFSN_LOG("Loaded %d relationships from slot '%s'", SaveData->NpcRelationships.Num(), *SaveSlotName);

	return true;
}

bool URfsnRelationshipManager::DoesSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

void URfsnRelationshipManager::ClearSavedRelationships()
{
	if (DoesSaveExist())
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
		RFSN_LOG("Deleted save slot '%s'", *SaveSlotName);
	}

	if (SaveData)
	{
		SaveData->NpcRelationships.Empty();
	}

	// Reset all registered clients
	for (auto& WeakClient : RegisteredClients)
	{
		if (URfsnNpcClientComponent* Client = WeakClient.Get())
		{
			Client->Affinity = 0.0f;
			Client->Relationship = TEXT("Stranger");
		}
	}
}

TArray<FString> URfsnRelationshipManager::GetAllNpcIds() const
{
	TArray<FString> Ids;
	if (SaveData)
	{
		SaveData->NpcRelationships.GetKeys(Ids);
	}
	return Ids;
}

void URfsnRelationshipManager::SyncClientFromSaveData(URfsnNpcClientComponent* Client)
{
	if (!Client || !SaveData)
	{
		return;
	}

	if (SaveData->NpcRelationships.Contains(Client->NpcId))
	{
		const FRfsnNpcRelationship& Rel = SaveData->NpcRelationships[Client->NpcId];
		Client->Affinity = Rel.Affinity;
		Client->Relationship = Rel.Relationship;

		RFSN_VERBOSE("Synced client %s from save: Affinity=%.2f, Relationship=%s", *Client->NpcId, Rel.Affinity,
		             *Rel.Relationship);
	}
}

void URfsnRelationshipManager::SyncSaveDataFromClient(URfsnNpcClientComponent* Client)
{
	if (!Client || !SaveData)
	{
		return;
	}

	FRfsnNpcRelationship& Rel = SaveData->GetOrCreateRelationship(Client->NpcId);
	Rel.Affinity = Client->Affinity;
	Rel.Relationship = Client->Relationship;
	Rel.LastInteraction = FDateTime::Now();
}
