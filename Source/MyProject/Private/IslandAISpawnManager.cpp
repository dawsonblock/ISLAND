#include "IslandAISpawnManager.h"
#include "CultistAIController.h"
#include "CultistCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

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
	if (NewState == ERadioTowerState::Transmitting || NewState == ERadioTowerState::ExtractWindow)
	{
		SpawnCultistNearTower();
		StartSpawning();
	}
}

void AIslandAISpawnManager::StartSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	float NextDelay = 10.0f;
	if (UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>())
	{
		NextDelay = Director->GetCurrentSpawnInterval();
	}

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AIslandAISpawnManager::TrySpawnCultist, NextDelay, false);
}

void AIslandAISpawnManager::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
}

void AIslandAISpawnManager::SpawnCultistNearTower()
{
	if (CachedTower)
	{
		const FVector TowerLocation = CachedTower->GetActorLocation();
		SpawnCultistAroundLocation(TowerLocation, nullptr, true);
	}
}

void AIslandAISpawnManager::SpawnCultistNearLastKnownPlayerLocation(const FVector& Location)
{
	FVector InvestigationLocation = Location;
	SpawnCultistAroundLocation(Location, &InvestigationLocation, false);
}

void AIslandAISpawnManager::RegisterCultistDeath(ACultistCharacter* Cultist)
{
	AliveCultistCount = FMath::Max(0, AliveCultistCount - 1);
}

void AIslandAISpawnManager::TrySpawnCultist()
{
	if (!CultistClass)
	{
		return;
	}

	UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>();
	const bool bTowerPressure = CachedTower &&
	                            (CachedTower->State == ERadioTowerState::Transmitting ||
	                             CachedTower->State == ERadioTowerState::ExtractWindow);
	const int32 DesiredCultists =
	    Director ? FMath::Max(Director->GetDesiredActiveCultists(), bTowerPressure ? 3 : 0)
	             : (bTowerPressure ? 3 : 0);
	if (AliveCultistCount >= FMath::Min(MaxAliveCultists, DesiredCultists))
	{
		if (CurrentIntensity != EIslandIntensityState::Passive || bTowerPressure)
		{
			StartSpawning();
		}
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn && !bTowerPressure)
	{
		return;
	}

	if (bTowerPressure && CachedTower)
	{
		SpawnCultistNearTower();
	}
	else if (PlayerPawn)
	{
		FVector InvestigationLocation = PlayerPawn->GetActorLocation();
		SpawnCultistAroundLocation(PlayerPawn->GetActorLocation(), &InvestigationLocation, false);
	}

	if (CurrentIntensity != EIslandIntensityState::Passive || bTowerPressure)
	{
		StartSpawning();
	}
}

bool AIslandAISpawnManager::SpawnCultistAroundLocation(const FVector& Origin, FVector* ForcedInvestigationLocation,
                                                       bool bGuardTower)
{
	if (!CultistClass || AliveCultistCount >= MaxAliveCultists)
	{
		return false;
	}

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
			if (ACultistCharacter* Cultist =
			        GetWorld()->SpawnActor<ACultistCharacter>(CultistClass, SpawnLoc.Location,
			                                                 FRotator::ZeroRotator, Params))
			{
				AliveCultistCount++;
				Cultist->OnCultistDied.AddDynamic(this, &AIslandAISpawnManager::RegisterCultistDeath);
				if (!Cultist->GetController())
				{
					Cultist->SpawnDefaultController();
				}

				if (ACultistAIController* CultistController = Cast<ACultistAIController>(Cultist->GetController()))
				{
					if (bGuardTower)
					{
						CultistController->SetGuardLocation(Origin);
					}
					if (ForcedInvestigationLocation)
					{
						CultistController->SetInvestigationLocation(*ForcedInvestigationLocation);
					}
				}

				if (bGuardTower)
				{
					Cultist->SetCultState(ECultistState::GuardTower);
				}

				return true;
			}
		}
	}

	return false;
}
