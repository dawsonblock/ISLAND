#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandRadioTower.h"
#include "IslandDirectorSubsystem.h"
#include "IslandAISpawnManager.generated.h"

UCLASS()
class MYPROJECT_API AIslandAISpawnManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AIslandAISpawnManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	TSubclassOf<APawn> HunterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float SpawnRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float MinSpawnDistance = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config|Intensity")
	float PassiveInterval = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config|Intensity")
	float AlertedInterval = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config|Intensity")
	float HostileInterval = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config|Intensity")
	float OverwhelmedInterval = 2.0f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTowerStateChanged(ERadioTowerState NewState);

	UFUNCTION()
	void OnIntensityChanged(EIslandIntensityState NewState);

	UPROPERTY()
	AIslandRadioTower* CachedTower;

	FTimerHandle SpawnTimer;
	void StartSpawning();
	void StopSpawning();
	void TrySpawnHunter();
	float GetCurrentInterval() const;
	
	EIslandIntensityState CurrentIntensity = EIslandIntensityState::Passive;
};

	FTimerHandle SpawnTimer;
	TObjectPtr<AIslandRadioTower> CachedTower;
};
