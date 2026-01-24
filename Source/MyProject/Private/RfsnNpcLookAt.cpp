// RFSN NPC Look-At Implementation

#include "RfsnNpcLookAt.h"
#include "RfsnDialogueManager.h"
#include "RfsnLogging.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

URfsnNpcLookAt::URfsnNpcLookAt()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f; // ~60fps
}

void URfsnNpcLookAt::BeginPlay()
{
	Super::BeginPlay();
}

void URfsnNpcLookAt::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled)
	{
		return;
	}

	// Check if we should be looking during dialogue
	if (bOnlyDuringDialogue)
	{
		UWorld* World = GetWorld();
		URfsnDialogueManager* DialogueMgr = World ? World->GetSubsystem<URfsnDialogueManager>() : nullptr;

		if (DialogueMgr && DialogueMgr->IsDialogueActive())
		{
			AActor* ActiveNpc = DialogueMgr->GetActiveNpc();
			if (ActiveNpc == GetOwner())
			{
				// We are the active dialogue NPC - look at player
				LookAtLocation(GetPlayerEyeLocation());
				bIsLookingAtPlayer = true;
			}
		}
		else
		{
			if (bIsLookingAtPlayer)
			{
				StopLooking();
			}
		}
	}

	if (bHasTarget)
	{
		UpdateLookAt(DeltaTime);
	}
}

void URfsnNpcLookAt::LookAtActor(AActor* Target)
{
	if (!Target)
	{
		StopLooking();
		return;
	}

	CurrentTarget = Target;
	bHasTarget = true;
	LookAtTarget = Target->GetActorLocation();
}

void URfsnNpcLookAt::LookAtLocation(FVector Location)
{
	CurrentTarget.Reset();
	LookAtTarget = Location;
	bHasTarget = true;
}

void URfsnNpcLookAt::StopLooking()
{
	bHasTarget = false;
	bIsLookingAtPlayer = false;
	CurrentTarget.Reset();
}

float URfsnNpcLookAt::GetAngleToTarget() const
{
	if (!bHasTarget)
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	FVector ToTarget = LookAtTarget - Owner->GetActorLocation();
	ToTarget.Z = 0; // Ignore vertical
	ToTarget.Normalize();

	FVector Forward = Owner->GetActorForwardVector();
	Forward.Z = 0;
	Forward.Normalize();

	float Dot = FVector::DotProduct(Forward, ToTarget);
	return FMath::RadiansToDegrees(FMath::Acos(Dot));
}

void URfsnNpcLookAt::UpdateLookAt(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Update target location if tracking an actor
	if (CurrentTarget.IsValid())
	{
		LookAtTarget = CurrentTarget->GetActorLocation();
		LookAtTarget.Z += EyeHeightOffset; // Eye level
	}

	// Calculate target rotation
	FVector OwnerLocation = Owner->GetActorLocation();
	TargetRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, LookAtTarget);

	// Only rotate yaw for body
	FRotator CurrentRotation = Owner->GetActorRotation();
	float AngleToTarget = GetAngleToTarget();

	if (bRotateBody && AngleToTarget > HeadOnlyAngle)
	{
		// Rotate body
		FRotator NewRotation = FMath::RInterpTo(
		    CurrentRotation, FRotator(CurrentRotation.Pitch, TargetRotation.Yaw, CurrentRotation.Roll), DeltaTime,
		    RotationSpeed / 90.0f // Convert to interp speed
		);
		Owner->SetActorRotation(NewRotation);
	}

	// Head bone rotation would be handled via animation blueprint
	// This component provides the target - animation BP reads it
}

FVector URfsnNpcLookAt::GetPlayerEyeLocation() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return FVector::ZeroVector;
	}

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		return FVector::ZeroVector;
	}

	FVector Location = PlayerPawn->GetActorLocation();
	Location.Z += EyeHeightOffset;
	return Location;
}
