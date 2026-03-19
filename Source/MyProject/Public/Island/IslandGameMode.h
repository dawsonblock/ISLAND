#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Island/IslandRadioTower.h"
#include "IslandGameMode.generated.h"

class AIslandExtractionZone;
class AIslandAISpawnManager;
class UIslandHUDWidget;

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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void OnTowerStateChanged(ERadioTowerState NewState);

	void TryAutoFindActors();
	void UpdateObjectiveText();
	void HandlePlayerDeath();
	void HandleTowerPhaseChanged(ERadioTowerState NewState);

	bool bHandledInvalidRun = false;
};
