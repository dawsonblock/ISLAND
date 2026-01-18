#include "IslandGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "IslandRunSaveGame.h"

void UIslandGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreate();
}

void UIslandGameInstanceSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		Save = Cast<UIslandRunSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	}
	if (!Save)
	{
		Save = Cast<UIslandRunSaveGame>(UGameplayStatics::CreateSaveGameObject(UIslandRunSaveGame::StaticClass()));
	}
}

void UIslandGameInstanceSubsystem::SaveToDisk()
{
	if (Save)
	{
		UGameplayStatics::SaveGameToSlot(Save, SlotName, UserIndex);
	}
}

void UIslandGameInstanceSubsystem::StartRun(int32 Seed)
{
	RunTimeSeconds = 0.0f;

	if (Seed == 0)
	{
		Seed = FMath::Rand();
	}
	CurrentSeed = Seed;

	if (!Save) LoadOrCreate();
	Save->LastRunSeed = Seed;
	SaveToDisk();
}

void UIslandGameInstanceSubsystem::EndRun(bool bEscaped)
{
	if (!Save) LoadOrCreate();

	Save->TotalRuns += 1;

	if (bEscaped)
	{
		Save->Escapes += 1;
		Save->BestEscapeTimeSeconds = FMath::Min(Save->BestEscapeTimeSeconds, RunTimeSeconds);
		UnlockClue(TEXT("Clue_RadioEscape"));
	}
	else
	{
		UnlockClue(TEXT("Clue_TheIslandKills"));
	}

	SaveToDisk();

	// Simple: restart current level
	if (UWorld* World = GetWorld())
	{
		const FName LevelName(*World->GetName());
		UGameplayStatics::OpenLevel(World, LevelName);
	}
}

void UIslandGameInstanceSubsystem::UnlockClue(FName ClueId)
{
	if (!Save) return;
	if (!Save->UnlockedClues.Contains(ClueId))
	{
		Save->UnlockedClues.Add(ClueId);
	}
}
