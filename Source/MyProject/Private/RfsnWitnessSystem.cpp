// RFSN Witness System Implementation

#include "RfsnWitnessSystem.h"
#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void URfsnWitnessSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RFSN_LOG(TEXT("WitnessSystem initialized"));
}

void URfsnWitnessSystem::Deinitialize()
{
	AllEvents.Empty();
	NpcKnowledge.Empty();
	Super::Deinitialize();
}

FGuid URfsnWitnessSystem::RecordPlayerAction(ERfsnWitnessEventType EventType, const FString& Description,
                                             FVector Location, const FString& TargetNpcId, float Importance,
                                             bool bPositive)
{
	// Create event
	FRfsnWitnessEvent Event;
	Event.EventType = EventType;
	Event.Description = Description;
	Event.Location = Location;
	Event.TargetNpcId = TargetNpcId;
	Event.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
	Event.bIsPositive = bPositive;
	Event.GameTimeWhenOccurred = UGameplayStatics::GetTimeSeconds(GetWorld());

	// Find witnesses
	TArray<FString> Witnesses = FindWitnesses(Location);
	Event.OriginalWitnesses = Witnesses;

	// Register knowledge for each witness
	for (const FString& WitnessId : Witnesses)
	{
		RegisterWitness(Event.EventId, WitnessId, 1.0f, TEXT("witnessed"));
		OnEventWitnessed.Broadcast(Event, WitnessId);
	}

	// Store event
	AllEvents.Add(Event);

	// Trim old events if over limit
	while (AllEvents.Num() > MaxTrackedEvents)
	{
		AllEvents.RemoveAt(0);
	}

	RFSN_LOG(TEXT("Recorded event: %s (witnessed by %d NPCs)"), *Description, Witnesses.Num());
	return Event.EventId;
}

TArray<FString> URfsnWitnessSystem::FindWitnesses(FVector Location, float Radius)
{
	TArray<FString> Witnesses;

	if (Radius < 0.0f)
	{
		Radius = WitnessRadius;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return Witnesses;
	}

	// Find all NPCs with RfsnNpcClientComponent in range
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		URfsnNpcClientComponent* NpcClient = Actor->FindComponentByClass<URfsnNpcClientComponent>();

		if (!NpcClient || NpcClient->NpcId.IsEmpty())
		{
			continue;
		}

		float Distance = FVector::Dist(Location, Actor->GetActorLocation());
		if (Distance <= Radius)
		{
			// Check line of sight (optional, simplified)
			Witnesses.Add(NpcClient->NpcId);
		}
	}

	return Witnesses;
}

void URfsnWitnessSystem::RegisterWitness(const FGuid& EventId, const FString& NpcId, float Accuracy,
                                         const FString& Source)
{
	FRfsnEventKnowledge Knowledge;
	Knowledge.EventId = EventId;
	Knowledge.Accuracy = Accuracy;
	Knowledge.Source = Source;
	Knowledge.bWillGossip = FMath::FRand() < 0.7f; // 70% chance to gossip

	// Calculate opinion based on faction
	const FRfsnWitnessEvent* Event = FindEvent(EventId);
	if (Event)
	{
		// Find NPC's faction
		UWorld* World = GetWorld();
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				URfsnNpcClientComponent* Client = (*It)->FindComponentByClass<URfsnNpcClientComponent>();
				if (Client && Client->NpcId == NpcId)
				{
					// TODO: Get faction from faction system once integrated
					Knowledge.Opinion = CalculateOpinion(TEXT(""), *Event);
					break;
				}
			}
		}

		// Update event's informed list
		FRfsnWitnessEvent* MutableEvent = FindEvent(EventId);
		if (MutableEvent && !MutableEvent->InformedNpcs.Contains(NpcId))
		{
			MutableEvent->InformedNpcs.Add(NpcId);
		}
	}

	// Store knowledge
	if (!NpcKnowledge.Contains(NpcId))
	{
		NpcKnowledge.Add(NpcId, TMap<FGuid, FRfsnEventKnowledge>());
	}
	NpcKnowledge[NpcId].Add(EventId, Knowledge);
}

bool URfsnWitnessSystem::DoesNpcKnow(const FString& NpcId, const FGuid& EventId) const
{
	const TMap<FGuid, FRfsnEventKnowledge>* KnowledgeMap = NpcKnowledge.Find(NpcId);
	if (!KnowledgeMap)
	{
		return false;
	}
	return KnowledgeMap->Contains(EventId);
}

FRfsnEventKnowledge URfsnWitnessSystem::GetNpcKnowledge(const FString& NpcId, const FGuid& EventId) const
{
	const TMap<FGuid, FRfsnEventKnowledge>* KnowledgeMap = NpcKnowledge.Find(NpcId);
	if (KnowledgeMap)
	{
		const FRfsnEventKnowledge* Knowledge = KnowledgeMap->Find(EventId);
		if (Knowledge)
		{
			return *Knowledge;
		}
	}
	return FRfsnEventKnowledge();
}

TArray<FRfsnWitnessEvent> URfsnWitnessSystem::GetNpcKnownEvents(const FString& NpcId) const
{
	TArray<FRfsnWitnessEvent> Result;

	const TMap<FGuid, FRfsnEventKnowledge>* KnowledgeMap = NpcKnowledge.Find(NpcId);
	if (!KnowledgeMap)
	{
		return Result;
	}

	for (const auto& Pair : *KnowledgeMap)
	{
		const FRfsnWitnessEvent* Event = FindEvent(Pair.Key);
		if (Event && !Event->bExpired)
		{
			Result.Add(*Event);
		}
	}

	return Result;
}

TArray<FRfsnWitnessEvent> URfsnWitnessSystem::GetRecentEvents(int32 MaxCount) const
{
	TArray<FRfsnWitnessEvent> Result;

	int32 StartIdx = FMath::Max(0, AllEvents.Num() - MaxCount);
	for (int32 i = AllEvents.Num() - 1; i >= StartIdx; --i)
	{
		if (!AllEvents[i].bExpired)
		{
			Result.Add(AllEvents[i]);
		}
	}

	return Result;
}

FString URfsnWitnessSystem::GetGossipForNpc(const FString& NpcId) const
{
	TArray<FRfsnWitnessEvent> KnownEvents = GetNpcKnownEvents(NpcId);
	if (KnownEvents.Num() == 0)
	{
		return TEXT("");
	}

	// Pick a random event they know about and will gossip
	TArray<FRfsnWitnessEvent> GossipEvents;
	for (const FRfsnWitnessEvent& Event : KnownEvents)
	{
		FRfsnEventKnowledge Knowledge = GetNpcKnowledge(NpcId, Event.EventId);
		if (Knowledge.bWillGossip && Knowledge.ShareCount < 3)
		{
			GossipEvents.Add(Event);
		}
	}

	if (GossipEvents.Num() == 0)
	{
		return TEXT("");
	}

	// Return most important/recent gossip
	GossipEvents.Sort([](const FRfsnWitnessEvent& A, const FRfsnWitnessEvent& B)
	                  { return A.Importance > B.Importance; });

	return FString::Printf(TEXT("I heard that %s"), *GossipEvents[0].Description);
}

FString URfsnWitnessSystem::GetWitnessContext(const FString& NpcId) const
{
	TArray<FRfsnWitnessEvent> KnownEvents = GetNpcKnownEvents(NpcId);
	if (KnownEvents.Num() == 0)
	{
		return TEXT("");
	}

	FString Context = TEXT("This NPC knows about: ");
	int32 Count = 0;

	for (const FRfsnWitnessEvent& Event : KnownEvents)
	{
		if (Count >= 3)
			break; // Limit context length

		FRfsnEventKnowledge Knowledge = GetNpcKnowledge(NpcId, Event.EventId);
		FString Accuracy = Knowledge.Accuracy > 0.8f   ? TEXT("clearly saw")
		                   : Knowledge.Accuracy > 0.5f ? TEXT("heard about")
		                                               : TEXT("vaguely heard");

		Context +=
		    FString::Printf(TEXT("[%s %s: %s] "), *Accuracy, *EventTypeToString(Event.EventType), *Event.Description);
		Count++;
	}

	return Context;
}

void URfsnWitnessSystem::SpreadRumor(const FGuid& EventId, const FString& FromNpc, const FString& ToNpc)
{
	if (!DoesNpcKnow(FromNpc, EventId))
	{
		return; // From NPC doesn't know
	}

	if (DoesNpcKnow(ToNpc, EventId))
	{
		return; // To NPC already knows
	}

	FRfsnEventKnowledge FromKnowledge = GetNpcKnowledge(FromNpc, EventId);

	// Degrade accuracy based on rumor hop
	float NewAccuracy = FMath::Max(0.1f, FromKnowledge.Accuracy - AccuracyDecayPerHop);

	RegisterWitness(EventId, ToNpc, NewAccuracy, FString::Printf(TEXT("heard from %s"), *FromNpc));

	// Increment share count
	if (TMap<FGuid, FRfsnEventKnowledge>* KnowledgeMap = NpcKnowledge.Find(FromNpc))
	{
		if (FRfsnEventKnowledge* Knowledge = KnowledgeMap->Find(EventId))
		{
			Knowledge->ShareCount++;
		}
	}

	OnRumorSpread.Broadcast(EventId, FromNpc, ToNpc);
	RFSN_LOG(TEXT("Rumor spread: %s -> %s (accuracy: %.2f)"), *FromNpc, *ToNpc, NewAccuracy);
}

void URfsnWitnessSystem::TickRumorSpreading()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Build list of all NPC pairs in proximity
	TArray<TPair<FString, FString>> NpcPairs;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		URfsnNpcClientComponent* ClientA = (*It)->FindComponentByClass<URfsnNpcClientComponent>();
		if (!ClientA || ClientA->NpcId.IsEmpty())
		{
			continue;
		}

		for (TActorIterator<AActor> It2(World); It2; ++It2)
		{
			if (*It == *It2)
			{
				continue;
			}

			URfsnNpcClientComponent* ClientB = (*It2)->FindComponentByClass<URfsnNpcClientComponent>();
			if (!ClientB || ClientB->NpcId.IsEmpty())
			{
				continue;
			}

			float Distance = FVector::Dist((*It)->GetActorLocation(), (*It2)->GetActorLocation());
			if (Distance < 500.0f) // Conversation distance
			{
				NpcPairs.Add(TPair<FString, FString>(ClientA->NpcId, ClientB->NpcId));
			}
		}
	}

	// For each pair, chance to spread rumor
	for (const auto& Pair : NpcPairs)
	{
		if (FMath::FRand() > RumorSpreadChance)
		{
			continue;
		}

		// Get events FromNpc knows but ToNpc doesn't
		TArray<FRfsnWitnessEvent> KnownEvents = GetNpcKnownEvents(Pair.Key);
		for (const FRfsnWitnessEvent& Event : KnownEvents)
		{
			if (!DoesNpcKnow(Pair.Value, Event.EventId))
			{
				FRfsnEventKnowledge Knowledge = GetNpcKnowledge(Pair.Key, Event.EventId);
				if (Knowledge.bWillGossip)
				{
					SpreadRumor(Event.EventId, Pair.Key, Pair.Value);
					break; // One rumor per tick per pair
				}
			}
		}
	}
}

void URfsnWitnessSystem::CleanupExpiredEvents()
{
	float CurrentTime = GetWorld() ? UGameplayStatics::GetTimeSeconds(GetWorld()) : 0.0f;
	float ExpiryTime = MemoryDurationHours * 3600.0f; // Convert hours to seconds

	for (FRfsnWitnessEvent& Event : AllEvents)
	{
		if (!Event.bExpired && (CurrentTime - Event.GameTimeWhenOccurred) > ExpiryTime)
		{
			Event.bExpired = true;
		}
	}

	// Remove very old events
	AllEvents.RemoveAll([this, CurrentTime, ExpiryTime](const FRfsnWitnessEvent& Event)
	                    { return Event.bExpired && (CurrentTime - Event.GameTimeWhenOccurred) > (ExpiryTime * 2.0f); });
}

FRfsnWitnessEvent* URfsnWitnessSystem::FindEvent(const FGuid& EventId)
{
	return AllEvents.FindByPredicate([EventId](const FRfsnWitnessEvent& Event) { return Event.EventId == EventId; });
}

const FRfsnWitnessEvent* URfsnWitnessSystem::FindEvent(const FGuid& EventId) const
{
	return AllEvents.FindByPredicate([EventId](const FRfsnWitnessEvent& Event) { return Event.EventId == EventId; });
}

float URfsnWitnessSystem::CalculateOpinion(const FString& NpcFaction, const FRfsnWitnessEvent& Event) const
{
	float Opinion = Event.bIsPositive ? 0.5f : -0.5f;

	// Faction loyalty
	if (!Event.TargetFaction.IsEmpty())
	{
		if (NpcFaction.Equals(Event.TargetFaction, ESearchCase::IgnoreCase))
		{
			// Same faction as target - strong opinion
			Opinion *= 2.0f;
		}
	}

	// Event type modifiers
	switch (Event.EventType)
	{
	case ERfsnWitnessEventType::Murder:
		Opinion = Event.bIsPositive ? 0.2f : -0.9f; // Murder is bad unless killing enemies
		break;
	case ERfsnWitnessEventType::Help:
		Opinion = 0.7f;
		break;
	case ERfsnWitnessEventType::Theft:
		Opinion = -0.6f;
		break;
	default:
		break;
	}

	return FMath::Clamp(Opinion, -1.0f, 1.0f);
}

FString URfsnWitnessSystem::EventTypeToString(ERfsnWitnessEventType Type)
{
	switch (Type)
	{
	case ERfsnWitnessEventType::Combat:
		return TEXT("Combat");
	case ERfsnWitnessEventType::Theft:
		return TEXT("Theft");
	case ERfsnWitnessEventType::Help:
		return TEXT("Help");
	case ERfsnWitnessEventType::Trade:
		return TEXT("Trade");
	case ERfsnWitnessEventType::Murder:
		return TEXT("Murder");
	case ERfsnWitnessEventType::Dialogue:
		return TEXT("Dialogue");
	case ERfsnWitnessEventType::QuestComplete:
		return TEXT("Quest");
	case ERfsnWitnessEventType::Trespass:
		return TEXT("Trespass");
	default:
		return TEXT("Custom");
	}
}
