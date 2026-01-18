#include "IslandHUD.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IslandDirectorSubsystem.h"
#include "IslandObjectiveSubsystem.h"
#include "IslandExtractionZone.h"
#include "IslandRadioTower.h"
#include "IslandGameMode.h"
#include "IslandInteractorComponent.h"
#include "Public/IslandVitalityComponent.h"
#include "MyProjectCharacter.h"

static FString TowerStateToString(ERadioTowerState S)
{
	switch (S)
	{
	case ERadioTowerState::Broken:        return TEXT("Damaged - Needs Repair");
	case ERadioTowerState::Unpowered:     return TEXT("Unpowered - Needs Fuel");
	case ERadioTowerState::Powered:       return TEXT("Ready - Transmit Signal");
	case ERadioTowerState::Transmitting:  return TEXT("Transmitting");
	case ERadioTowerState::ExtractWindow: return TEXT("ExtractWindow");
	case ERadioTowerState::Cooldown:      return TEXT("Cooldown");
	default:                              return TEXT("Unknown");
	}
}

void AIslandHUD::DrawHUD()
{
	Super::DrawHUD();

	UWorld* World = GetWorld();
	if (!World) return;

	float yy = Y;
	const float BarWidth = 200.0f;
	const float BarHeight = 12.0f;

	// Vitality Stats
	if (APawn* Player = GetOwningPawn())
	{
		if (UIslandVitalityComponent* Vitality = Player->FindComponentByClass<UIslandVitalityComponent>())
		{
			// Health
			DrawText(TEXT("Health"), FLinearColor::White, X, yy);
			DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
			DrawRect(FLinearColor::Red, X + 82, yy + 2, (BarWidth - 4) * Vitality->GetHealthNormalized(), BarHeight - 4);
			yy += Line;

			// Stamina
			DrawText(TEXT("Stamina"), FLinearColor::White, X, yy);
			DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
			DrawRect(FLinearColor::Blue, X + 82, yy + 2, (BarWidth - 4) * Vitality->GetStaminaNormalized(), BarHeight - 4);
			yy += Line;

			// Hunger
			DrawText(TEXT("Hunger"), FLinearColor::White, X, yy);
			DrawRect(FLinearColor::Black, X + 80, yy, BarWidth, BarHeight);
			DrawRect(FLinearColor::Green, X + 82, yy + 2, (BarWidth - 4) * Vitality->GetHungerNormalized(), BarHeight - 4);
			yy += Line + 10.0f;
		}
	}

	// Alert
	if (UIslandDirectorSubsystem* Dir = World->GetSubsystem<UIslandDirectorSubsystem>())
	{
		const float Alert = Dir->GetAlertLevel();
		FLinearColor AlertColor = FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor::Red, Alert / 100.0f);
		DrawText(FString::Printf(TEXT("Threat: %.1f%%"), Alert), AlertColor, X, yy);
		yy += Line;
		
		// Draw Alert Bar
		DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
		DrawRect(AlertColor, X + 2, yy + 2, (BarWidth - 4) * (Alert / 100.0f), BarHeight - 4);
		yy += BarHeight + 10.0f;
	}

	// Tutorial Message
	if (!CurrentTutorialMessage.IsEmpty() && World->GetTimeSeconds() < TutorialMessageExpireTime)
	{
		float ScreenW = Canvas->ClipX;
		float ScreenH = Canvas->ClipY;
		float TextScale = 1.5f;
		
		float TextW, TextH;
		GetTextSize(CurrentTutorialMessage, TextW, TextH, NULL, TextScale);
		
		float MsgX = (ScreenW - TextW) / 2.0f;
		float MsgY = ScreenH * 0.2f;

		// Draw background for legibility
		DrawRect(FLinearColor(0,0,0,0.5f), MsgX - 10, MsgY - 5, TextW + 20, TextH + 10);
		DrawText(CurrentTutorialMessage, FLinearColor::Yellow, MsgX, MsgY, NULL, TextScale);
	}

	AIslandGameMode* GM = Cast<AIslandGameMode>(World->GetAuthGameMode());
	
	// Tower State & Transmit Progress
	if (GM && GM->Tower)
	{
		DrawText(FString::Printf(TEXT("Radio: %s"), *TowerStateToString(GM->Tower->State)), FLinearColor::White, X, yy);
		yy += Line;

		if (GM->Tower->State == ERadioTowerState::Transmitting)
		{
			float Progress = GM->Tower->GetTransmitProgress();
			DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
			DrawRect(FLinearColor::Blue, X + 2, yy + 2, (BarWidth - 4) * Progress, BarHeight - 4);
			yy += BarHeight + 10.0f;
		}
	}
}

void AIslandHUD::ShowTutorialMessage(const FString& Message, float Duration)
{
	CurrentTutorialMessage = Message;
	if (UWorld* World = GetWorld())
	{
		TutorialMessageExpireTime = World->GetTimeSeconds() + Duration;
	}
	if (GM && GM->Extraction)
	{
		const bool bActive = GM->Extraction->bActive;
		if (bActive)
		{
			const float Rem = GM->Extraction->GetRemainingSeconds();
			DrawText(FString::Printf(TEXT("EXTRACT WINDOW: %.1fs"), Rem), FLinearColor::Green, X, yy);
			yy += Line;

			APlayerController* PC = World->GetFirstPlayerController();
			if (PC && PC->GetPawn())
			{
				float HoldProg = GM->Extraction->GetHoldProgress(PC->GetPawn());
				if (HoldProg > 0.0f)
				{
					DrawText(TEXT("Extracting..."), FLinearColor::White, X, yy);
					yy += Line;
					DrawRect(FLinearColor::Black, X, yy, BarWidth, BarHeight);
					DrawRect(FLinearColor::Green, X + 2, yy + 2, (BarWidth - 4) * HoldProg, BarHeight - 4);
					yy += BarHeight + 10.0f;
				}
			}
		}
	}

	// Interact prompt
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		APawn* P = PC->GetPawn();
		if (P)
		{
			if (UIslandInteractorComponent* Inter = P->FindComponentByClass<UIslandInteractorComponent>())
			{
				if (!Inter->FocusedPrompt.IsEmpty())
				{
					DrawText(FString::Printf(TEXT("[E] %s"), *Inter->FocusedPrompt.ToString()), FLinearColor::White, X, yy);
				}
			}
		}
	}
}
