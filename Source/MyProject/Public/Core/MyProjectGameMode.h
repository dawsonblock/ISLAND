// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyProjectGameMode.generated.h"

/**
 * Legacy migration stub retained for template redirect compatibility.
 * The live gameplay authority is IslandGameMode, not this class.
 */
UCLASS(config = Game, Blueprintable)
class MYPROJECT_API AMyProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyProjectGameMode();
};
