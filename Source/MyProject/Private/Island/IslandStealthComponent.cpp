#include "IslandStealthComponent.h"

UIslandStealthComponent::UIslandStealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIslandStealthComponent::UpdateStealth(float DeltaTime, const FVector& Velocity, bool bIsCrouched,
                                            bool bIsSprinting)
{
	const float Speed = Velocity.Size2D();
	const bool bIsMoving = Speed > 10.0f;

	if (!bIsMoving)
	{
		CurrentNoise = 0.0f;
		CurrentVisibility = bIsCrouched ? 0.45f : 0.8f;
	}
	else if (bIsSprinting)
	{
		CurrentNoise = 1.0f;
		CurrentVisibility = 1.35f;
	}
	else if (bIsCrouched)
	{
		CurrentNoise = 0.2f;
		CurrentVisibility = 0.55f;
	}
	else
	{
		CurrentNoise = 0.6f;
		CurrentVisibility = 1.0f;
	}

	if (bFlashlightOn)
	{
		CurrentVisibility += 0.45f;
	}

	CurrentNoise = FMath::Clamp(CurrentNoise, 0.0f, 1.5f);
	CurrentVisibility = FMath::Clamp(CurrentVisibility, 0.2f, 2.0f);

	if (DeltaTime <= 0.0f)
	{
		CurrentNoise = 0.0f;
	}
}

float UIslandStealthComponent::GetNoiseLoudness() const
{
	return CurrentNoise;
}

float UIslandStealthComponent::GetVisibilityMultiplier() const
{
	return CurrentVisibility;
}

void UIslandStealthComponent::SetFlashlightEnabled(bool bEnabled)
{
	bFlashlightOn = bEnabled;
}
