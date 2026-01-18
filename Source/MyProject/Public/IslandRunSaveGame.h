#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "IslandRunSaveGame.generated.h"

UCLASS()
class MYPROJECT_API UIslandRunSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category="Run")
	int32 TotalRuns = 0;

	UPROPERTY(VisibleAnywhere, Category="Run")
	int32 Escapes = 0;

	UPROPERTY(VisibleAnywhere, Category="Run")
	float BestEscapeTimeSeconds = 999999.0f;

	UPROPERTY(VisibleAnywhere, Category="Run")
	int32 LastRunSeed = 0;

	UPROPERTY(VisibleAnywhere, Category="Run")
	TArray<FName> UnlockedClues;
};
