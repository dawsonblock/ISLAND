#include "IslandGameMode.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "IslandExtractionZone.h"
#include "IslandGameInstanceSubsystem.h"
#include "IslandHUD.h"

AIslandGameMode::AIslandGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDClass = AIslandHUD::StaticClass();
}

void AIslandGameMode::BeginPlay()
{
	Super::BeginPlay();

	TryAutoFindActors();

	if (Tower)
	{
		Tower->OnStateChanged.AddDynamic(this, &AIslandGameMode::OnTowerStateChanged);
	}

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
					MyHUD->ShowTutorialMessage(TEXT("Welcome to the Island.\nAvoid the entities.\nLocate the Radio Tower to escape."), 8.0f);
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
}

void AIslandGameMode::OnTowerStateChanged(ERadioTowerState NewState)
{
	if (!Extraction || !Tower) return;

	if (NewState == ERadioTowerState::ExtractWindow)
	{
		Extraction->SetActive(true, Tower->ExtractWindowSeconds);
	}
	else
	{
		Extraction->SetActive(false, 0.0f);
	}
}
