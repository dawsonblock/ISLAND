#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IslandInteractableInterface.generated.h"

USTRUCT(BlueprintType)
struct FIslandInteractContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Interactor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FVector HitLocation = FVector::ZeroVector;
};

UINTERFACE(BlueprintType)
class MYPROJECT_API UIslandInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class MYPROJECT_API IIslandInteractableInterface
{
	GENERATED_BODY()

public:
	// Return true if currently interactable (e.g., tower not powered yet, has fuel, etc.)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	bool CanInteract(const FIslandInteractContext& Ctx) const;

	// Perform the interaction
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	void Interact(const FIslandInteractContext& Ctx);

	// UI prompt text
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interact")
	FText GetInteractPrompt(const FIslandInteractContext& Ctx) const;
};
