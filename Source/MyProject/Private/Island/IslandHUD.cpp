#include "Island/IslandHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Island/IslandDirectorSubsystem.h"
#include "Island/IslandExtractionZone.h"
#include "Island/IslandGameMode.h"
#include "Island/IslandHUDWidget.h"
#include "Island/IslandInventoryComponent.h"
#include "Island/IslandInteractorComponent.h"
#include "Island/IslandObjectiveSubsystem.h"
#include "Island/IslandRadioTower.h"
#include "Island/IslandStealthComponent.h"
#include "Island/IslandVitalityComponent.h"
#include "Dialogue/RfsnNpcClientComponent.h"

static FText TowerStateToText(ERadioTowerState S) {
  switch (S) {
  case ERadioTowerState::Broken:
    return FText::FromString(TEXT("Damaged"));
  case ERadioTowerState::NeedsParts:
    return FText::FromString(TEXT("Needs Parts"));
  case ERadioTowerState::Unpowered:
    return FText::FromString(TEXT("Unpowered"));
  case ERadioTowerState::Repairing:
    return FText::FromString(TEXT("Repairing"));
  case ERadioTowerState::Powered:
    return FText::FromString(TEXT("Powered"));
  case ERadioTowerState::Transmitting:
    return FText::FromString(TEXT("Transmitting"));
  case ERadioTowerState::ExtractWindow:
    return FText::FromString(TEXT("Extraction Window"));
  case ERadioTowerState::Cooldown:
    return FText::FromString(TEXT("Cooldown"));
  default:
    return FText::FromString(TEXT("Unknown"));
  }
}

static FText ItemTypeToText(EIslandItemType ItemType) {
  switch (ItemType) {
  case EIslandItemType::TowerFuse:
    return FText::FromString(TEXT("Fuse"));
  case EIslandItemType::TowerFuel:
    return FText::FromString(TEXT("Fuel"));
  case EIslandItemType::AntennaCrank:
    return FText::FromString(TEXT("Crank"));
  case EIslandItemType::Medkit:
    return FText::FromString(TEXT("Medkit"));
  case EIslandItemType::Food:
    return FText::FromString(TEXT("Food"));
  case EIslandItemType::None:
  default:
    return FText::FromString(TEXT("Unknown"));
  }
}

void AIslandHUD::BeginPlay() {
  Super::BeginPlay();
  EnsureWidgetCreated();
}

void AIslandHUD::EnsureWidgetCreated() {
  if (IslandHUDWidget) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  APlayerController *PC = PlayerOwner;
  if (!PC) {
    PC = World->GetFirstPlayerController();
  }
  if (!PC || !PC->IsLocalController()) {
    return;
  }

  AIslandGameMode *GM = Cast<AIslandGameMode>(World->GetAuthGameMode());
  if (!GM || !GM->IslandHUDWidgetClass) {
    return;
  }

  IslandHUDWidget = CreateWidget<UIslandHUDWidget>(PC, GM->IslandHUDWidgetClass);
  if (IslandHUDWidget) {
    IslandHUDWidget->AddToViewport(0);
  }
}

FIslandHUDState AIslandHUD::BuildHudState() const {
  FIslandHUDState State;

  UWorld *World = GetWorld();
  if (!World) {
    return State;
  }

  APlayerController *PC = PlayerOwner;
  if (!PC) {
    PC = World->GetFirstPlayerController();
  }
  APawn *PlayerPawn = PC ? PC->GetPawn() : nullptr;

  if (UIslandObjectiveSubsystem *Objectives =
          World->GetSubsystem<UIslandObjectiveSubsystem>()) {
    State.ObjectiveText = Objectives->GetObjectiveText();
  }

  if (PlayerPawn) {
    if (UIslandVitalityComponent *Vitality =
            PlayerPawn->FindComponentByClass<UIslandVitalityComponent>()) {
      State.bHasVitality = true;
      State.HealthNormalized = Vitality->GetHealthNormalized();
      State.StaminaNormalized = Vitality->GetStaminaNormalized();
      State.HungerNormalized = Vitality->GetHungerNormalized();
    }

    if (UIslandStealthComponent *Stealth =
            PlayerPawn->FindComponentByClass<UIslandStealthComponent>()) {
      State.NoisePercent = Stealth->GetNoiseLoudness() * 100.0f;
      State.VisibilityPercent = Stealth->GetVisibilityMultiplier() * 100.0f;
      State.bFlashlightOn = Stealth->bFlashlightOn;
    }

    if (UIslandInteractorComponent *Interactor =
            PlayerPawn->FindComponentByClass<UIslandInteractorComponent>()) {
      State.InteractPrompt = Interactor->FocusedPrompt;
    }
  }

  if (UIslandDirectorSubsystem *Dir =
          World->GetSubsystem<UIslandDirectorSubsystem>()) {
    State.ThreatPercent = Dir->GetAlertLevel();
  }

  if (!CurrentTutorialMessage.IsEmpty() &&
      World->GetTimeSeconds() < TutorialMessageExpireTime) {
    State.bShowTutorial = true;
    State.TutorialMessage = FText::FromString(CurrentTutorialMessage);
  }

  if (!CurrentDialogueSentence.IsEmpty() &&
      World->GetTimeSeconds() < DialogueExpireTime) {
    State.bShowDialogue = true;
    State.DialogueSpeaker = FText::FromString(CurrentDialogueNpcName);
    State.DialogueSentence = FText::FromString(CurrentDialogueSentence);
  }

  if (AIslandGameMode *GM = Cast<AIslandGameMode>(World->GetAuthGameMode())) {
    if (GM->Tower) {
      State.RadioStateText = TowerStateToText(GM->Tower->State);
      State.RepairProgress = GM->Tower->RepairProgress;
      State.bShowRepairProgress = GM->Tower->bRepairInProgress ||
                                  GM->Tower->State ==
                                      ERadioTowerState::Repairing ||
                                  GM->Tower->RepairProgress > 0.0f;
      State.TransmitProgress = GM->Tower->GetTransmitProgress();
      State.bShowTransmitProgress =
          GM->Tower->State == ERadioTowerState::Transmitting;

      UIslandInventoryComponent *Inventory =
          PlayerPawn
              ? PlayerPawn->FindComponentByClass<UIslandInventoryComponent>()
              : nullptr;
      for (const EIslandItemType RequiredItem : GM->Tower->RequiredParts) {
        FIslandHUDPartState PartState;
        PartState.Label = ItemTypeToText(RequiredItem);
        PartState.bCollected =
            GM->Tower->AreRequiredPartsInstalled() ||
            (Inventory && Inventory->HasItem(RequiredItem));
        State.TowerParts.Add(PartState);
      }
    }

    if (GM->Extraction) {
      State.bExtractionActive = GM->Extraction->bActive;
      State.ExtractionRemainingSeconds = GM->Extraction->GetRemainingSeconds();
      if (PlayerPawn) {
        State.ExtractionHoldProgress =
            GM->Extraction->GetHoldProgress(PlayerPawn);
      }
    }
  }

  return State;
}

void AIslandHUD::DrawHUD() {
  Super::DrawHUD();

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  EnsureWidgetCreated();
  const FIslandHUDState State = BuildHudState();
  if (IslandHUDWidget) {
    IslandHUDWidget->ApplyHudState(State);
    if (bHideCanvasWhenWidgetActive) {
      return;
    }
  }

  float yy = Y;
  const float BarWidth = 200.0f;
  const float BarHeight = 12.0f;

  if (!State.ObjectiveText.IsEmpty()) {
    DrawText(FString::Printf(TEXT("Objective: %s"),
                             *State.ObjectiveText.ToString()),
             FLinearColor::Yellow, X, yy);
    yy += Line + 6.0f;
  }

  if (State.bHasVitality) {
    DrawText(TEXT("Health"), FLinearColor::White, X, yy);
    DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
    DrawRect(FLinearColor::Red, X + 82, yy + 2,
             (BarWidth - 4) * State.HealthNormalized, BarHeight - 4);
    yy += Line;

    DrawText(TEXT("Stamina"), FLinearColor::White, X, yy);
    DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
    DrawRect(FLinearColor::Blue, X + 82, yy + 2,
             (BarWidth - 4) * State.StaminaNormalized, BarHeight - 4);
    yy += Line;

    DrawText(TEXT("Hunger"), FLinearColor::White, X, yy);
    DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
    DrawRect(FLinearColor::Green, X + 82, yy + 2,
             (BarWidth - 4) * State.HungerNormalized, BarHeight - 4);
    yy += Line + 10.0f;
  }

  FLinearColor AlertColor = FLinearColor::LerpUsingHSV(
      FLinearColor::White, FLinearColor::Red, State.ThreatPercent / 100.0f);
  DrawText(FString::Printf(TEXT("Threat: %.1f%%"), State.ThreatPercent),
           AlertColor, X, yy);
  yy += Line;
  DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
  DrawRect(AlertColor, X + 2, yy + 2,
           (BarWidth - 4) * (State.ThreatPercent / 100.0f), BarHeight - 4);
  yy += BarHeight + 10.0f;

  if (State.bShowTutorial) {
    float ScreenW = Canvas->ClipX;
    float ScreenH = Canvas->ClipY;
    float TextScale = 1.5f;
    const FString TutorialText = State.TutorialMessage.ToString();

    float TextW, TextH;
    GetTextSize(TutorialText, TextW, TextH, NULL, TextScale);

    float MsgX = (ScreenW - TextW) / 2.0f;
    float MsgY = ScreenH * 0.2f;
    DrawRect(FLinearColor(0, 0, 0, 0.5f), MsgX - 10, MsgY - 5, TextW + 20,
             TextH + 10);
    DrawText(TutorialText, FLinearColor::Yellow, MsgX, MsgY, NULL, TextScale);
  }

  if (State.bShowDialogue) {
    float ScreenW = Canvas->ClipX;
    float ScreenH = Canvas->ClipY;
    float TextScale = 1.2f;
    const FString DialogueText =
        FString::Printf(TEXT("%s: %s"), *State.DialogueSpeaker.ToString(),
                        *State.DialogueSentence.ToString());

    float TextW, TextH;
    GetTextSize(DialogueText, TextW, TextH, NULL, TextScale);

    float MsgX = (ScreenW - TextW) / 2.0f;
    float MsgY = ScreenH * 0.85f;
    float MaxWidth = ScreenW * 0.8f;
    if (TextW > MaxWidth) {
      TextScale = TextScale * (MaxWidth / TextW);
      GetTextSize(DialogueText, TextW, TextH, NULL, TextScale);
      MsgX = (ScreenW - TextW) / 2.0f;
    }

    DrawRect(FLinearColor(0, 0, 0, 0.7f), MsgX - 15, MsgY - 8, TextW + 30,
             TextH + 16);
    DrawText(DialogueText, FLinearColor(0.2f, 0.9f, 1.0f, 1.0f), MsgX, MsgY,
             NULL, TextScale);
  }

  if (!State.RadioStateText.IsEmpty()) {
    DrawText(
        FString::Printf(TEXT("Radio: %s"), *State.RadioStateText.ToString()),
        FLinearColor::White, X, yy);
    yy += Line;

    if (State.TowerParts.Num() > 0) {
      DrawText(TEXT("Tower Parts:"), FLinearColor::White, X, yy);
      yy += Line;
      for (const FIslandHUDPartState &PartState : State.TowerParts) {
        DrawText(FString::Printf(TEXT("  %s [%s]"),
                                 *PartState.Label.ToString(),
                                 PartState.bCollected ? TEXT("X") : TEXT(" ")),
                 PartState.bCollected ? FLinearColor::Green
                                      : FLinearColor::White,
                 X, yy);
        yy += Line;
      }
    }

    if (State.bShowRepairProgress) {
      DrawText(TEXT("Repair Progress"), FLinearColor::White, X, yy);
      yy += Line;
      DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Yellow, X + 2, yy + 2,
               (BarWidth - 4) * State.RepairProgress, BarHeight - 4);
      yy += BarHeight + 10.0f;
    }

    if (State.bShowTransmitProgress) {
      DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Blue, X + 2, yy + 2,
               (BarWidth - 4) * State.TransmitProgress, BarHeight - 4);
      yy += BarHeight + 10.0f;
    }
  }

  if (State.VisibilityPercent > 0.0f || State.NoisePercent > 0.0f ||
      State.bFlashlightOn) {
    DrawText(FString::Printf(TEXT("Noise: %.0f%%"), State.NoisePercent),
             FLinearColor::White, X, yy);
    yy += Line;
    DrawText(FString::Printf(TEXT("Visibility: %.0f%%"), State.VisibilityPercent),
             State.bFlashlightOn ? FLinearColor::Yellow
                                 : FLinearColor::White,
             X, yy);
    yy += Line + 4.0f;
  }

  if (State.bExtractionActive) {
    DrawText(
        FString::Printf(TEXT("EXTRACT WINDOW: %.1fs"),
                        State.ExtractionRemainingSeconds),
        FLinearColor::Green, X, yy);
    yy += Line;

    if (State.ExtractionHoldProgress > 0.0f) {
      DrawText(TEXT("Extracting..."), FLinearColor::White, X, yy);
      yy += Line;
      DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Green, X + 2, yy + 2,
               (BarWidth - 4) * State.ExtractionHoldProgress, BarHeight - 4);
      yy += BarHeight + 10.0f;
    }
  }

  if (!State.InteractPrompt.IsEmpty()) {
    DrawText(FString::Printf(TEXT("[E] %s"), *State.InteractPrompt.ToString()),
             FLinearColor::White, X, yy);
  }
}

void AIslandHUD::ShowTutorialMessage(const FString &Message, float Duration) {
  CurrentTutorialMessage = Message;
  if (UWorld *World = GetWorld()) {
    TutorialMessageExpireTime = World->GetTimeSeconds() + Duration;
  }
}

void AIslandHUD::ShowNpcDialogue(const FString &NpcName,
                                 const FString &Sentence, float Duration) {
  CurrentDialogueNpcName = NpcName;
  CurrentDialogueSentence = Sentence;
  if (UWorld *World = GetWorld()) {
    DialogueExpireTime = World->GetTimeSeconds() + Duration;
  }
}

void AIslandHUD::ClearNpcDialogue() {
  CurrentDialogueNpcName.Empty();
  CurrentDialogueSentence.Empty();
  DialogueExpireTime = 0.0f;
}

void AIslandHUD::BindToRfsnClient(URfsnNpcClientComponent *RfsnClient) {
  if (RfsnClient) {
    BoundNpcName = RfsnClient->NpcName;
    RfsnClient->OnSentenceReceived.AddDynamic(this,
                                              &AIslandHUD::OnRfsnSentence);
  }
}

void AIslandHUD::OnRfsnSentence(const FRfsnSentence &Sentence) {
  if (!Sentence.Sentence.IsEmpty()) {
    ShowNpcDialogue(BoundNpcName, Sentence.Sentence, 5.0f);
  }
}
