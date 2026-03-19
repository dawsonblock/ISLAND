class AActor;
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IslandNoiseLibrary.generated.h"

UCLASS()
class MYPROJECT_API UIslandNoiseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Island|Noise", meta = (WorldContext = "WorldContextObject"))
	static void EmitNoise(UObject* WorldContextObject, FVector Location, float Loudness, AActor* Source);
};
