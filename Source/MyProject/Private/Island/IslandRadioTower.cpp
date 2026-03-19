#include "IslandRadioTower.h"
#include "IslandDirectorSubsystem.h"
#include "IslandInventoryComponent.h"
#include "IslandLifeStateInterface.h"
#include "IslandNoiseLibrary.h"
#include "IslandObjectiveSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

AIslandRadioTower::AIslandRadioTower()
{
	PrimaryActorTick.bCanEverTick = true;

	State = ERadioTowerState::NeedsParts;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	TowerMesh->SetupAttachment(Root);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(TowerMesh);
	StatusLight->SetIntensity(500.0f);
	StatusLight->SetLightColor(FLinearColor::Red);

	RequiredParts = {
		EIslandItemType::TowerFuse,
		EIslandItemType::TowerFuel,
		EIslandItemType::AntennaCrank,
	};
}

void AIslandRadioTower::BeginPlay()
{
	Super::BeginPlay();

	if (RequiredParts.Num() == 0 && (State == ERadioTowerState::Broken || State == ERadioTowerState::NeedsParts))
	{
		State = ERadioTowerState::Unpowered;
	}
	else if (State == ERadioTowerState::Broken)
	{
		State = ERadioTowerState::NeedsParts;
	}

	UpdateVisuals();
}

void AIslandRadioTower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (State == ERadioTowerState::Transmitting)
	{
		// Pulse the light intensity during transmission
		float Sine = FMath::Sin(GetWorld()->GetTimeSeconds() * 10.0f);
		StatusLight->SetIntensity(1000.0f + (Sine * 500.0f));
	}

	if (bRepairInProgress)
	{
		TickRepair(DeltaTime);
	}
}

void AIslandRadioTower::SetState(ERadioTowerState NewState)
{
	if (State == NewState) return;
	
	State = NewState;
	UpdateVisuals();
	OnStateChanged.Broadcast(NewState);
}

void AIslandRadioTower::UpdateVisuals()
{
	if (!StatusLight) return;

	switch (State)
	{
	case ERadioTowerState::Broken:
	case ERadioTowerState::NeedsParts:
		StatusLight->SetIntensity(100.0f);
		StatusLight->SetLightColor(FLinearColor::Red); // Dim red flickering?
		break;
	case ERadioTowerState::Unpowered:
		StatusLight->SetIntensity(0.0f);
		break;
	case ERadioTowerState::Repairing:
		StatusLight->SetIntensity(700.0f);
		StatusLight->SetLightColor(FLinearColor(1.0f, 0.5f, 0.0f));
		break;
	case ERadioTowerState::Powered:
		StatusLight->SetIntensity(500.0f);
		StatusLight->SetLightColor(FLinearColor::Yellow);
		break;
	case ERadioTowerState::Transmitting:
		StatusLight->SetIntensity(1000.0f);
		StatusLight->SetLightColor(FLinearColor::Blue);
		break;
	case ERadioTowerState::ExtractWindow:
		StatusLight->SetIntensity(2000.0f);
		StatusLight->SetLightColor(FLinearColor::Green);
		break;
	case ERadioTowerState::Cooldown:
		StatusLight->SetIntensity(200.0f);
		StatusLight->SetLightColor(FLinearColor::Red);
		break;
	}
}

void AIslandRadioTower::PowerOn()
{
	if (State != ERadioTowerState::Unpowered) return;

	SetState(ERadioTowerState::Powered);

	if (PowerOnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PowerOnSound, GetActorLocation());
	}
}

void AIslandRadioTower::StartTransmit()
{
	if (State != ERadioTowerState::Powered) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bTransmissionCompleted = false;
	SetState(ERadioTowerState::Transmitting);
	TransmitStartTime = World->GetTimeSeconds();

	// Trigger the final pressure spike.
	if (UIslandDirectorSubsystem* Director = World->GetSubsystem<UIslandDirectorSubsystem>())
	{
		Director->AddAlertFromTransmissionPulse(25.0f);
	}

	// Activate objective marker
	if (UIslandObjectiveSubsystem* Obj = World->GetSubsystem<UIslandObjectiveSubsystem>())
	{
		Obj->SetObjectiveActive(true, GetActorLocation());
		Obj->SetObjectiveText(FText::FromString(TEXT("Hold the radio tower while the distress signal transmits.")));
	}

	// Start transmit timer
	World->GetTimerManager().SetTimer(TransmitTimer, this, &AIslandRadioTower::OnTransmitComplete, TransmitDurationSeconds, false);

	// Start pulse timer
	World->GetTimerManager().SetTimer(PulseTimer, this, &AIslandRadioTower::SendPulse, PulseInterval, true);
	SendPulse(); // Immediate first pulse
}

float AIslandRadioTower::GetTransmitProgress() const
{
	if (State != ERadioTowerState::Transmitting) return 0.0f;
	float Elapsed = GetWorld()->GetTimeSeconds() - TransmitStartTime;
	return FMath::Clamp(Elapsed / TransmitDurationSeconds, 0.0f, 1.0f);
}

void AIslandRadioTower::OnTransmitComplete()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Stop pulses
	World->GetTimerManager().ClearTimer(PulseTimer);

	// Deactivate objective
	if (UIslandObjectiveSubsystem* Obj = World->GetSubsystem<UIslandObjectiveSubsystem>())
	{
		Obj->SetObjectiveActive(false, FVector::ZeroVector);
	}

	if (TransmitCompleteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TransmitCompleteSound, GetActorLocation());
	}

	if (TransmitFinishedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TransmitFinishedEffect, GetActorLocation());
	}

	// Enter extract window
	bTransmissionCompleted = true;
	SetState(ERadioTowerState::ExtractWindow);

	// Start cooldown timer
	World->GetTimerManager().SetTimer(CooldownTimer, this, &AIslandRadioTower::OnCooldownComplete, ExtractWindowSeconds, false);
}

void AIslandRadioTower::OnCooldownComplete()
{
	SetState(ERadioTowerState::Cooldown);
}

void AIslandRadioTower::SendPulse()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Add alert spike
	if (UIslandDirectorSubsystem* Director = World->GetSubsystem<UIslandDirectorSubsystem>())
	{
		Director->AddAlertFromTransmissionPulse(10.0f);
	}

	UIslandNoiseLibrary::EmitNoise(this, GetActorLocation(), 1.5f, this);

	if (PulseEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PulseEffect, GetActorLocation());
	}

	if (PulseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PulseSound, GetActorLocation());
	}
}

bool AIslandRadioTower::CanInteract_Implementation(const FIslandInteractContext& Ctx) const
{
	APawn* Pawn = Cast<APawn>(Ctx.Interactor.Get());
	switch (State)
	{
	case ERadioTowerState::NeedsParts:
		return CanConsumeRequiredParts(Pawn);
	case ERadioTowerState::Broken:
		return bPartsInstalled || CanConsumeRequiredParts(Pawn);
	case ERadioTowerState::Unpowered:
	case ERadioTowerState::Powered:
		return true;
	default:
		return false;
	}
}

FText AIslandRadioTower::GetInteractPrompt_Implementation(const FIslandInteractContext& Ctx) const
{
	switch (State)
	{
	case ERadioTowerState::Broken:
		return bPartsInstalled ? FText::FromString(TEXT("Begin Tower Repair"))
		                       : FText::FromString(TEXT("Tower Needs Parts"));
	case ERadioTowerState::NeedsParts:
		return CanConsumeRequiredParts(Cast<APawn>(Ctx.Interactor.Get()))
		           ? FText::FromString(TEXT("Install Tower Parts"))
		           : FText::FromString(TEXT("Tower Needs Fuse, Fuel, and Crank"));
	case ERadioTowerState::Repairing: return FText::FromString(TEXT("Repairing..."));
	case ERadioTowerState::Unpowered: return FText::FromString(TEXT("Power Radio"));
	case ERadioTowerState::Powered:   return FText::FromString(TEXT("Transmit Distress Signal"));
	default:                          return FText();
	}
}

void AIslandRadioTower::Interact_Implementation(const FIslandInteractContext& Ctx)
{
	APawn* Pawn = Cast<APawn>(Ctx.Interactor.Get());

	if (State == ERadioTowerState::Broken || State == ERadioTowerState::NeedsParts)
	{
		if (!bPartsInstalled && !ConsumeRequiredParts(Pawn))
		{
			return;
		}

		BeginRepair(Pawn);
	}
	else if (State == ERadioTowerState::Unpowered)
	{
		PowerOn();
	}
	else if (State == ERadioTowerState::Powered)
	{
		StartTransmit();
	}
}

void AIslandRadioTower::Repair()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (!bPartsInstalled && !ConsumeRequiredParts(Pawn))
		{
			return;
		}

		BeginRepair(Pawn);
	}
}

bool AIslandRadioTower::CanConsumeRequiredParts(APawn* Pawn) const
{
	if (bPartsInstalled || RequiredParts.Num() == 0)
	{
		return true;
	}

	if (!Pawn)
	{
		return false;
	}

	const UIslandInventoryComponent* Inventory = Pawn->FindComponentByClass<UIslandInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	for (const EIslandItemType ItemType : RequiredParts)
	{
		if (!Inventory->HasItem(ItemType))
		{
			return false;
		}
	}

	return true;
}

bool AIslandRadioTower::ConsumeRequiredParts(APawn* Pawn)
{
	if (bPartsInstalled || RequiredParts.Num() == 0)
	{
		bPartsInstalled = true;
		return true;
	}

	if (!Pawn)
	{
		return false;
	}

	UIslandInventoryComponent* Inventory = Pawn->FindComponentByClass<UIslandInventoryComponent>();
	if (!Inventory || !CanConsumeRequiredParts(Pawn))
	{
		return false;
	}

	for (const EIslandItemType ItemType : RequiredParts)
	{
		Inventory->ConsumeItem(ItemType);
	}

	bPartsInstalled = true;
	SetState(ERadioTowerState::Broken);
	return true;
}

void AIslandRadioTower::BeginRepair(APawn* Pawn)
{
	if (!Pawn || bRepairInProgress || (State != ERadioTowerState::Broken && State != ERadioTowerState::NeedsParts))
	{
		return;
	}

	bRepairInProgress = true;
	RepairingPawn = Pawn;
	RepairNoiseTickAccumulator = 0.0f;
	SetState(ERadioTowerState::Repairing);
}

void AIslandRadioTower::CancelRepair()
{
	if (!bRepairInProgress)
	{
		return;
	}

	bRepairInProgress = false;
	RepairingPawn = nullptr;
	SetState(bPartsInstalled ? ERadioTowerState::Broken : ERadioTowerState::NeedsParts);
}

void AIslandRadioTower::CompleteRepair()
{
	bRepairInProgress = false;
	RepairingPawn = nullptr;
	RepairProgress = 1.0f;
	SetState(ERadioTowerState::Unpowered);
}

void AIslandRadioTower::TickRepair(float DeltaTime)
{
	APawn* Pawn = RepairingPawn.Get();
	if (!Pawn)
	{
		CancelRepair();
		return;
	}

	if (Pawn->GetClass()->ImplementsInterface(UIslandLifeStateInterface::StaticClass()))
	{
		if (IIslandLifeStateInterface::Execute_IsDead(Pawn) ||
		    IIslandLifeStateInterface::Execute_IsDowned(Pawn))
		{
			CancelRepair();
			return;
		}
	}

	if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) >
	    FMath::Square(RepairCancelDistance))
	{
		CancelRepair();
		return;
	}

	RepairProgress = FMath::Clamp(RepairProgress + (DeltaTime / FMath::Max(RequiredRepairTime, 0.1f)), 0.0f, 1.0f);
	if (UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>())
	{
		Director->AddAlertFromTowerRepair(RepairNoisePerSecond * DeltaTime);
	}

	RepairNoiseTickAccumulator += DeltaTime;
	if (RepairNoiseTickAccumulator >= 0.25f)
	{
		RepairNoiseTickAccumulator = 0.0f;
		UIslandNoiseLibrary::EmitNoise(this, GetActorLocation(),
		                               FMath::Clamp(RepairNoisePerSecond / 8.0f, 0.6f, 2.0f), this);
	}

	if (RepairProgress >= 1.0f)
	{
		CompleteRepair();
	}
}
