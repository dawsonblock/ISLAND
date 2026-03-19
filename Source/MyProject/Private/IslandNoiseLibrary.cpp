#include "IslandNoiseLibrary.h"

#include "CultistCharacter.h"
#include "EngineUtils.h"

void UIslandNoiseLibrary::EmitNoise(UObject* WorldContextObject, FVector Location, float Loudness, AActor* Source)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ACultistCharacter> It(World); It; ++It)
	{
		ACultistCharacter* Cultist = *It;
		if (Cultist)
		{
			Cultist->HearNoiseAt(Location, Loudness, Source);
		}
	}
}
