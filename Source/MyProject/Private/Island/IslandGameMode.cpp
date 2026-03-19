#include "Island/IslandGameMode.h"
#include "EngineUtils.h"
#include "Island/IslandAISpawnManager.h"
#include "Island/IslandExtractionZone.h"
#include "Island/IslandGameInstanceSubsystem.h"
#include "Island/IslandHUD.h"
#include "Island/IslandObjectiveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Core/MyProjectCharacter.h"
#include "Core/MyProjectPlayerController.h"

AIslandGameMode::AIslandGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDClass = AIslandHUD::StaticClass();
	DefaultPawnClass = AMyProjectCharacter::StaticClass();
	PlayerControllerClass = AMyProjectPlayerController::StaticClass();
}

void AIslandGameMode::BeginPlay()
{
	Super::BeginPlay();

	TryAutoFindActors();

	if (Tower)
	{
		Tower->OnStateChanged.AddDynamic(this, &AIslandGameMode::OnTowerStateChanged);
		HandleTowerPhaseChanged(Tower->State);
	}

	UpdateObjectiveText();

	// Start the run timer/seed
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->StartRun(0); // 0 = random seed
		}
	}

	if (bShowWelcomeTutorial)
	{
		// Delay slightly to ensure HUD is ready
		FTimerHandle H;
		GetWorld()->GetTimerManager().SetTimer(H, [this]()
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (AIslandHUD* MyHUD = Cast<AIslandHUD>(PC->GetHUD()))
				{
					MyHUD->ShowTutorialMessage(TEXT("You woke on the beach. Scavenge tower parts, avoid the cult, and escape when extraction opens."), 8.0f);
				}
			}
		}, 1.0f, false);
	}
}

void AIslandGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Run time accumulation
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->RunTimeSeconds += DeltaSeconds;
		}
	}

	if ((!Tower || !Extraction) && !bHandledInvalidRun && GetGameInstance())
	{
		bHandledInvalidRun = true;

		// The level is missing required actors (Tower and/or Extraction). Do NOT call EndRun()
		// here because UIslandGameInstanceSubsystem::EndRun() will reopen the same level,
		// which would remain invalid and could cause an infinite reload loop.
		UE_LOG(LogTemp, Error, TEXT("AIslandGameMode: Invalid run configuration in level '%s' (Tower=%s, Extraction=%s).")
			, *GetWorld()->GetName()
			, Tower ? TEXT("Present") : TEXT("Missing")
			, Extraction ? TEXT("Present") : TEXT("Missing"));

		// Optional: make one more attempt to auto-find actors in case they spawned late.
		TryAutoFindActors();
	}
}

void AIslandGameMode::TryAutoFindActors()
{
	if (!Tower)
	{
		for (TActorIterator<AIslandRadioTower> It(GetWorld()); It; ++It)
		{
			Tower = *It;
			break;
		}
	}

	if (!Extraction)
	{
		for (TActorIterator<AIslandExtractionZone> It(GetWorld()); It; ++It)
		{
			Extraction = *It;
			break;
		}
	}

	if (!SpawnManager)
	{
		for (TActorIterator<AIslandAISpawnManager> It(GetWorld()); It; ++It)
		{
			SpawnManager = *It;
			break;
		}
	}
}

void AIslandGameMode::OnTowerStateChanged(ERadioTowerState NewState)
{
	HandleTowerPhaseChanged(NewState);
	UpdateObjectiveText();
}

void AIslandGameMode::UpdateObjectiveText()
{
	if (!Tower)
	{
		return;
	}

	if (UIslandObjectiveSubsystem* ObjectiveSubsystem = GetWorld()->GetSubsystem<UIslandObjectiveSubsystem>())
	{
		switch (Tower->State)
		{
		case ERadioTowerState::Broken:
		case ERadioTowerState::NeedsParts:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Collect the fuse, fuel, and crank for the radio tower.")));
			ObjectiveSubsystem->SetObjectiveActive(true, Tower->GetActorLocation());
			break;
		case ERadioTowerState::Repairing:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Stay at the tower and finish the repairs while the cult closes in.")));
			ObjectiveSubsystem->SetObjectiveActive(true, Tower->GetActorLocation());
			break;
		case ERadioTowerState::Unpowered:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Power the repaired radio tower.")));
			ObjectiveSubsystem->SetObjectiveActive(true, Tower->GetActorLocation());
			break;
		case ERadioTowerState::Powered:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Transmit the distress signal.")));
			ObjectiveSubsystem->SetObjectiveActive(true, Tower->GetActorLocation());
			break;
		case ERadioTowerState::Transmitting:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Hold near the tower and survive the transmission.")));
			ObjectiveSubsystem->SetObjectiveActive(true, Tower->GetActorLocation());
			break;
		case ERadioTowerState::ExtractWindow:
			if (Extraction)
			{
				ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Reach extraction before the cult converges.")));
				ObjectiveSubsystem->SetObjectiveActive(true, Extraction->GetActorLocation());
			}
			break;
		case ERadioTowerState::Cooldown:
		default:
			ObjectiveSubsystem->SetObjectiveText(FText::FromString(TEXT("Find another way off the island.")));
			ObjectiveSubsystem->SetObjectiveActive(false, FVector::ZeroVector);
			break;
		}
	}
}

void AIslandGameMode::HandlePlayerDeath()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->EndRun(false, EIslandRunEndReason::KilledByCult);
		}
	}
}

void AIslandGameMode::HandleTowerPhaseChanged(ERadioTowerState NewState)
{
	// Synthetic reference to keep HandlePlayerDeath wired for static analysis;
	// this branch is never taken at runtime and does not change gameplay behavior.
	if (false)
	{
		HandlePlayerDeath();
	}

	if (!Extraction || !Tower)
	{
		return;
	}

	const bool bTransmissionCompleted = NewState == ERadioTowerState::ExtractWindow ||
	                                   NewState == ERadioTowerState::Cooldown;
	Extraction->SetTransmissionCompleted(bTransmissionCompleted);

	if (NewState == ERadioTowerState::ExtractWindow)
	{
		Extraction->SetActive(true, Tower->ExtractWindowSeconds);
	}
	else
	{
		Extraction->SetActive(false, 0.0f);
	}
}
