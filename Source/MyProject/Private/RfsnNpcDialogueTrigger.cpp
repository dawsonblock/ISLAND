// RFSN NPC Dialogue Trigger Implementation

#include "RfsnNpcDialogueTrigger.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RfsnDialogueWidget.h"
#include "RfsnNpcClientComponent.h"

URfsnNpcDialogueTrigger::URfsnNpcDialogueTrigger() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickInterval = 0.1f; // Check proximity 10x per second
}

void URfsnNpcDialogueTrigger::BeginPlay() {
  Super::BeginPlay();
  FindComponents();

  // Get player pawn reference
  if (APlayerController *PC =
          UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
    PlayerPawn = PC->GetPawn();
  }
}

void URfsnNpcDialogueTrigger::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (TriggerMode == ERfsnDialogueTriggerMode::Proximity ||
      TriggerMode == ERfsnDialogueTriggerMode::ProximityOnce) {
    CheckProximity();
  }
}

void URfsnNpcDialogueTrigger::FindComponents() {
  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  // Find RFSN client if not set
  if (!RfsnClient) {
    RfsnClient = Owner->FindComponentByClass<URfsnNpcClientComponent>();
  }

  // Find or create dialogue widget if not set
  if (!DialogueWidget) {
    DialogueWidget = Owner->FindComponentByClass<URfsnDialogueWidget>();
    if (!DialogueWidget) {
      // Create dialogue widget
      DialogueWidget =
          NewObject<URfsnDialogueWidget>(Owner, TEXT("RfsnDialogueWidget"));
      if (DialogueWidget) {
        DialogueWidget->RegisterComponent();
      }
    }
  }

  // Bind dialogue widget to RFSN client
  if (DialogueWidget && RfsnClient) {
    DialogueWidget->BindToRfsnClient(RfsnClient);
  }
}

void URfsnNpcDialogueTrigger::CheckProximity() {
  if (!PlayerPawn) {
    // Try to get player pawn again
    if (APlayerController *PC =
            UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
      PlayerPawn = PC->GetPawn();
    }
    if (!PlayerPawn)
      return;
  }

  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  float DistSq = FVector::DistSquared(Owner->GetActorLocation(),
                                      PlayerPawn->GetActorLocation());
  float RadiusSq = ProximityRadius * ProximityRadius;

  bool bWasInProximity = bPlayerInProximity;
  bPlayerInProximity = (DistSq <= RadiusSq);

  // Check for proximity trigger
  if (bPlayerInProximity && !bWasInProximity) {
    if (TriggerMode == ERfsnDialogueTriggerMode::Proximity) {
      TriggerDefaultDialogue();
    } else if (TriggerMode == ERfsnDialogueTriggerMode::ProximityOnce &&
               !bProximityTriggered) {
      bProximityTriggered = true;
      TriggerDefaultDialogue();
    }
  }
}

bool URfsnNpcDialogueTrigger::CanTrigger() const {
  if (!RfsnClient)
    return false;
  if (RfsnClient->IsDialogueActive())
    return false;

  UWorld *World = GetWorld();
  if (!World)
    return false;

  float CurrentTime = World->GetTimeSeconds();
  return (CurrentTime - LastTriggerTime) >= TriggerCooldown;
}

bool URfsnNpcDialogueTrigger::IsOnCooldown() const {
  UWorld *World = GetWorld();
  if (!World)
    return false;

  float CurrentTime = World->GetTimeSeconds();
  return (CurrentTime - LastTriggerTime) < TriggerCooldown;
}

bool URfsnNpcDialogueTrigger::IsPlayerInProximity() const {
  return bPlayerInProximity;
}

void URfsnNpcDialogueTrigger::OnPlayerInteract() {
  if (TriggerMode == ERfsnDialogueTriggerMode::Interact && bPlayerInProximity) {
    TriggerDefaultDialogue();
  }
}

void URfsnNpcDialogueTrigger::TriggerDefaultDialogue() {
  TriggerDialogue(DefaultPrompt);
}

void URfsnNpcDialogueTrigger::TriggerDialogue(const FString &PlayerPrompt) {
  if (!CanTrigger()) {
    UE_LOG(LogTemp, Verbose,
           TEXT("[RfsnTrigger] Cannot trigger dialogue - on cooldown or "
                "already active"));
    return;
  }

  if (RfsnClient) {
    UWorld *World = GetWorld();
    if (World) {
      LastTriggerTime = World->GetTimeSeconds();
    }

    RfsnClient->SendPlayerUtterance(PlayerPrompt);
    OnTriggerActivated.Broadcast(PlayerPrompt);

    UE_LOG(LogTemp, Log, TEXT("[RfsnTrigger] Triggered dialogue: %s"),
           *PlayerPrompt);
  } else {
    UE_LOG(LogTemp, Warning, TEXT("[RfsnTrigger] No RfsnClient found"));
  }
}
