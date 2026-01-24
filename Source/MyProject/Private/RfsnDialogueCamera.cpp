// RFSN Dialogue Camera Implementation

#include "RfsnDialogueCamera.h"
#include "RfsnDialogueManager.h"
#include "RfsnLogging.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

URfsnDialogueCamera::URfsnDialogueCamera()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f;
}

void URfsnDialogueCamera::BeginPlay()
{
	Super::BeginPlay();
}

void URfsnDialogueCamera::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled)
	{
		return;
	}

	// Auto-manage camera based on dialogue state
	UWorld* World = GetWorld();
	URfsnDialogueManager* DialogueMgr = World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;

	if (DialogueMgr)
	{
		if (DialogueMgr->IsDialogueActive() && !bDialogueCameraActive)
		{
			AActor* Npc = DialogueMgr->GetActiveNpc();
			if (Npc)
			{
				StartDialogueCamera(Npc, DefaultMode);
			}
		}
		else if (!DialogueMgr->IsDialogueActive() && bDialogueCameraActive)
		{
			StopDialogueCamera();
		}
	}

	if (bDialogueCameraActive)
	{
		UpdateCameraPosition();
	}
}

void URfsnDialogueCamera::StartDialogueCamera(AActor* NpcActor, ERfsnCameraMode Mode)
{
	if (!NpcActor || !bEnabled)
	{
		return;
	}

	DialogueNpc = NpcActor;
	CurrentMode = Mode;
	bDialogueCameraActive = true;

	RFSN_DIALOGUE_LOG("Started dialogue camera: Mode=%d", static_cast<int32>(Mode));

	// Note: Actual camera blending would be done via PlayerController::SetViewTarget
	// or Sequencer. This component calculates transforms.
}

void URfsnDialogueCamera::StopDialogueCamera()
{
	bDialogueCameraActive = false;
	CurrentMode = ERfsnCameraMode::None;
	DialogueNpc.Reset();

	RFSN_DIALOGUE_LOG("Stopped dialogue camera");
}

void URfsnDialogueCamera::SetCameraMode(ERfsnCameraMode NewMode)
{
	if (CurrentMode != NewMode)
	{
		CurrentMode = NewMode;
		RFSN_DIALOGUE_LOG("Camera mode changed to: %d", static_cast<int32>(NewMode));
	}
}

FTransform URfsnDialogueCamera::GetDialogueCameraTransform() const
{
	switch (CurrentMode)
	{
	case ERfsnCameraMode::FocusNpc:
		return CalculateFocusTransform();
	case ERfsnCameraMode::OverShoulder:
		return CalculateOverShoulderTransform();
	case ERfsnCameraMode::TwoShot:
		return CalculateTwoShotTransform();
	default:
		return FTransform::Identity;
	}
}

void URfsnDialogueCamera::UpdateCameraPosition()
{
	// This provides the calculated transform
	// Actual camera control is left to Blueprint/Controller
	// to avoid conflicting with existing camera systems
}

FTransform URfsnDialogueCamera::CalculateFocusTransform() const
{
	if (!DialogueNpc.IsValid())
	{
		return FTransform::Identity;
	}

	FVector NpcLocation = DialogueNpc->GetActorLocation();
	NpcLocation.Z += FocusHeightOffset;

	// Get player location
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn())
	{
		return FTransform::Identity;
	}

	FVector PlayerLocation = PC->GetPawn()->GetActorLocation();

	// Position camera between player and NPC, facing NPC
	FVector Direction = (NpcLocation - PlayerLocation).GetSafeNormal();
	FVector CameraLocation = NpcLocation - Direction * FocusDistance;
	CameraLocation.Z = NpcLocation.Z;

	FRotator CameraRotation = UKismetMathLibrary::FindLookAtRotation(CameraLocation, NpcLocation);

	return FTransform(CameraRotation, CameraLocation);
}

FTransform URfsnDialogueCamera::CalculateOverShoulderTransform() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn() || !DialogueNpc.IsValid())
	{
		return FTransform::Identity;
	}

	FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
	FVector NpcLocation = DialogueNpc->GetActorLocation();

	// Calculate direction from player to NPC
	FVector ToNpc = (NpcLocation - PlayerLocation).GetSafeNormal();
	FRotator ToNpcRot = ToNpc.Rotation();

	// Apply shoulder offset in local space
	FVector CameraLocation = PlayerLocation;
	CameraLocation += ToNpcRot.RotateVector(ShoulderOffset);

	// Look at NPC
	FRotator CameraRotation =
	    UKismetMathLibrary::FindLookAtRotation(CameraLocation, NpcLocation + FVector(0, 0, FocusHeightOffset));

	return FTransform(CameraRotation, CameraLocation);
}

FTransform URfsnDialogueCamera::CalculateTwoShotTransform() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn() || !DialogueNpc.IsValid())
	{
		return FTransform::Identity;
	}

	FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
	FVector NpcLocation = DialogueNpc->GetActorLocation();

	// Midpoint between player and NPC
	FVector Midpoint = (PlayerLocation + NpcLocation) * 0.5f;
	Midpoint.Z += FocusHeightOffset;

	// Position camera to the side
	FVector ToNpc = (NpcLocation - PlayerLocation).GetSafeNormal();
	FVector Side = FVector::CrossProduct(ToNpc, FVector::UpVector).GetSafeNormal();

	FVector CameraLocation = Midpoint + Side * FocusDistance * 1.5f;
	CameraLocation.Z = Midpoint.Z;

	FRotator CameraRotation = UKismetMathLibrary::FindLookAtRotation(CameraLocation, Midpoint);

	return FTransform(CameraRotation, CameraLocation);
}
