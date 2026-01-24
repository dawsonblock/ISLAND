// RFSN Director Bridge Implementation
// Connects RFSN Orchestrator decisions to Island game systems

#include "RfsnDirectorBridge.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "IslandDirectorSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

URfsnDirectorBridge::URfsnDirectorBridge() {
  PrimaryComponentTick.bCanEverTick = false;
}

void URfsnDirectorBridge::BeginPlay() {
  Super::BeginPlay();

  // Cache Director subsystem reference
  if (UWorld *World = GetWorld()) {
    DirectorSubsystem = World->GetSubsystem<UIslandDirectorSubsystem>();
  }

  if (bAutoPolling) {
    StartPolling();
  }
}

void URfsnDirectorBridge::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  StopPolling();
  Super::EndPlay(EndPlayReason);
}

void URfsnDirectorBridge::StartPolling() {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().SetTimer(
        PollTimer, this, &URfsnDirectorBridge::OnPollTick, PollInterval,
        true // Loop
    );
  }
}

void URfsnDirectorBridge::StopPolling() {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(PollTimer);
  }
}

void URfsnDirectorBridge::OnPollTick() { RequestDirectorCommand(); }

void URfsnDirectorBridge::RequestDirectorCommand() {
  if (!DirectorSubsystem) {
    UE_LOG(LogTemp, Warning, TEXT("[RFSN] Director subsystem not available"));
    return;
  }

  // Build game state payload for RFSN
  TSharedPtr<FJsonObject> GameState = MakeShareable(new FJsonObject());
  GameState->SetNumberField(TEXT("alert_level"),
                            DirectorSubsystem->GetAlertLevel());
  GameState->SetNumberField(TEXT("intensity"),
                            DirectorSubsystem->GetNormalizedIntensity());
  GameState->SetBoolField(TEXT("can_use_tower"),
                          DirectorSubsystem->CanUseTower());
  GameState->SetBoolField(TEXT("can_transmit"),
                          DirectorSubsystem->CanTransmit());

  // Add intensity state
  FString IntensityState;
  switch (DirectorSubsystem->CurrentIntensity) {
  case EIslandIntensityState::Passive:
    IntensityState = TEXT("passive");
    break;
  case EIslandIntensityState::Alerted:
    IntensityState = TEXT("alerted");
    break;
  case EIslandIntensityState::Hostile:
    IntensityState = TEXT("hostile");
    break;
  case EIslandIntensityState::Overwhelmed:
    IntensityState = TEXT("overwhelmed");
    break;
  }
  GameState->SetStringField(TEXT("intensity_state"), IntensityState);

  FString JsonString;
  TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
  FJsonSerializer::Serialize(GameState.ToSharedRef(), Writer);

  // Send to RFSN
  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
      FHttpModule::Get().CreateRequest();
  Request->SetURL(DirectorUrl);
  Request->SetVerb(TEXT("POST"));
  Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
  Request->SetContentAsString(JsonString);
  Request->OnProcessRequestComplete().BindUObject(
      this, &URfsnDirectorBridge::OnDirectorResponse);
  Request->ProcessRequest();

  UE_LOG(LogTemp, Verbose,
         TEXT("[RFSN] Sent director state: alert=%.1f, intensity=%s"),
         DirectorSubsystem->GetAlertLevel(), *IntensityState);
}

void URfsnDirectorBridge::OnDirectorResponse(FHttpRequestPtr Request,
                                             FHttpResponsePtr Response,
                                             bool bSuccess) {
  if (!bSuccess || !Response.IsValid()) {
    // Silently fail - RFSN might not be running
    return;
  }

  if (Response->GetResponseCode() != 200) {
    return;
  }

  // Parse RFSN director command
  TSharedPtr<FJsonObject> JsonObject;
  TSharedRef<TJsonReader<>> Reader =
      TJsonReaderFactory<>::Create(Response->GetContentAsString());

  if (!FJsonSerializer::Deserialize(Reader, JsonObject) ||
      !JsonObject.IsValid()) {
    return;
  }

  // Process director commands
  FString Command;
  if (JsonObject->TryGetStringField(TEXT("command"), Command)) {
    if (Command == TEXT("spawn_horde") && DirectorSubsystem) {
      DirectorSubsystem->AddAlert(25.0f);
      UE_LOG(LogTemp, Log,
             TEXT("[RFSN] Director command: spawn_horde -> adding alert"));
    } else if (Command == TEXT("respite") && DirectorSubsystem) {
      // Let alert decay naturally - could modify decay rate
      UE_LOG(LogTemp, Log, TEXT("[RFSN] Director command: respite"));
    } else if (Command == TEXT("escalate") && DirectorSubsystem) {
      DirectorSubsystem->AddAlert(15.0f);
      UE_LOG(LogTemp, Log, TEXT("[RFSN] Director command: escalate"));
    }
  }

  // Process alert modification
  double AlertMod = 0.0;
  if (JsonObject->TryGetNumberField(TEXT("alert_modifier"), AlertMod) &&
      DirectorSubsystem) {
    DirectorSubsystem->AddAlert(static_cast<float>(AlertMod));
  }
}

void URfsnDirectorBridge::ApplyNpcActionToDirector(ERfsnNpcAction Action) {
  if (!DirectorSubsystem) {
    return;
  }

  float AlertMod = GetAlertModifierForAction(Action);
  if (AlertMod != 0.0f) {
    DirectorSubsystem->AddAlert(AlertMod);
    UE_LOG(LogTemp, Log, TEXT("[RFSN] NPC action %d -> alert modifier %.1f"),
           static_cast<int32>(Action), AlertMod);
  }
}

float URfsnDirectorBridge::GetAlertModifierForAction(
    ERfsnNpcAction Action) const {
  switch (Action) {
  case ERfsnNpcAction::Attack:
  case ERfsnNpcAction::Threaten:
    return 10.0f;

  case ERfsnNpcAction::Warn:
  case ERfsnNpcAction::Flee:
    return 5.0f;

  case ERfsnNpcAction::Greet:
  case ERfsnNpcAction::Help:
  case ERfsnNpcAction::Trade:
  case ERfsnNpcAction::Agree:
    return -2.0f;

  case ERfsnNpcAction::Apologize:
    return -5.0f;

  default:
    return 0.0f;
  }
}
