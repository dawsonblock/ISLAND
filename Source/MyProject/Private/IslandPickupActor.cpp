#include "IslandPickupActor.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AIslandPickupActor::AIslandPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Root);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PickupPrompt = FText::FromString(TEXT("Pick Up"));
}

bool AIslandPickupActor::CanInteract_Implementation(const FIslandInteractContext& Ctx) const
{
	if (const AActor* Interactor = Ctx.Interactor.Get())
	{
		return Interactor->FindComponentByClass<UIslandInventoryComponent>() != nullptr;
	}

	return false;
}

void AIslandPickupActor::Interact_Implementation(const FIslandInteractContext& Ctx)
{
	AActor* Interactor = Ctx.Interactor.Get();
	if (!Interactor || ItemType == EIslandItemType::None || Quantity <= 0)
	{
		return;
	}

	if (UIslandInventoryComponent* Inventory = Interactor->FindComponentByClass<UIslandInventoryComponent>())
	{
		Inventory->AddItem(ItemType, Quantity);
		if (PickupSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}

		Destroy();
	}
}

FText AIslandPickupActor::GetInteractPrompt_Implementation(const FIslandInteractContext& Ctx) const
{
	return PickupPrompt.IsEmpty() ? FText::FromString(TEXT("Pick Up")) : PickupPrompt;
}
