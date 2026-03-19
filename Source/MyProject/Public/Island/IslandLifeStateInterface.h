#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IslandLifeStateInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UIslandLifeStateInterface : public UInterface
{
	GENERATED_BODY()
};

class IIslandLifeStateInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Life State")
	bool IsDowned() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Life State")
	bool IsDead() const;
};
