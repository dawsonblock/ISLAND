#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IslandStealthComponent.generated.h"

UCLASS(ClassGroup = (Island), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UIslandStealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIslandStealthComponent();

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	float CurrentNoise = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	float CurrentVisibility = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	bool bFlashlightOn = false;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void UpdateStealth(float DeltaTime, const FVector& Velocity, bool bIsCrouched, bool bIsSprinting);

	UFUNCTION(BlueprintPure, Category = "Stealth")
	float GetNoiseLoudness() const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	float GetVisibilityMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetFlashlightEnabled(bool bEnabled);
};
