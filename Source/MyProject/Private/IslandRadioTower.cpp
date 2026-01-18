#include "IslandRadioTower.h"
#include "IslandDirectorSubsystem.h"
#include "IslandObjectiveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AIslandRadioTower::AIslandRadioTower()
{
	PrimaryActorTick.bCanEverTick = true;

	State = ERadioTowerState::Broken;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	TowerMesh->SetupAttachment(Root);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(TowerMesh);
	StatusLight->SetIntensity(500.0f);
	StatusLight->SetLightColor(FLinearColor::Red);
}

void AIslandRadioTower::BeginPlay()
{
	Super::BeginPlay();
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
		StatusLight->SetIntensity(100.0f);
		StatusLight->SetLightColor(FLinearColor::Red); // Dim red flickering?
		break;
	case ERadioTowerState::Unpowered:
		StatusLight->SetIntensity(0.0f);
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

	UWorld* World = GetWorld();
	if (!World) return;

	UIslandDirectorSubsystem* Director = World->GetSubsystem<UIslandDirectorSubsystem>();
	if (!Director || !Director->CanUseTower()) return;

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

	UIslandDirectorSubsystem* Director = World->GetSubsystem<UIslandDirectorSubsystem>();
	if (!Director || !Director->CanTransmit()) return;

	SetState(ERadioTowerState::Transmitting);
	TransmitStartTime = World->GetTimeSeconds();

	// Alert the director heavily
	if (Director)
	{
		Director->AddAlert(40.0f);
	}

	// Activate objective marker
	if (UIslandObjectiveSubsystem* Obj = World->GetSubsystem<UIslandObjectiveSubsystem>())
	{
		Obj->SetObjectiveActive(true, GetActorLocation());
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
		Director->AddAlert(10.0f);
	}

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
	// Allow interaction if broken, unpowered or powered
	return State == ERadioTowerState::Broken || State == ERadioTowerState::Unpowered || State == ERadioTowerState::Powered;
}

FText AIslandRadioTower::GetInteractPrompt_Implementation(const FIslandInteractContext& Ctx) const
{
	switch (State)
	{
	case ERadioTowerState::Broken:    return FText::FromString("Repair Radio Tower");
	case ERadioTowerState::Unpowered: return FText::FromString("Power Radio");
	case ERadioTowerState::Powered:   return FText::FromString("Transmit Distress Signal");
	default:                          return FText();
	}
}

void AIslandRadioTower::Interact_Implementation(const FIslandInteractContext& Ctx)
{
	if (State == ERadioTowerState::Broken)
	{
		Repair();
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
	// In a full implementation, we might want a localized progress bar.
	// For now, we'll just simulate a repair time or instant repair for this slice.
	SetState(ERadioTowerState::Unpowered);
}
