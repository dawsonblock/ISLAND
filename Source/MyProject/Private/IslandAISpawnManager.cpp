#include "IslandAISpawnManager.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AIslandAISpawnManager::AIslandAISpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AIslandAISpawnManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AIslandRadioTower> It(GetWorld()); It; ++It)
	{
		CachedTower = *It;
		CachedTower->OnStateChanged.AddDynamic(this, &AIslandAISpawnManager::OnTowerStateChanged);
		break;
	}

	if (UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>())
	{
		Director->OnIntensityStateChanged.AddDynamic(this, &AIslandAISpawnManager::OnIntensityChanged);
		OnIntensityChanged(Director->CurrentIntensity);
	}
}

void AIslandAISpawnManager::OnIntensityChanged(EIslandIntensityState NewState)
{
	CurrentIntensity = NewState;
	
	if (CurrentIntensity != EIslandIntensityState::Passive)
	{
		StartSpawning();
	}
	else
	{
		StopSpawning();
	}
}

void AIslandAISpawnManager::OnTowerStateChanged(ERadioTowerState NewState)
{
	// Tower transmit state now influences the Director directly (logic to be added in Director)
}

void AIslandAISpawnManager::StartSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	float NextDelay = GetCurrentInterval();
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AIslandAISpawnManager::TrySpawnHunter, NextDelay, false);
}

void AIslandAISpawnManager::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
}

float AIslandAISpawnManager::GetCurrentInterval() const
{
	switch (CurrentIntensity)
	{
	case EIslandIntensityState::Alerted: return AlertedInterval;
	case EIslandIntensityState::Hostile: return HostileInterval;
	case EIslandIntensityState::Overwhelmed: return OverwhelmedInterval;
	case EIslandIntensityState::Passive: default: return PassiveInterval;
	}
}

void AIslandAISpawnManager::TrySpawnHunter()
{
	if (!HunterClass) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	FVector Origin = PlayerPawn->GetActorLocation();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (NavSys)
	{
		FNavLocation SpawnLoc;
		bool bFound = false;
		int32 Tries = 0;

		while (!bFound && Tries < 10)
		{
			Tries++;
			if (NavSys->GetRandomReachablePointInRadius(Origin, SpawnRadius, SpawnLoc))
			{
				if (FVector::Dist(SpawnLoc.Location, Origin) >= MinSpawnDistance)
				{
					bFound = true;
				}
			}
		}

		if (bFound)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			GetWorld()->SpawnActor<APawn>(HunterClass, SpawnLoc.Location, FRotator::ZeroRotator, Params);
		}
	}

	// Schedule next spawn if not passive
	if (CurrentIntensity != EIslandIntensityState::Passive)
	{
		StartSpawning();
	}
}
