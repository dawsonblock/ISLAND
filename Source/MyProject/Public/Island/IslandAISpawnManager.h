#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Island/IslandDirectorSubsystem.h"
#include "Island/IslandRadioTower.h"
#include "IslandAISpawnManager.generated.h"

UCLASS()
class MYPROJECT_API AIslandAISpawnManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AIslandAISpawnManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	TSubclassOf<class ACultistCharacter> CultistClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float SpawnRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float MinSpawnDistance = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxAliveCultists = 6;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 AliveCultistCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnCultistNearTower();

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnCultistNearLastKnownPlayerLocation(const FVector& Location);

	UFUNCTION()
	void RegisterCultistDeath(class ACultistCharacter* Cultist);

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
	void TrySpawnCultist();
	int32 GetDesiredPressureSpawnCount() const;
	bool ShouldSpawnForTowerPressure() const;
	bool ShouldSpawnForDirectorIntensity() const;
	bool SpawnCultistAroundLocation(const FVector& Origin, FVector* ForcedInvestigationLocation = nullptr,
	                                bool bGuardTower = false);
	
	EIslandIntensityState CurrentIntensity = EIslandIntensityState::Passive;
};
