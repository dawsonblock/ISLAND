#include "Island/IslandDirectorSubsystem.h"
#include "TimerManager.h"

void UIslandDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UIslandDirectorSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DecayTimer);
	}
	Super::Deinitialize();
}

void UIslandDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	InWorld.GetTimerManager().SetTimer(DecayTimer, this, &UIslandDirectorSubsystem::DecayAlert, 1.0f, true);
}

void UIslandDirectorSubsystem::AddAlert(float Amount)
{
	AlertLevel = FMath::Clamp(AlertLevel + Amount, 0.0f, 100.0f);
	
	UpdateIntensityState();

	// Broadcast every 10% change
	if (FMath::Abs(AlertLevel - LastBroadcastAlert) >= 10.0f)
	{
		OnAlertThresholdReached.Broadcast(AlertLevel);
		LastBroadcastAlert = AlertLevel;
	}
}

void UIslandDirectorSubsystem::AddAlertFromTowerRepair(float Amount)
{
	AddAlert(Amount);
}

void UIslandDirectorSubsystem::AddAlertFromTransmissionPulse(float Amount)
{
	AddAlert(Amount);
}

void UIslandDirectorSubsystem::AddAlertFromPlayerNoise(float Amount)
{
	AddAlert(Amount);
}

void UIslandDirectorSubsystem::AddAlertFromCultSpotting(float Amount)
{
	AddAlert(Amount);
}

void UIslandDirectorSubsystem::DecayAlert()
{
	AlertLevel = FMath::Max(0.0f, AlertLevel - AlertDecayRate);
	
	UpdateIntensityState();

	if (FMath::Abs(AlertLevel - LastBroadcastAlert) >= 10.0f)
	{
		OnAlertThresholdReached.Broadcast(AlertLevel);
		LastBroadcastAlert = AlertLevel;
	}
}

void UIslandDirectorSubsystem::UpdateIntensityState()
{
	EIslandIntensityState NewState = EIslandIntensityState::Passive;

	if (AlertLevel >= 90.0f) NewState = EIslandIntensityState::Overwhelmed;
	else if (AlertLevel >= 60.0f) NewState = EIslandIntensityState::Hostile;
	else if (AlertLevel >= 30.0f) NewState = EIslandIntensityState::Alerted;

	if (NewState != CurrentIntensity)
	{
		CurrentIntensity = NewState;
		OnIntensityStateChanged.Broadcast(CurrentIntensity);
	}
}

float UIslandDirectorSubsystem::GetCurrentSpawnInterval() const
{
	switch (CurrentIntensity)
	{
	case EIslandIntensityState::Alerted:
		return AlertedSpawnInterval;
	case EIslandIntensityState::Hostile:
		return HostileSpawnInterval;
	case EIslandIntensityState::Overwhelmed:
		return OverwhelmedSpawnInterval;
	case EIslandIntensityState::Passive:
	default:
		return PassiveSpawnInterval;
	}
}

float UIslandDirectorSubsystem::GetCurrentSearchDuration() const
{
	switch (CurrentIntensity)
	{
	case EIslandIntensityState::Alerted:
		return AlertedSearchDuration;
	case EIslandIntensityState::Hostile:
		return HostileSearchDuration;
	case EIslandIntensityState::Overwhelmed:
		return OverwhelmedSearchDuration;
	case EIslandIntensityState::Passive:
	default:
		return PassiveSearchDuration;
	}
}

int32 UIslandDirectorSubsystem::GetDesiredActiveCultists() const
{
	switch (CurrentIntensity)
	{
	case EIslandIntensityState::Alerted:
		return AlertedCultistCount;
	case EIslandIntensityState::Hostile:
		return HostileCultistCount;
	case EIslandIntensityState::Overwhelmed:
		return OverwhelmedCultistCount;
	case EIslandIntensityState::Passive:
	default:
		return 0;
	}
}
