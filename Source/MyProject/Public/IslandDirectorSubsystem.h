#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandDirectorSubsystem.generated.h"

UENUM(BlueprintType)
enum class EIslandIntensityState : uint8
{
	Passive,
	Alerted,
	Hostile,
	Overwhelmed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAlertThresholdReached, float, NewAlertLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntensityStateChanged, EIslandIntensityState, NewState);

UCLASS()
class MYPROJECT_API UIslandDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Director")
	float AlertLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director")
	float AlertDecayRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director")
	float MinAlertForTower = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director")
	float MinAlertForTransmit = 50.0f;

	UPROPERTY(BlueprintAssignable, Category="Director")
	FAlertThresholdReached OnAlertThresholdReached;

	UPROPERTY(BlueprintAssignable, Category="Director")
	FOnIntensityStateChanged OnIntensityStateChanged;

	UPROPERTY(BlueprintReadOnly, Category="Director")
	EIslandIntensityState CurrentIntensity = EIslandIntensityState::Passive;

	UFUNCTION(BlueprintCallable, Category="Director")
	void AddAlert(float Amount);

	UFUNCTION(BlueprintCallable, Category="Director")
	float GetAlertLevel() const { return AlertLevel; }

	UFUNCTION(BlueprintPure, Category="Director")
	float GetNormalizedIntensity() const { return AlertLevel / 100.0f; }

	UFUNCTION(BlueprintCallable, Category="Director")
	bool CanUseTower() const { return AlertLevel >= MinAlertForTower; }

	UFUNCTION(BlueprintCallable, Category="Director")
	bool CanTransmit() const { return AlertLevel >= MinAlertForTransmit; }

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	FTimerHandle DecayTimer;
	float LastBroadcastAlert = 0.0f;
	void DecayAlert();
	void UpdateIntensityState();
};
