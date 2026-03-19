// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyProjectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Island/IslandDirectorSubsystem.h"
#include "Island/IslandGameMode.h"
#include "Island/IslandGameInstanceSubsystem.h"
#include "Island/IslandInteractorComponent.h"
#include "Island/IslandInventoryComponent.h"
#include "Island/IslandNoiseLibrary.h"
#include "Island/IslandStealthComponent.h"
#include "Island/IslandVitalityComponent.h"
#include "MyProject.h"
#include "Dialogue/RfsnDialogueManager.h"

AMyProjectCharacter::AMyProjectCharacter() {
  PrimaryActorTick.bCanEverTick = true;

  // Set size for collision capsule
  GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

  // Create the first person mesh that will be viewed only by this character's
  // owner
  FirstPersonMesh =
      CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

  FirstPersonMesh->SetupAttachment(GetMesh());
  FirstPersonMesh->SetOnlyOwnerSee(true);
  FirstPersonMesh->FirstPersonPrimitiveType =
      EFirstPersonPrimitiveType::FirstPerson;
  FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

  // Create the Camera Component
  FirstPersonCameraComponent =
      CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
  FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
  FirstPersonCameraComponent->SetRelativeLocationAndRotation(
      FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
  FirstPersonCameraComponent->bUsePawnControlRotation = true;
  FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
  FirstPersonCameraComponent->bEnableFirstPersonScale = true;
  FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
  FirstPersonCameraComponent->FirstPersonScale = 0.6f;

  // configure the character comps
  GetMesh()->SetOwnerNoSee(true);
  GetMesh()->FirstPersonPrimitiveType =
      EFirstPersonPrimitiveType::WorldSpaceRepresentation;

  GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

  GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
  GetCharacterMovement()->AirControl = 0.5f;

  VitalityComponent = CreateDefaultSubobject<UIslandVitalityComponent>(
      TEXT("VitalityComponent"));
  InteractorComponent = CreateDefaultSubobject<UIslandInteractorComponent>(
      TEXT("InteractorComponent"));
  InventoryComponent = CreateDefaultSubobject<UIslandInventoryComponent>(
      TEXT("InventoryComponent"));
  StealthComponent = CreateDefaultSubobject<UIslandStealthComponent>(
      TEXT("StealthComponent"));

  GetCharacterMovement()->MaxWalkSpeed = 600.0f;
}

void AMyProjectCharacter::BeginPlay() {
  Super::BeginPlay();

  if (VitalityComponent) {
    VitalityComponent->OnDeath.AddDynamic(this, &AMyProjectCharacter::HandleDeath);
  }
}

void AMyProjectCharacter::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  if (bDead) {
    return;
  }

  // Handle stamina drain during sprint
  if (bIsSprinting && GetVelocity().SizeSquared2D() > 0.0f) {
    VitalityComponent->ModifyStamina(-20.0f * DeltaTime);

    if (VitalityComponent->GetStaminaNormalized() <= 0.0f) {
      DoSprintEnd();
    }
  }

  if (StealthComponent) {
    StealthComponent->UpdateStealth(DeltaTime, GetVelocity(), bIsCrouched,
                                    bIsSprinting);
  }

  EmitMovementNoise(DeltaTime);
}

bool AMyProjectCharacter::IsDowned_Implementation() const { return bDowned; }

bool AMyProjectCharacter::IsDead_Implementation() const { return bDead; }

void AMyProjectCharacter::HandleDeath(bool bIsFatal) {
  if (bDead) {
    return;
  }

  bDowned = false;
  bDead = true;
  bIsSprinting = false;
  GetCharacterMovement()->DisableMovement();
  GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  if (APlayerController *PC = Cast<APlayerController>(GetController())) {
    DisableInput(PC);
  }

  const EIslandRunEndReason Reason =
      VitalityComponent && VitalityComponent->GetHungerNormalized() <= 0.0f
          ? EIslandRunEndReason::Starved
          : EIslandRunEndReason::KilledByCult;

  if (AIslandGameMode *IslandGameMode =
          GetWorld() ? Cast<AIslandGameMode>(GetWorld()->GetAuthGameMode())
                     : nullptr) {
    IslandGameMode->HandlePlayerDeath(Reason);
    return;
  }

  if (UGameInstance *GI = GetGameInstance()) {
    if (UIslandGameInstanceSubsystem *Run =
            GI->GetSubsystem<UIslandGameInstanceSubsystem>()) {
      Run->EndRun(false, Reason);
    }
  }
}

void AMyProjectCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  // Set up action bindings
  if (UEnhancedInputComponent *EnhancedInputComponent =
          Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
    // Jumping
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this,
                                       &AMyProjectCharacter::DoJumpStart);
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed,
                                       this, &AMyProjectCharacter::DoJumpEnd);

    // Moving
    EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered,
                                       this, &AMyProjectCharacter::MoveInput);

    // Looking/Aiming
    EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered,
                                       this, &AMyProjectCharacter::LookInput);
    EnhancedInputComponent->BindAction(MouseLookAction,
                                       ETriggerEvent::Triggered, this,
                                       &AMyProjectCharacter::LookInput);

    // Sprinting
    EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started,
                                       this,
                                       &AMyProjectCharacter::DoSprintStart);
    EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed,
                                       this, &AMyProjectCharacter::DoSprintEnd);

    if (InteractAction) {
      EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started,
                                         this,
                                         &AMyProjectCharacter::TryInteract);
    }

    if (FlashlightAction) {
      EnhancedInputComponent->BindAction(
          FlashlightAction, ETriggerEvent::Started, this,
          &AMyProjectCharacter::ToggleFlashlight);
    }

    // Dialogue
    if (DialogueAction) {
      EnhancedInputComponent->BindAction(
          DialogueAction, ETriggerEvent::Started, this,
          &AMyProjectCharacter::TryStartDialogue);
    }
  } else {
    UE_LOG(
        LogMyProject, Error,
        TEXT("'%s' Failed to find an Enhanced Input Component! This template "
             "is built to use the Enhanced Input system. If you intend to use "
             "the legacy system, then you will need to update this C++ file."),
        *GetNameSafe(this));
  }
}

void AMyProjectCharacter::MoveInput(const FInputActionValue &Value) {
  // get the Vector2D move axis
  FVector2D MovementVector = Value.Get<FVector2D>();

  // pass the axis values to the move input
  DoMove(MovementVector.X, MovementVector.Y);
}

void AMyProjectCharacter::LookInput(const FInputActionValue &Value) {
  // get the Vector2D look axis
  FVector2D LookAxisVector = Value.Get<FVector2D>();

  // pass the axis values to the aim input
  DoAim(LookAxisVector.X, LookAxisVector.Y);
}

void AMyProjectCharacter::DoAim(float Yaw, float Pitch) {
  if (!bDead && GetController()) {
    // pass the rotation inputs
    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
  }
}

void AMyProjectCharacter::DoMove(float Right, float Forward) {
  if (!bDead && GetController()) {
    // pass the move inputs
    AddMovementInput(GetActorRightVector(), Right);
    AddMovementInput(GetActorForwardVector(), Forward);
  }
}

void AMyProjectCharacter::DoJumpStart() {
  if (bDead) {
    return;
  }
  // pass Jump to the character
  Jump();
}

void AMyProjectCharacter::DoJumpEnd() {
  // pass StopJumping to the character
  StopJumping();
}
void AMyProjectCharacter::DoSprintStart() {
  if (!bDead && VitalityComponent &&
      VitalityComponent->GetStaminaNormalized() > 0.1f) {
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = 1000.0f;
  }
}

void AMyProjectCharacter::DoSprintEnd() {
  bIsSprinting = false;
  GetCharacterMovement()->MaxWalkSpeed = 600.0f;
}

void AMyProjectCharacter::TryInteract() {
  if (InteractorComponent) {
    InteractorComponent->TryInteract();
  }
}

void AMyProjectCharacter::ToggleFlashlight() {
  if (StealthComponent) {
    StealthComponent->SetFlashlightEnabled(!StealthComponent->bFlashlightOn);
  }
}

void AMyProjectCharacter::EmitMovementNoise(float DeltaTime) {
  if (!StealthComponent) {
    return;
  }

  MovementNoiseCooldown = FMath::Max(0.0f, MovementNoiseCooldown - DeltaTime);

  const float Loudness = GetCurrentNoiseLoudness();
  if (Loudness <= 0.0f || GetVelocity().SizeSquared2D() <= 25.0f) {
    return;
  }

  if (UWorld *World = GetWorld()) {
    if (UIslandDirectorSubsystem *Director =
            World->GetSubsystem<UIslandDirectorSubsystem>()) {
      Director->AddAlertFromPlayerNoise(Loudness * DeltaTime * 4.0f);
    }
  }

  if (MovementNoiseCooldown > 0.0f) {
    return;
  }

  MovementNoiseCooldown = bIsSprinting ? 0.18f : (bIsCrouched ? 0.45f : 0.3f);
  UIslandNoiseLibrary::EmitNoise(this, GetActorLocation(), Loudness, this);
}

float AMyProjectCharacter::GetCurrentNoiseLoudness() const {
  return StealthComponent ? StealthComponent->GetNoiseLoudness() : 0.0f;
}

void AMyProjectCharacter::TryStartDialogue() {
  if (bDead) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  URfsnDialogueManager *DialogueManager =
      World->GetSubsystem<URfsnDialogueManager>();
  if (!DialogueManager) {
    return;
  }

  // Find nearest RFSN NPC
  AActor *NearestNpc =
      DialogueManager->FindNearestRfsnNpc(GetActorLocation(), 300.0f);
  if (NearestNpc) {
    DialogueManager->StartDialogue(NearestNpc);
    // Send a default greeting
    DialogueManager->SendPlayerMessage(TEXT("Hello."));
  } else {
    UE_LOG(LogTemp, Warning, TEXT("No RFSN NPC in range"));
  }
}
