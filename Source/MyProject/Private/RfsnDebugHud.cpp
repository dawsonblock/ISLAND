// RFSN Debug HUD Implementation

#include "RfsnDebugHud.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "RfsnDialogueManager.h"
#include "RfsnNpcClientComponent.h"

URfsnDebugHud::URfsnDebugHud() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickInterval = 0.5f; // Update 2x per second
}

void URfsnDebugHud::BeginPlay() { Super::BeginPlay(); }

void URfsnDebugHud::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // Could ping server health here
}

void URfsnDebugHud::UpdateFromMeta(const FString &NpcAction,
                                   const FString &PlayerSignal,
                                   const FString &BanditKey) {
  LastNpcAction = NpcAction;
  LastPlayerSignal = PlayerSignal;
  LastBanditKey = BanditKey;
  bServerConnected = true;
}

FString URfsnDebugHud::GetActiveNpcName() const {
  UWorld *World = GetWorld();
  if (!World) {
    return TEXT("None");
  }

  URfsnDialogueManager *Manager = World->GetSubsystem<URfsnDialogueManager>();
  if (!Manager || !Manager->IsDialogueActive()) {
    return TEXT("None");
  }

  AActor *Npc = Manager->GetActiveNpc();
  if (!Npc) {
    return TEXT("None");
  }

  URfsnNpcClientComponent *Client =
      Npc->FindComponentByClass<URfsnNpcClientComponent>();
  return Client ? Client->NpcName : Npc->GetName();
}

void URfsnDebugHud::DrawDebugInfo(AHUD *HUD) {
  if (!bEnabled || !HUD) {
    return;
  }

  UCanvas *Canvas = HUD->Canvas;
  if (!Canvas) {
    return;
  }

  float Y = ScreenY;
  float LineHeight = 18.0f;

  // Header
  HUD->DrawText(TEXT("=== RFSN DEBUG ==="), FLinearColor::Yellow, ScreenX, Y);
  Y += LineHeight;

  // Server status
  FLinearColor StatusColor =
      bServerConnected ? FLinearColor::Green : FLinearColor::Red;
  HUD->DrawText(FString::Printf(TEXT("Server: %s"), bServerConnected
                                                        ? TEXT("Connected")
                                                        : TEXT("Disconnected")),
                StatusColor, ScreenX, Y);
  Y += LineHeight;

  // Active NPC
  HUD->DrawText(FString::Printf(TEXT("Active NPC: %s"), *GetActiveNpcName()),
                FLinearColor::White, ScreenX, Y);
  Y += LineHeight;

  // Last action
  HUD->DrawText(FString::Printf(TEXT("Last Action: %s"), *LastNpcAction),
                FLinearColor::Cyan, ScreenX, Y);
  Y += LineHeight;

  // Player signal
  HUD->DrawText(FString::Printf(TEXT("Player Signal: %s"), *LastPlayerSignal),
                FLinearColor::White, ScreenX, Y);
  Y += LineHeight;

  // Bandit key (truncated)
  FString TruncatedKey = LastBanditKey.Len() > 30
                             ? LastBanditKey.Left(30) + TEXT("...")
                             : LastBanditKey;
  HUD->DrawText(FString::Printf(TEXT("Bandit Key: %s"), *TruncatedKey),
                FLinearColor(0.5f, 0.5f, 0.5f), ScreenX, Y);
}
