// RFSN Quest Bridge Implementation

#include "RfsnQuestBridge.h"
#include "Engine/World.h"
#include "IslandObjectiveSubsystem.h"
#include "RfsnNpcClientComponent.h"

URfsnQuestBridge::URfsnQuestBridge() {
  PrimaryComponentTick.bCanEverTick = false;
}

void URfsnQuestBridge::BeginPlay() { Super::BeginPlay(); }

void URfsnQuestBridge::BindToRfsnClient(URfsnNpcClientComponent *Client) {
  if (Client) {
    Client->OnNpcActionReceived.AddDynamic(this,
                                           &URfsnQuestBridge::OnRfsnNpcAction);
  }
}

void URfsnQuestBridge::TriggerQuestProgress(const FString &ObjectiveTag,
                                            float Progress) {
  UIslandObjectiveSubsystem *ObjectiveSys = GetObjectiveSubsystem();
  if (ObjectiveSys) {
    // The objective subsystem would need a method like this
    // ObjectiveSys->AddProgress(ObjectiveTag, Progress);
    UE_LOG(LogTemp, Log, TEXT("[QuestBridge] Progress: %s += %.1f"),
           *ObjectiveTag, Progress);
  }

  OnQuestProgress.Broadcast(ObjectiveTag, Progress);
}

UIslandObjectiveSubsystem *URfsnQuestBridge::GetObjectiveSubsystem() const {
  UWorld *World = GetWorld();
  if (!World) {
    return nullptr;
  }
  return World->GetSubsystem<UIslandObjectiveSubsystem>();
}

void URfsnQuestBridge::OnRfsnNpcAction(ERfsnNpcAction Action) {
  switch (Action) {
  case ERfsnNpcAction::Help:
    TriggerQuestProgress(HelpObjectiveTag, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("[QuestBridge] NPC helped - objective: %s"),
           *HelpObjectiveTag);
    break;

  case ERfsnNpcAction::Trade:
    TriggerQuestProgress(TradeObjectiveTag, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("[QuestBridge] Trade completed - objective: %s"),
           *TradeObjectiveTag);
    break;

  case ERfsnNpcAction::Explain:
  case ERfsnNpcAction::Answer:
    TriggerQuestProgress(IntelObjectiveTag, 0.5f);
    UE_LOG(LogTemp, Log, TEXT("[QuestBridge] Intel gathered - objective: %s"),
           *IntelObjectiveTag);
    break;

  case ERfsnNpcAction::Greet:
    // First contact - small progress
    TriggerQuestProgress(HelpObjectiveTag, 0.25f);
    break;

  default:
    // Other actions don't affect quests
    break;
  }
}
