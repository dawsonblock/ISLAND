#include "IslandExtractionZone.h"
#include "IslandLifeStateInterface.h"
#include "IslandGameInstanceSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AIslandExtractionZone::AIslandExtractionZone()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ExtractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionVolume"));
	ExtractionVolume->SetupAttachment(Root);
	ExtractionVolume->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	ExtractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AIslandExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	ExtractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AIslandExtractionZone::OnVolumeBeginOverlap);
	ExtractionVolume->OnComponentEndOverlap.AddDynamic(this, &AIslandExtractionZone::OnVolumeEndOverlap);
}

void AIslandExtractionZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bActive) return;

	// Check if we timed out
	if (GetWorld()->GetTimeSeconds() > ActiveUntilTime)
	{
		SetActive(false, 0.0f);
		return;
	}

	// Update hold timers
	TArray<TObjectPtr<APawn>> ToRemove;
	for (auto& Pair : HoldTimers)
	{
		APawn* Pawn = Pair.Key;
		if (!Pawn || !IsPawnEligible(Pawn))
		{
			ToRemove.Add(Pawn);
			continue;
		}

		Pair.Value += DeltaTime;
		if (Pair.Value >= HoldTimeSeconds)
		{
			TriggerWin(Pawn);
			ToRemove.Add(Pawn);
		}
	}

	for (APawn* Pawn : ToRemove)
	{
		HoldTimers.Remove(Pawn);
	}
}

void AIslandExtractionZone::SetActive(bool bInActive, float WindowSeconds)
{
	if (bActive == bInActive) return;

	bActive = bInActive;
	ActiveUntilTime = bActive ? GetWorld()->GetTimeSeconds() + WindowSeconds : 0.0f;
	
	if (bActive)
	{
		if (ActiveEffect)
		{
			ActiveNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ActiveEffect, GetActorLocation());
		}
		if (ActiveLoopSound)
		{
			ActiveAudioComp = UGameplayStatics::SpawnSoundAtLocation(this, ActiveLoopSound, GetActorLocation());
		}
	}
	else
	{
		HoldTimers.Empty();
		
		if (ActiveNiagaraComp)
		{
			ActiveNiagaraComp->Deactivate();
			ActiveNiagaraComp = nullptr;
		}
		if (ActiveAudioComp)
		{
			ActiveAudioComp->Stop();
			ActiveAudioComp = nullptr;
		}
	}
}

float AIslandExtractionZone::GetRemainingSeconds() const
{
	if (!bActive) return 0.0f;
	return FMath::Max(0.0f, ActiveUntilTime - GetWorld()->GetTimeSeconds());
}

float AIslandExtractionZone::GetHoldProgress(APawn* Pawn) const
{
	if (!HoldTimers.Contains(Pawn)) return 0.0f;
	return FMath::Clamp(HoldTimers[Pawn] / HoldTimeSeconds, 0.0f, 1.0f);
}

bool AIslandExtractionZone::IsPawnEligible(APawn* Pawn) const
{
	if (!Pawn || !Pawn->IsPlayerControlled())
		return false;

	if (Pawn->GetClass()->ImplementsInterface(UIslandLifeStateInterface::StaticClass()))
	{
		const bool bDead = IIslandLifeStateInterface::Execute_IsDead(Pawn);
		const bool bDowned = IIslandLifeStateInterface::Execute_IsDowned(Pawn);
		return !bDead && !bDowned;
	}

	return true; // debug-friendly
}

void AIslandExtractionZone::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bActive) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !IsPawnEligible(Pawn)) return;

	if (!HoldTimers.Contains(Pawn))
	{
		HoldTimers.Add(Pawn, 0.0f);
	}
}

void AIslandExtractionZone::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn)
	{
		HoldTimers.Remove(Pawn);
	}
}

void AIslandExtractionZone::TriggerWin(APawn* Pawn)
{
	if (SuccessSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SuccessSound, GetActorLocation());
	}
	if (successEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, successEffect, GetActorLocation());
	}

	SetActive(false, 0.0f);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UIslandGameInstanceSubsystem* Run = GI->GetSubsystem<UIslandGameInstanceSubsystem>())
		{
			Run->EndRun(true);
		}
	}
}
