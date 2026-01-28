// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MyProjectCameraManager.generated.h"

/**
 *  Basic First Person camera manager.
 *  Limits min/max look pitch.
 */
UCLASS(Blueprintable)
class MYPROJECT_API AMyProjectCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	/** Constructor */
	AMyProjectCameraManager();
};
