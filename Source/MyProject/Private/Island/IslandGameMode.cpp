#include "Island/IslandGameMode.h"
#include "Core/MyProjectCharacter.h"
#include "Core/MyProjectPlayerController.h"
#include "EngineUtils.h"
#include "Island/IslandAISpawnManager.h"
#include "Island/IslandExtractionZone.h"
#include "Island/IslandHUD.h"
#include "Island/IslandObjectiveSubsystem.h"
#include "Kismet/GameplayStatics.h"

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

	ObjectiveSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UIslandObjectiveSubsystem>() : nullptr;

	TryAutoFindActors();
	ValidateRunRequirements();

	if (!bRuntimeValid)
	{
		HandleRunInvalid();
		return;
	}

	BindRuntimeActors();
	UpdateObjectiveText();
	StartRun();

	if (bShowWelcomeTutorial)
	{
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

	if (bRuntimeValid && !bRunEnded)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
			{
				Run->RunTimeSeconds += DeltaSeconds;
			}
		}
	}

	if (!bRuntimeValid && !bHasRetriedRuntimeDiscovery)
	{
		bHasRetriedRuntimeDiscovery = true;
		TryAutoFindActors();
		ValidateRunRequirements();
		if (bRuntimeValid)
		{
			BindRuntimeActors();
			UpdateObjectiveText();
			StartRun();
		}
		else
		{
			HandleRunInvalid();
		}
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

void AIslandGameMode::ValidateRunRequirements()
{
	RuntimeValidationError.Empty();

	const auto AppendMissing = [this](const TCHAR* Label)
	{
		if (!RuntimeValidationError.IsEmpty())
		{
			RuntimeValidationError += TEXT(", ");
		}
		RuntimeValidationError += Label;
	};

	if (!Tower)
	{
		AppendMissing(TEXT("IslandRadioTower"));
	}
	if (!Extraction)
	{
		AppendMissing(TEXT("IslandExtractionZone"));
	}
	if (!SpawnManager)
	{
		AppendMissing(TEXT("IslandAISpawnManager"));
	}

	bRuntimeValid = RuntimeValidationError.IsEmpty();

	if (!bRuntimeValid)
	{
		RuntimeValidationError = FString::Printf(TEXT("Island runtime missing required actors: %s"), *RuntimeValidationError);
		UE_LOG(LogTemp, Error, TEXT("%s"), *RuntimeValidationError);
	}
}

void AIslandGameMode::BindRuntimeActors()
{
	if (!bRuntimeValid || bRuntimeActorsBound)
	{
		return;
	}

	if (Tower)
	{
		Tower->OnStateChanged.RemoveDynamic(this, &AIslandGameMode::HandleTowerStateChanged);
		Tower->OnStateChanged.AddDynamic(this, &AIslandGameMode::HandleTowerStateChanged);
		HandleTowerPhaseChanged(Tower->State);
	}

	if (Extraction)
	{
		Extraction->OnExtractionSuccess.RemoveDynamic(this, &AIslandGameMode::HandleExtractionSuccess);
		Extraction->OnExtractionSuccess.AddDynamic(this, &AIslandGameMode::HandleExtractionSuccess);
	}

	bRuntimeActorsBound = true;
}

void AIslandGameMode::HandleRunInvalid()
{
	if (bHandledInvalidRun)
	{
		return;
	}

	bHandledInvalidRun = true;

	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->SetObjectiveText(FText::FromString(RuntimeValidationError));
		ObjectiveSubsystem->SetObjectiveActive(false, FVector::ZeroVector);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AIslandHUD* MyHUD = Cast<AIslandHUD>(PC->GetHUD()))
		{
			MyHUD->ShowTutorialMessage(RuntimeValidationError, 10.0f);
		}
	}
}

void AIslandGameMode::StartRun()
{
	if (bRunStarted)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->StartRun(0);
			bRunStarted = true;
		}
	}
}

void AIslandGameMode::HandleTowerStateChanged(ERadioTowerState NewState)
{
	if (bRunEnded)
	{
		return;
	}

	HandleTowerPhaseChanged(NewState);
	UpdateObjectiveText();
}

void AIslandGameMode::HandleExtractionSuccess()
{
	if (bRunEnded)
	{
		return;
	}

	bRunEnded = true;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->EndRun(true, EIslandRunEndReason::Escaped);
		}
	}
}

void AIslandGameMode::UpdateObjectiveText()
{
	if (!Tower || !ObjectiveSubsystem)
	{
		return;
	}

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

void AIslandGameMode::HandlePlayerDeath(EIslandRunEndReason Reason)
{
	if (bRunEnded)
	{
		return;
	}

	bRunEnded = true;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->EndRun(false, Reason);
		}
	}
}

void AIslandGameMode::HandleTowerPhaseChanged(ERadioTowerState NewState)
{
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
