#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandInteractableInterface.h"
#include "IslandInventoryComponent.h"
#include "IslandPickupActor.generated.h"

class USoundBase;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class MYPROJECT_API AIslandPickupActor : public AActor, public IIslandInteractableInterface
{
	GENERATED_BODY()

public:
	AIslandPickupActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EIslandItemType ItemType = EIslandItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	FText PickupPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	TObjectPtr<USoundBase> PickupSound;

	virtual bool CanInteract_Implementation(const FIslandInteractContext& Ctx) const override;
	virtual void Interact_Implementation(const FIslandInteractContext& Ctx) override;
	virtual FText GetInteractPrompt_Implementation(const FIslandInteractContext& Ctx) const override;
};
