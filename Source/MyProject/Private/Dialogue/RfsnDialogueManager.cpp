// RFSN Dialogue Manager Implementation

#include "RfsnDialogueManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "IslandHUD.h"
#include "Kismet/GameplayStatics.h"
#include "RfsnNpcClientComponent.h"

void URfsnDialogueManager::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  UE_LOG(LogTemp, Log, TEXT("[RfsnDialogueManager] Initialized"));
}

void URfsnDialogueManager::Deinitialize() {
  EndDialogue();
  Super::Deinitialize();
}

bool URfsnDialogueManager::StartDialogue(AActor *NpcActor) {
  if (!NpcActor) {
    return false;
  }

  // Already in dialogue with this NPC?
  if (ActiveNpc == NpcActor) {
    return true;
  }

  // End any existing dialogue
  if (ActiveNpc) {
    EndDialogue();
  }

  // Find RFSN client on NPC
  URfsnNpcClientComponent *Client =
      NpcActor->FindComponentByClass<URfsnNpcClientComponent>();
  if (!Client) {
    UE_LOG(LogTemp, Warning,
           TEXT("[RfsnDialogueManager] NPC has no RfsnNpcClientComponent"));
    return false;
  }

  // Store references
  ActiveNpc = NpcActor;
  ActiveClient = Client;

  // Bind to client events
  Client->OnSentenceReceived.AddDynamic(
      this, &URfsnDialogueManager::OnSentenceReceived);
  Client->OnDialogueComplete.AddDynamic(
      this, &URfsnDialogueManager::OnDialogueComplete);
  Client->OnNpcActionReceived.AddDynamic(this,
                                         &URfsnDialogueManager::OnNpcAction);

  // Bind HUD to client for subtitle display
  UWorld *World = GetWorld();
  if (World) {
    APlayerController *PC = World->GetFirstPlayerController();
    if (PC) {
      AIslandHUD *HUD = Cast<AIslandHUD>(PC->GetHUD());
      if (HUD) {
        HUD->BindToRfsnClient(Client);
      }
    }
  }

  OnDialogueStarted.Broadcast(NpcActor);
  UE_LOG(LogTemp, Log, TEXT("[RfsnDialogueManager] Started dialogue with %s"),
         *NpcActor->GetName());

  return true;
}

void URfsnDialogueManager::EndDialogue() {
  if (ActiveClient) {
    ActiveClient->OnSentenceReceived.RemoveDynamic(
        this, &URfsnDialogueManager::OnSentenceReceived);
    ActiveClient->OnDialogueComplete.RemoveDynamic(
        this, &URfsnDialogueManager::OnDialogueComplete);
    ActiveClient->OnNpcActionReceived.RemoveDynamic(
        this, &URfsnDialogueManager::OnNpcAction);
  }

  // Clear HUD dialogue
  UWorld *World = GetWorld();
  if (World) {
    APlayerController *PC = World->GetFirstPlayerController();
    if (PC) {
      AIslandHUD *HUD = Cast<AIslandHUD>(PC->GetHUD());
      if (HUD) {
        HUD->ClearNpcDialogue();
      }
    }
  }

  ActiveNpc = nullptr;
  ActiveClient = nullptr;

  OnDialogueEnded.Broadcast();
  UE_LOG(LogTemp, Log, TEXT("[RfsnDialogueManager] Dialogue ended"));
}

void URfsnDialogueManager::SendPlayerMessage(const FString &Message) {
  if (ActiveClient && !Message.IsEmpty()) {
    ActiveClient->SendPlayerUtterance(Message);
  }
}

AActor *URfsnDialogueManager::FindNearestRfsnNpc(const FVector &Location,
                                                 float MaxDistance) const {
  UWorld *World = GetWorld();
  if (!World) {
    return nullptr;
  }

  AActor *NearestNpc = nullptr;
  float NearestDistSq = MaxDistance * MaxDistance;

  for (TActorIterator<AActor> It(World); It; ++It) {
    AActor *Actor = *It;
    if (!Actor || Actor == ActiveNpc) {
      continue;
    }

    // Check if has RFSN client
    if (!Actor->FindComponentByClass<URfsnNpcClientComponent>()) {
      continue;
    }

    float DistSq = FVector::DistSquared(Location, Actor->GetActorLocation());
    if (DistSq < NearestDistSq) {
      NearestDistSq = DistSq;
      NearestNpc = Actor;
    }
  }

  return NearestNpc;
}

void URfsnDialogueManager::OnSentenceReceived(const FRfsnSentence &Sentence) {
  // HUD binding handles display automatically
  UE_LOG(LogTemp, Verbose, TEXT("[RfsnDialogueManager] Sentence: %s"),
         *Sentence.Sentence);
}

void URfsnDialogueManager::OnDialogueComplete() {
  UE_LOG(LogTemp, Log, TEXT("[RfsnDialogueManager] NPC finished speaking"));
}

void URfsnDialogueManager::OnNpcAction(ERfsnNpcAction Action) {
  // Could trigger game events based on NPC action
  if (Action == ERfsnNpcAction::Attack || Action == ERfsnNpcAction::Flee) {
    // End dialogue if NPC becomes hostile or flees
    EndDialogue();
  }
}
