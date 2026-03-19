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

	UFUNCTION(BlueprintCallable, Category = "Director")
	void AddAlertFromTowerRepair(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Director")
	void AddAlertFromTransmissionPulse(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Director")
	void AddAlertFromPlayerNoise(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Director")
	void AddAlertFromCultSpotting(float Amount);

	UFUNCTION(BlueprintCallable, Category="Director")
	float GetAlertLevel() const { return AlertLevel; }

	UFUNCTION(BlueprintPure, Category="Director")
	float GetNormalizedIntensity() const { return AlertLevel / 100.0f; }

	UFUNCTION(BlueprintCallable, Category="Director")
	bool CanUseTower() const { return AlertLevel >= MinAlertForTower; }

	UFUNCTION(BlueprintCallable, Category="Director")
	bool CanTransmit() const { return AlertLevel >= MinAlertForTransmit; }

	UFUNCTION(BlueprintPure, Category = "Director")
	float GetCurrentSpawnInterval() const;

	UFUNCTION(BlueprintPure, Category = "Director")
	float GetCurrentSearchDuration() const;

	UFUNCTION(BlueprintPure, Category = "Director")
	int32 GetDesiredActiveCultists() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	float PassiveSpawnInterval = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	float AlertedSpawnInterval = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	float HostileSpawnInterval = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	float OverwhelmedSpawnInterval = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Search")
	float PassiveSearchDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Search")
	float AlertedSearchDuration = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Search")
	float HostileSearchDuration = 11.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Search")
	float OverwhelmedSearchDuration = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	int32 AlertedCultistCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	int32 HostileCultistCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Spawning")
	int32 OverwhelmedCultistCount = 6;

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
