// RFSN Sample Guard Implementation

#include "RfsnSampleGuard.h"
#include "RfsnNpcClientComponent.h"

ARfsnSampleGuard::ARfsnSampleGuard() {
  // ShooterNPC already creates RfsnClient
}

void ARfsnSampleGuard::BeginPlay() {
  Super::BeginPlay();

  // Configure RFSN based on initial patrol state
  if (bOnPatrol) {
    ConfigureAsNeutralGuard();
  } else {
    ConfigureAsHostileGuard();
  }

  UE_LOG(LogTemp, Log, TEXT("[Guard] %s initialized, patrol=%d"), *GetName(),
         bOnPatrol);
}

void ARfsnSampleGuard::ConfigureAsHostileGuard() {
  if (RfsnClient) {
    RfsnClient->NpcName = TEXT("Hostile Guard");
    RfsnClient->Mood = TEXT("Aggressive");
    RfsnClient->Relationship = TEXT("Enemy");
    RfsnClient->Affinity = -0.8f;
  }
}

void ARfsnSampleGuard::ConfigureAsNeutralGuard() {
  if (RfsnClient) {
    RfsnClient->NpcName = TEXT("Guard");
    RfsnClient->Mood = TEXT("Suspicious");
    RfsnClient->Relationship = TEXT("Stranger");
    RfsnClient->Affinity = -0.2f;
  }
}

void ARfsnSampleGuard::OnRfsnNpcAction(ERfsnNpcAction Action) {
  // Let parent handle basic action mapping
  Super::OnRfsnNpcAction(Action);

  // Guard-specific behavior
  switch (Action) {
  case ERfsnNpcAction::Warn:
    bPlayerWarned = true;
    UE_LOG(LogTemp, Log, TEXT("[Guard] Warning player!"));
    // Could play warning animation or trigger alert
    break;

  case ERfsnNpcAction::Threaten:
    if (bPlayerWarned) {
      // Already warned - become hostile
      ConfigureAsHostileGuard();
      bOnPatrol = false;
      UE_LOG(LogTemp, Log,
             TEXT("[Guard] Player ignored warning - becoming hostile!"));
    } else {
      bPlayerWarned = true;
    }
    break;

  case ERfsnNpcAction::Accept:
    // Player complied - stay neutral
    UE_LOG(LogTemp, Log, TEXT("[Guard] Player complied, standing down"));
    break;

  case ERfsnNpcAction::Attack:
    bOnPatrol = false;
    ConfigureAsHostileGuard();
    UE_LOG(LogTemp, Log, TEXT("[Guard] Engaging player!"));
    // Parent class handles attack behavior
    break;

  default:
    break;
  }
}
