#include "IslandHUD.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IslandDirectorSubsystem.h"
#include "IslandExtractionZone.h"
#include "IslandGameMode.h"
#include "IslandInventoryComponent.h"
#include "IslandInteractorComponent.h"
#include "IslandObjectiveSubsystem.h"
#include "IslandRadioTower.h"
#include "IslandStealthComponent.h"
#include "IslandVitalityComponent.h"
#include "MyProjectCharacter.h"
#include "RfsnNpcClientComponent.h"

static FString TowerStateToString(ERadioTowerState S) {
  switch (S) {
  case ERadioTowerState::Broken:
    return TEXT("Damaged");
  case ERadioTowerState::NeedsParts:
    return TEXT("Needs Parts");
  case ERadioTowerState::Unpowered:
    return TEXT("Unpowered");
  case ERadioTowerState::Repairing:
    return TEXT("Repairing");
  case ERadioTowerState::Powered:
    return TEXT("Powered");
  case ERadioTowerState::Transmitting:
    return TEXT("Transmitting");
  case ERadioTowerState::ExtractWindow:
    return TEXT("Extraction Window");
  case ERadioTowerState::Cooldown:
    return TEXT("Cooldown");
  default:
    return TEXT("Unknown");
  }
}

static FString ItemTypeToLabel(EIslandItemType ItemType) {
  switch (ItemType) {
  case EIslandItemType::TowerFuse:
    return TEXT("Fuse");
  case EIslandItemType::TowerFuel:
    return TEXT("Fuel");
  case EIslandItemType::AntennaCrank:
    return TEXT("Crank");
  case EIslandItemType::Medkit:
    return TEXT("Medkit");
  case EIslandItemType::Food:
    return TEXT("Food");
  case EIslandItemType::None:
  default:
    return TEXT("Unknown");
  }
}

void AIslandHUD::DrawHUD() {
  Super::DrawHUD();

  UWorld *World = GetWorld();
  if (!World)
    return;

  float yy = Y;
  const float BarWidth = 200.0f;
  const float BarHeight = 12.0f;
  APlayerController *PC = World->GetFirstPlayerController();
  APawn *PlayerPawn = PC ? PC->GetPawn() : nullptr;
  UIslandInventoryComponent *Inventory =
      PlayerPawn ? PlayerPawn->FindComponentByClass<UIslandInventoryComponent>()
                 : nullptr;
  UIslandStealthComponent *Stealth =
      PlayerPawn ? PlayerPawn->FindComponentByClass<UIslandStealthComponent>()
                 : nullptr;

  if (UIslandObjectiveSubsystem *Objectives =
          World->GetSubsystem<UIslandObjectiveSubsystem>()) {
    const FString ObjectiveText = Objectives->GetObjectiveText().ToString();
    if (!ObjectiveText.IsEmpty()) {
      DrawText(FString::Printf(TEXT("Objective: %s"), *ObjectiveText),
               FLinearColor::Yellow, X, yy);
      yy += Line + 6.0f;
    }
  }

  // Vitality Stats
  if (APawn *Player = GetOwningPawn()) {
    if (UIslandVitalityComponent *Vitality =
            Player->FindComponentByClass<UIslandVitalityComponent>()) {
      // Health
      DrawText(TEXT("Health"), FLinearColor::White, X, yy);
      DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Red, X + 82, yy + 2,
               (BarWidth - 4) * Vitality->GetHealthNormalized(), BarHeight - 4);
      yy += Line;

      // Stamina
      DrawText(TEXT("Stamina"), FLinearColor::White, X, yy);
      DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Blue, X + 82, yy + 2,
               (BarWidth - 4) * Vitality->GetStaminaNormalized(),
               BarHeight - 4);
      yy += Line;

      // Hunger
      DrawText(TEXT("Hunger"), FLinearColor::White, X, yy);
      DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Green, X + 82, yy + 2,
               (BarWidth - 4) * Vitality->GetHungerNormalized(), BarHeight - 4);
      yy += Line + 10.0f;
    }
  }

  // Alert
  if (UIslandDirectorSubsystem *Dir =
          World->GetSubsystem<UIslandDirectorSubsystem>()) {
    const float Alert = Dir->GetAlertLevel();
    FLinearColor AlertColor = FLinearColor::LerpUsingHSV(
        FLinearColor::White, FLinearColor::Red, Alert / 100.0f);
    DrawText(FString::Printf(TEXT("Threat: %.1f%%"), Alert), AlertColor, X, yy);
    yy += Line;

    // Draw Alert Bar
    DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
    DrawRect(AlertColor, X + 2, yy + 2, (BarWidth - 4) * (Alert / 100.0f),
             BarHeight - 4);
    yy += BarHeight + 10.0f;
  }

  // Tutorial Message
  if (!CurrentTutorialMessage.IsEmpty() &&
      World->GetTimeSeconds() < TutorialMessageExpireTime) {
    float ScreenW = Canvas->ClipX;
    float ScreenH = Canvas->ClipY;
    float TextScale = 1.5f;

    float TextW, TextH;
    GetTextSize(CurrentTutorialMessage, TextW, TextH, NULL, TextScale);

    float MsgX = (ScreenW - TextW) / 2.0f;
    float MsgY = ScreenH * 0.2f;

    // Draw background for legibility
    DrawRect(FLinearColor(0, 0, 0, 0.5f), MsgX - 10, MsgY - 5, TextW + 20,
             TextH + 10);
    DrawText(CurrentTutorialMessage, FLinearColor::Yellow, MsgX, MsgY, NULL,
             TextScale);
  }

  // NPC Dialogue Subtitles (bottom of screen)
  if (!CurrentDialogueSentence.IsEmpty() &&
      World->GetTimeSeconds() < DialogueExpireTime) {
    float ScreenW = Canvas->ClipX;
    float ScreenH = Canvas->ClipY;
    float TextScale = 1.2f;

    // Format: "NpcName: Sentence"
    FString DialogueText = FString::Printf(
        TEXT("%s: %s"), *CurrentDialogueNpcName, *CurrentDialogueSentence);

    float TextW, TextH;
    GetTextSize(DialogueText, TextW, TextH, NULL, TextScale);

    // Center horizontally, 15% from bottom
    float MsgX = (ScreenW - TextW) / 2.0f;
    float MsgY = ScreenH * 0.85f;

    // Clamp width for long sentences
    float MaxWidth = ScreenW * 0.8f;
    if (TextW > MaxWidth) {
      TextScale = TextScale * (MaxWidth / TextW);
      GetTextSize(DialogueText, TextW, TextH, NULL, TextScale);
      MsgX = (ScreenW - TextW) / 2.0f;
    }

    // Dark background box
    DrawRect(FLinearColor(0, 0, 0, 0.7f), MsgX - 15, MsgY - 8, TextW + 30,
             TextH + 16);

    // NPC name in cyan, rest in white
    DrawText(DialogueText, FLinearColor(0.2f, 0.9f, 1.0f, 1.0f), MsgX, MsgY,
             NULL, TextScale);
  }

  AIslandGameMode *GM = Cast<AIslandGameMode>(World->GetAuthGameMode());

  // Tower State & Transmit Progress
  if (GM && GM->Tower) {
    DrawText(FString::Printf(TEXT("Radio: %s"),
                             *TowerStateToString(GM->Tower->State)),
             FLinearColor::White, X, yy);
    yy += Line;

    if (Inventory && GM->Tower->RequiredParts.Num() > 0) {
      DrawText(TEXT("Tower Parts:"), FLinearColor::White, X, yy);
      yy += Line;
      for (const EIslandItemType RequiredItem : GM->Tower->RequiredParts) {
        const bool bHasItem =
            GM->Tower->AreRequiredPartsInstalled() ||
            (Inventory && Inventory->HasItem(RequiredItem));
        DrawText(FString::Printf(TEXT("  %s [%s]"),
                                 *ItemTypeToLabel(RequiredItem),
                                 bHasItem ? TEXT("X") : TEXT(" ")),
                 bHasItem ? FLinearColor::Green : FLinearColor::White, X, yy);
        yy += Line;
      }
    }

    if (GM->Tower->bRepairInProgress || GM->Tower->State == ERadioTowerState::Repairing ||
        GM->Tower->RepairProgress > 0.0f) {
      DrawText(TEXT("Repair Progress"), FLinearColor::White, X, yy);
      yy += Line;
      DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Yellow, X + 2, yy + 2,
               (BarWidth - 4) * GM->Tower->RepairProgress, BarHeight - 4);
      yy += BarHeight + 10.0f;
    }

    if (GM->Tower->State == ERadioTowerState::Transmitting) {
      float Progress = GM->Tower->GetTransmitProgress();
      DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
      DrawRect(FLinearColor::Blue, X + 2, yy + 2, (BarWidth - 4) * Progress,
               BarHeight - 4);
      yy += BarHeight + 10.0f;
    }
  }

  if (Stealth) {
    DrawText(FString::Printf(TEXT("Noise: %.0f%%"),
                             Stealth->GetNoiseLoudness() * 100.0f),
             FLinearColor::White, X, yy);
    yy += Line;
    DrawText(FString::Printf(TEXT("Visibility: %.0f%%"),
                             Stealth->GetVisibilityMultiplier() * 100.0f),
             Stealth->bFlashlightOn ? FLinearColor::Yellow : FLinearColor::White,
             X, yy);
    yy += Line + 4.0f;
  }

  if (GM && GM->Extraction) {
    const bool bActive = GM->Extraction->bActive;
    if (bActive) {
      const float Rem = GM->Extraction->GetRemainingSeconds();
      DrawText(FString::Printf(TEXT("EXTRACT WINDOW: %.1fs"), Rem),
               FLinearColor::Green, X, yy);
      yy += Line;

      if (PC && PC->GetPawn()) {
        float HoldProg = GM->Extraction->GetHoldProgress(PC->GetPawn());
        if (HoldProg > 0.0f) {
          DrawText(TEXT("Extracting..."), FLinearColor::White, X, yy);
          yy += Line;
          DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
          DrawRect(FLinearColor::Green, X + 2, yy + 2,
                   (BarWidth - 4) * HoldProg, BarHeight - 4);
          yy += BarHeight + 10.0f;
        }
      }
    }
  }

  // Interact prompt
  if (PC) {
    APawn *P = PC->GetPawn();
    if (P) {
      if (UIslandInteractorComponent *Inter =
              P->FindComponentByClass<UIslandInteractorComponent>()) {
        if (!Inter->FocusedPrompt.IsEmpty()) {
          DrawText(
              FString::Printf(TEXT("[E] %s"), *Inter->FocusedPrompt.ToString()),
              FLinearColor::White, X, yy);
        }
      }
    }
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
