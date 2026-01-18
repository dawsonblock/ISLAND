#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IslandRadioTower.h"
#include "IslandGameMode.generated.h"

class AIslandExtractionZone;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	bool bShowWelcomeTutorial = true;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void OnTowerStateChanged(ERadioTowerState NewState);

	void TryAutoFindActors();
};
