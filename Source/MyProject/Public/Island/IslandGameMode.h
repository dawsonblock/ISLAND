#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Island/IslandGameInstanceSubsystem.h"
#include "Island/IslandRadioTower.h"
#include "IslandGameMode.generated.h"

class AIslandExtractionZone;
class AIslandAISpawnManager;
class UIslandHUDWidget;
class UIslandObjectiveSubsystem;

UCLASS()
class MYPROJECT_API AIslandGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIslandGameMode();

	// Assign these in the level (details panel) or find them dynamically.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island")
	TObjectPtr<AIslandRadioTower> Tower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island")
	TObjectPtr<AIslandExtractionZone> Extraction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island")
	TObjectPtr<AIslandAISpawnManager> SpawnManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island|UI")
	TSubclassOf<UIslandHUDWidget> IslandHUDWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	bool bShowWelcomeTutorial = true;

	void HandlePlayerDeath(EIslandRunEndReason Reason = EIslandRunEndReason::KilledByCult);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleTowerStateChanged(ERadioTowerState NewState);

	UFUNCTION()
	void HandleExtractionSuccess();

	void TryAutoFindActors();
	void ValidateRunRequirements();
	void BindRuntimeActors();
	void HandleRunInvalid();
	void StartRun();
	void UpdateObjectiveText();
	void HandleTowerPhaseChanged(ERadioTowerState NewState);

	UPROPERTY(Transient)
	TObjectPtr<UIslandObjectiveSubsystem> ObjectiveSubsystem;

	bool bRuntimeValid = false;
	bool bRuntimeActorsBound = false;
	bool bRunStarted = false;
	bool bRunEnded = false;
	bool bHandledInvalidRun = false;
	bool bHasRetriedRuntimeDiscovery = false;
	FString RuntimeValidationError;
};
