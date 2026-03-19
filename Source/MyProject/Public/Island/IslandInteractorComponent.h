#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IslandInteractableInterface.h"
#include "IslandInteractorComponent.generated.h"

UCLASS(ClassGroup=(Island), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UIslandInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIslandInteractorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	float MaxUseDistance = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Last focused interactable (for UI)
	UPROPERTY(BlueprintReadOnly, Category="Interact")
	TObjectPtr<AActor> FocusedActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Interact")
	FText FocusedPrompt;

	// Call when player presses Interact
	UFUNCTION(BlueprintCallable, Category="Interact")
	bool TryInteract();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool UpdateFocus(FIslandInteractContext& OutCtx, FHitResult& OutHit);
};
