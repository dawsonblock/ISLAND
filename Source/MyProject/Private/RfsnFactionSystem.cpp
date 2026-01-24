// RFSN Faction System Implementation

#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"

void URfsnFactionSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreateDefaultFactions();
	RFSN_LOG("Faction system initialized with %d factions", Factions.Num());
}

void URfsnFactionSystem::RegisterFaction(const FRfsnFaction& Faction)
{
	Factions.Add(Faction.FactionId, Faction);
	RFSN_LOG("Registered faction: %s", *Faction.DisplayName);
}

bool URfsnFactionSystem::GetFaction(const FString& FactionId, FRfsnFaction& OutFaction)
{
	if (FRfsnFaction* Found = Factions.Find(FactionId))
	{
		OutFaction = *Found;
		return true;
	}
	return false;
}

TArray<FRfsnFaction> URfsnFactionSystem::GetAllFactions() const
{
	TArray<FRfsnFaction> Result;
	Factions.GenerateValueArray(Result);
	return Result;
}

float URfsnFactionSystem::GetReputation(const FString& FactionId) const
{
	if (const FRfsnFaction* Found = Factions.Find(FactionId))
	{
		return Found->Reputation;
	}
	return 0.0f;
}

void URfsnFactionSystem::ModifyReputation(const FString& FactionId, float Delta)
{
	if (FRfsnFaction* Found = Factions.Find(FactionId))
	{
		Found->Reputation = FMath::Clamp(Found->Reputation + Delta, -100.0f, 100.0f);
		OnFactionReputationChanged.Broadcast(FactionId, Found->Reputation);
		RFSN_LOG("Faction %s reputation changed by %.1f to %.1f", *FactionId, Delta, Found->Reputation);

		// Propagate to allies/enemies
		for (const FString& Ally : Found->Allies)
		{
			if (FRfsnFaction* AllyFaction = Factions.Find(Ally))
			{
				float AllyDelta = Delta * 0.5f; // Half effect on allies
				AllyFaction->Reputation = FMath::Clamp(AllyFaction->Reputation + AllyDelta, -100.0f, 100.0f);
			}
		}

		for (const FString& Enemy : Found->Enemies)
		{
			if (FRfsnFaction* EnemyFaction = Factions.Find(Enemy))
			{
				float EnemyDelta = -Delta * 0.5f; // Opposite effect on enemies
				EnemyFaction->Reputation = FMath::Clamp(EnemyFaction->Reputation + EnemyDelta, -100.0f, 100.0f);
			}
		}
	}
}

void URfsnFactionSystem::SetReputation(const FString& FactionId, float Value)
{
	if (FRfsnFaction* Found = Factions.Find(FactionId))
	{
		Found->Reputation = FMath::Clamp(Value, -100.0f, 100.0f);
		OnFactionReputationChanged.Broadcast(FactionId, Found->Reputation);
	}
}

FString URfsnFactionSystem::GetReputationTier(const FString& FactionId) const
{
	float Rep = GetReputation(FactionId);

	if (Rep <= -60.0f)
		return TEXT("Hostile");
	if (Rep <= -20.0f)
		return TEXT("Unfriendly");
	if (Rep <= 20.0f)
		return TEXT("Neutral");
	if (Rep <= 60.0f)
		return TEXT("Friendly");
	return TEXT("Allied");
}

bool URfsnFactionSystem::AreFactionsAllied(const FString& FactionA, const FString& FactionB) const
{
	if (const FRfsnFaction* Found = Factions.Find(FactionA))
	{
		return Found->Allies.Contains(FactionB);
	}
	return false;
}

bool URfsnFactionSystem::AreFactionsHostile(const FString& FactionA, const FString& FactionB) const
{
	if (const FRfsnFaction* Found = Factions.Find(FactionA))
	{
		return Found->Enemies.Contains(FactionB);
	}
	return false;
}

float URfsnFactionSystem::GetNpcAffinityFromFaction(const FString& FactionId) const
{
	// Convert -100 to 100 reputation to -1 to 1 affinity
	return GetReputation(FactionId) / 100.0f;
}

void URfsnFactionSystem::CreateDefaultFactions()
{
	// Survivors faction
	FRfsnFaction Survivors;
	Survivors.FactionId = TEXT("survivors");
	Survivors.DisplayName = TEXT("Survivors");
	Survivors.DefaultMood = TEXT("Cautious");
	Survivors.Reputation = 0.0f;
	RegisterFaction(Survivors);

	// Bandits faction
	FRfsnFaction Bandits;
	Bandits.FactionId = TEXT("bandits");
	Bandits.DisplayName = TEXT("Bandits");
	Bandits.DefaultMood = TEXT("Hostile");
	Bandits.Reputation = -40.0f;
	Bandits.Enemies.Add(TEXT("survivors"));
	Bandits.Enemies.Add(TEXT("military"));
	RegisterFaction(Bandits);

	// Military faction
	FRfsnFaction Military;
	Military.FactionId = TEXT("military");
	Military.DisplayName = TEXT("Military");
	Military.DefaultMood = TEXT("Suspicious");
	Military.Reputation = 20.0f;
	Military.Enemies.Add(TEXT("bandits"));
	RegisterFaction(Military);

	// Merchants faction
	FRfsnFaction Merchants;
	Merchants.FactionId = TEXT("merchants");
	Merchants.DisplayName = TEXT("Merchants");
	Merchants.DefaultMood = TEXT("Friendly");
	Merchants.Reputation = 30.0f;
	Merchants.Allies.Add(TEXT("survivors"));
	RegisterFaction(Merchants);

	// Cultists faction
	FRfsnFaction Cultists;
	Cultists.FactionId = TEXT("cultists");
	Cultists.DisplayName = TEXT("The Devoted");
	Cultists.DefaultMood = TEXT("Mysterious");
	Cultists.Reputation = -20.0f;
	RegisterFaction(Cultists);
}
