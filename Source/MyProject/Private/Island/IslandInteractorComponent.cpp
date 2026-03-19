#include "IslandInteractorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"

UIslandInteractorComponent::UIslandInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIslandInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UIslandInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FIslandInteractContext Ctx;
	FHitResult Hit;
	if (UpdateFocus(Ctx, Hit))
	{
		FocusedActor = Hit.GetActor();
		if (FocusedActor && FocusedActor->GetClass()->ImplementsInterface(UIslandInteractableInterface::StaticClass()))
		{
			const bool bCan = IIslandInteractableInterface::Execute_CanInteract(FocusedActor, Ctx);
			FocusedPrompt = bCan ? IIslandInteractableInterface::Execute_GetInteractPrompt(FocusedActor, Ctx) : FText();
		}
		else
		{
			FocusedPrompt = FText();
		}
	}
	else
	{
		FocusedActor = nullptr;
		FocusedPrompt = FText();
	}
}

bool UIslandInteractorComponent::UpdateFocus(FIslandInteractContext& OutCtx, FHitResult& OutHit)
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (!PC)
	{
		// If owner isn't using instigator controller, attempt to get controller from pawn
		if (APawn* P = Cast<APawn>(Owner))
		{
			PC = Cast<APlayerController>(P->GetController());
		}
	}
	if (!PC) return false;

	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CamDir = PC->PlayerCameraManager->GetActorForwardVector();
	const FVector End = CamLoc + CamDir * MaxUseDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandInteractTrace), false);
	Params.AddIgnoredActor(Owner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, CamLoc, End, TraceChannel, Params);
	if (!bHit || !OutHit.GetActor()) return false;

	AActor* HitActor = OutHit.GetActor();
	if (!HitActor->GetClass()->ImplementsInterface(UIslandInteractableInterface::StaticClass()))
		return false;

	OutCtx.Interactor = Owner;
	OutCtx.HitLocation = OutHit.ImpactPoint;
	return true;
}

bool UIslandInteractorComponent::TryInteract()
{
	if (!FocusedActor) return false;

	FIslandInteractContext Ctx;
	Ctx.Interactor = GetOwner();
	Ctx.HitLocation = FocusedActor->GetActorLocation();

	if (!FocusedActor->GetClass()->ImplementsInterface(UIslandInteractableInterface::StaticClass()))
		return false;

	if (!IIslandInteractableInterface::Execute_CanInteract(FocusedActor, Ctx))
		return false;

	IIslandInteractableInterface::Execute_Interact(FocusedActor, Ctx);
	return true;
}
