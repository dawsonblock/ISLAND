// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyProjectGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(config = Game, Blueprintable)
class MYPROJECT_API AMyProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyProjectGameMode();
};
