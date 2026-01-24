// RFSN NPC Awareness Implementation

#include "RfsnNpcAwareness.h"
#include "RfsnLogging.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

URfsnNpcAwareness::URfsnNpcAwareness()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 updates per second
}

void URfsnNpcAwareness::BeginPlay()
{
	Super::BeginPlay();
	RFSN_LOG(TEXT("NpcAwareness initialized for %s"), *GetOwner()->GetName());
}

void URfsnNpcAwareness::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentTarget)
	{
		UpdateVisualDetection(DeltaTime);
	}
	else
	{
		// Decay awareness when no target
		if (AwarenessValue > 0.0f)
		{
			AwarenessValue = FMath::Max(0.0f, AwarenessValue - AwarenessDecayRate * DeltaTime);
			UpdateAwarenessLevel();
		}
	}

	// Update time since detection
	if (!bCanSeeTarget)
	{
		TimeSinceDetection += DeltaTime;

		// Return to unaware after alert duration
		if (CurrentAwareness >= ERfsnAwarenessLevel::Alerted && TimeSinceDetection > AlertDuration)
		{
			AwarenessValue = InvestigateThreshold - 0.1f;
			UpdateAwarenessLevel();
		}
	}
	else
	{
		TimeSinceDetection = 0.0f;
	}

	// Clear old events
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RecentEvents.RemoveAll([CurrentTime](const FRfsnDetectionEvent& Event)
	                       { return (CurrentTime - Event.Timestamp) > 30.0f; });
}

void URfsnNpcAwareness::UpdateVisualDetection(float DeltaTime)
{
	if (!CurrentTarget)
	{
		bCanSeeTarget = false;
		return;
	}

	bCanSeeTarget = CanSeeActor(CurrentTarget);

	if (bCanSeeTarget)
	{
		float Visibility = CalculateVisibility(CurrentTarget);
		AwarenessValue = FMath::Min(1.0f, AwarenessValue + AwarenessGainRate * Visibility * DeltaTime);
		LastKnownLocation = CurrentTarget->GetActorLocation();
	}
	else
	{
		// Decay when can't see
		AwarenessValue = FMath::Max(0.0f, AwarenessValue - AwarenessDecayRate * DeltaTime * 0.5f);
	}

	UpdateAwarenessLevel();
}

void URfsnNpcAwareness::UpdateAwarenessLevel()
{
	ERfsnAwarenessLevel NewLevel;

	if (AwarenessValue >= AlertedThreshold)
	{
		NewLevel = ERfsnAwarenessLevel::Alerted;
	}
	else if (AwarenessValue >= InvestigateThreshold)
	{
		NewLevel = ERfsnAwarenessLevel::Investigating;
	}
	else if (AwarenessValue >= SuspiciousThreshold)
	{
		NewLevel = ERfsnAwarenessLevel::Suspicious;
	}
	else
	{
		NewLevel = ERfsnAwarenessLevel::Unaware;
	}

	if (NewLevel != CurrentAwareness)
	{
		ERfsnAwarenessLevel OldLevel = CurrentAwareness;
		CurrentAwareness = NewLevel;
		OnAwarenessChanged.Broadcast(NewLevel, OldLevel);

		if (NewLevel == ERfsnAwarenessLevel::Alerted && CurrentTarget)
		{
			OnTargetDetected.Broadcast(CurrentTarget);
		}

		RFSN_LOG(TEXT("%s awareness: %s -> %s (%.2f)"), *GetOwner()->GetName(), *AwarenessToString(OldLevel),
		         *AwarenessToString(NewLevel), AwarenessValue);
	}
}

bool URfsnNpcAwareness::CanSeeActor(AActor* Target)
{
	if (!Target)
	{
		return false;
	}

	FVector MyLocation = GetOwner()->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();

	// Check distance
	float Distance = FVector::Dist(MyLocation, TargetLocation);
	if (Distance > SightRange)
	{
		return false;
	}

	// Check FOV
	if (!IsInFieldOfView(TargetLocation))
	{
		return false;
	}

	// Check line of sight
	if (!HasLineOfSight(Target))
	{
		return false;
	}

	return true;
}

bool URfsnNpcAwareness::CanHearSound(FVector SoundLocation, float SoundLoudness)
{
	FVector MyLocation = GetOwner()->GetActorLocation();
	float Distance = FVector::Dist(MyLocation, SoundLocation);

	float EffectiveRange = HearingRange * SoundLoudness * HearingSensitivity;
	return Distance <= EffectiveRange;
}

void URfsnNpcAwareness::ReportSound(FVector SoundLocation, float Loudness, AActor* Source)
{
	if (!CanHearSound(SoundLocation, Loudness))
	{
		return;
	}

	// Create detection event
	FRfsnDetectionEvent Event;
	Event.Location = SoundLocation;
	Event.Strength = Loudness * HearingSensitivity;
	Event.Type = TEXT("Sound");
	Event.DetectedActor = Source;
	Event.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RecentEvents.Add(Event);

	// Increase awareness
	float AwarenessGain = Event.Strength * 0.3f;
	AwarenessValue = FMath::Min(1.0f, AwarenessValue + AwarenessGain);
	LastKnownLocation = SoundLocation;

	if (Source && !CurrentTarget)
	{
		CurrentTarget = Source;
	}

	UpdateAwarenessLevel();
	OnSuspiciousSound.Broadcast(SoundLocation);

	RFSN_LOG(TEXT("%s heard sound (loudness: %.2f)"), *GetOwner()->GetName(), Loudness);
}

void URfsnNpcAwareness::AlertToTarget(AActor* Target)
{
	CurrentTarget = Target;
	AwarenessValue = 1.0f;
	bCanSeeTarget = CanSeeActor(Target);
	if (Target)
	{
		LastKnownLocation = Target->GetActorLocation();
	}
	UpdateAwarenessLevel();
}

void URfsnNpcAwareness::ResetAwareness()
{
	CurrentAwareness = ERfsnAwarenessLevel::Unaware;
	AwarenessValue = 0.0f;
	CurrentTarget = nullptr;
	bCanSeeTarget = false;
	TimeSinceDetection = 0.0f;
	RecentEvents.Empty();
}

FVector URfsnNpcAwareness::GetInvestigationLocation() const
{
	if (LastKnownLocation != FVector::ZeroVector)
	{
		return LastKnownLocation;
	}

	// Return last sound location
	if (RecentEvents.Num() > 0)
	{
		return RecentEvents.Last().Location;
	}

	return GetOwner()->GetActorLocation();
}

FString URfsnNpcAwareness::GetAwarenessContext() const
{
	switch (CurrentAwareness)
	{
	case ERfsnAwarenessLevel::Suspicious:
		return TEXT("NPC senses something is off, looking around cautiously.");
	case ERfsnAwarenessLevel::Investigating:
		return TEXT("NPC is actively searching for the source of a disturbance.");
	case ERfsnAwarenessLevel::Alerted:
		return TEXT("NPC has detected an intruder and is on high alert.");
	case ERfsnAwarenessLevel::Hostile:
		return TEXT("NPC is hostile and ready to attack.");
	default:
		return TEXT("");
	}
}

FString URfsnNpcAwareness::AwarenessToString(ERfsnAwarenessLevel Level)
{
	switch (Level)
	{
	case ERfsnAwarenessLevel::Unaware:
		return TEXT("Unaware");
	case ERfsnAwarenessLevel::Suspicious:
		return TEXT("Suspicious");
	case ERfsnAwarenessLevel::Investigating:
		return TEXT("Investigating");
	case ERfsnAwarenessLevel::Alerted:
		return TEXT("Alerted");
	case ERfsnAwarenessLevel::Hostile:
		return TEXT("Hostile");
	default:
		return TEXT("Unknown");
	}
}

float URfsnNpcAwareness::CalculateVisibility(AActor* Target) const
{
	if (!Target)
	{
		return 0.0f;
	}

	FVector MyLocation = GetOwner()->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	float Distance = FVector::Dist(MyLocation, TargetLocation);

	// Distance factor
	float DistanceFactor = 1.0f - FMath::Clamp(Distance / SightRange, 0.0f, 1.0f);

	// FOV factor (lower in peripheral)
	FVector ToTarget = (TargetLocation - MyLocation).GetSafeNormal();
	FVector Forward = GetOwner()->GetActorForwardVector();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Forward, ToTarget)));

	float FOVFactor = 1.0f;
	if (Angle > FieldOfView * 0.5f)
	{
		// In peripheral vision
		FOVFactor = 0.3f;
	}

	// Movement factor (moving targets easier to see)
	float MovementFactor = 1.0f;
	if (Target->GetVelocity().Size() > 50.0f)
	{
		MovementFactor = 1.3f;
	}

	return DistanceFactor * FOVFactor * MovementFactor;
}

bool URfsnNpcAwareness::HasLineOfSight(AActor* Target) const
{
	if (!Target || !GetWorld())
	{
		return false;
	}

	FVector Start = GetOwner()->GetActorLocation() + FVector(0, 0, 50); // Eye level
	FVector End = Target->GetActorLocation() + FVector(0, 0, 50);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	if (!bHit)
	{
		return true;
	}

	return HitResult.GetActor() == Target;
}

bool URfsnNpcAwareness::IsInFieldOfView(FVector Location) const
{
	FVector MyLocation = GetOwner()->GetActorLocation();
	FVector Forward = GetOwner()->GetActorForwardVector();
	FVector ToLocation = (Location - MyLocation).GetSafeNormal();

	float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Forward, ToLocation)));
	return Angle <= PeripheralFOV * 0.5f;
}
