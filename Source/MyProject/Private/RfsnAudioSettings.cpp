// RFSN Audio Settings Implementation

#include "RfsnAudioSettings.h"
#include "RfsnTtsAudioComponent.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

URfsnAudioSettings::URfsnAudioSettings()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Update 10 times per second
}

void URfsnAudioSettings::BeginPlay()
{
	Super::BeginPlay();
}

void URfsnAudioSettings::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableAttenuation)
	{
		return;
	}

	// Auto-apply to TTS component if present
	if (URfsnTtsAudioComponent* TtsComp = GetOwner()->FindComponentByClass<URfsnTtsAudioComponent>())
	{
		ApplyToTtsComponent(TtsComp);
	}
}

float URfsnAudioSettings::GetDistanceVolume() const
{
	float Distance = GetDistanceToPlayer();

	if (Distance <= InnerRadius)
	{
		return MaxVolume;
	}

	if (Distance >= OuterRadius)
	{
		return MinVolume;
	}

	// Calculate attenuation
	float Range = OuterRadius - InnerRadius;
	float DistanceInRange = Distance - InnerRadius;
	float Fraction = DistanceInRange / Range;

	// Apply exponent for non-linear falloff
	float Attenuation = FMath::Pow(1.0f - Fraction, AttenuationExponent);

	return FMath::Lerp(MinVolume, MaxVolume, Attenuation);
}

bool URfsnAudioSettings::IsOccluded() const
{
	if (!bEnableOcclusion)
	{
		return false;
	}

	return DoOcclusionTrace();
}

float URfsnAudioSettings::GetFinalVolume() const
{
	float Volume = GetDistanceVolume();

	if (IsOccluded())
	{
		Volume *= OcclusionMultiplier;
	}

	return Volume;
}

void URfsnAudioSettings::ApplyToTtsComponent(URfsnTtsAudioComponent* TtsComponent)
{
	if (!TtsComponent)
	{
		return;
	}

	TtsComponent->VolumeMultiplier = GetFinalVolume();
}

float URfsnAudioSettings::GetDistanceToPlayer() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn())
	{
		return OuterRadius; // Max distance if no player
	}

	return FVector::Dist(GetOwner()->GetActorLocation(), PC->GetPawn()->GetActorLocation());
}

bool URfsnAudioSettings::DoOcclusionTrace() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn())
	{
		return false;
	}

	FVector Start = GetOwner()->GetActorLocation();
	FVector End = PC->GetPawn()->GetActorLocation();

	// Offset to head height
	Start.Z += 150.0f;
	End.Z += 150.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(PC->GetPawn());

	FHitResult Hit;
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	return bHit;
}
