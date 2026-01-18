#include "IslandDirectorSubsystem.h"
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
