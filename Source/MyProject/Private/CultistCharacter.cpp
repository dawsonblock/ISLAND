#include "CultistCharacter.h"

#include "CultistAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IslandVitalityComponent.h"
#include "Kismet/GameplayStatics.h"

ACultistCharacter::ACultistCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AwarenessComponent = CreateDefaultSubobject<URfsnNpcAwareness>(TEXT("AwarenessComponent"));
	VitalityComponent = CreateDefaultSubobject<UIslandVitalityComponent>(TEXT("VitalityComponent"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ACultistAIController::StaticClass();

	GetCharacterMovement()->MaxWalkSpeed = 325.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
}

void ACultistCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (VitalityComponent)
	{
		VitalityComponent->OnDeath.AddDynamic(this, &ACultistCharacter::HandleDeath);
	}

	if (AwarenessComponent)
	{
		AwarenessComponent->OnAwarenessChanged.AddDynamic(this, &ACultistCharacter::OnAwarenessChanged);
		if (AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			AwarenessComponent->CurrentTarget = PlayerPawn;
			LastKnownPlayerLocation = PlayerPawn->GetActorLocation();
		}
	}
}

void ACultistCharacter::SetCultState(ECultistState NewState)
{
	if (CurrentState != ECultistState::Dead)
	{
		CurrentState = NewState;
	}
}

void ACultistCharacter::HearNoiseAt(const FVector& Location, float Loudness, AActor* Source)
{
	if (IsDead() || !AwarenessComponent)
	{
		return;
	}

	AwarenessComponent->ReportSound(Location, Loudness, Source);
	LastKnownPlayerLocation = Location;
}

void ACultistCharacter::ForceInvestigateLocation(const FVector& Location)
{
	if (IsDead())
	{
		return;
	}

	LastKnownPlayerLocation = Location;
	if (ACultistAIController* CultistController = Cast<ACultistAIController>(GetController()))
	{
		CultistController->SetInvestigationLocation(Location);
	}
	SetCultState(ECultistState::Investigate);
}

void ACultistCharacter::StartChase(AActor* Target)
{
	if (IsDead() || !Target)
	{
		return;
	}

	LastKnownPlayerLocation = Target->GetActorLocation();
	if (AwarenessComponent)
	{
		AwarenessComponent->AlertToTarget(Target);
	}

	if (ACultistAIController* CultistController = Cast<ACultistAIController>(GetController()))
	{
		CultistController->SetCurrentTarget(Target);
	}

	SetCultState(ECultistState::Chase);
}

void ACultistCharacter::LoseTarget()
{
	if (IsDead())
	{
		return;
	}

	if (ACultistAIController* CultistController = Cast<ACultistAIController>(GetController()))
	{
		CultistController->ClearCurrentTarget();
		if (LastKnownPlayerLocation != FVector::ZeroVector)
		{
			CultistController->SetInvestigationLocation(LastKnownPlayerLocation);
		}
	}

	SetCultState(ECultistState::Search);
}

void ACultistCharacter::HandleDeath(bool bFatal)
{
	if (CurrentState == ECultistState::Dead)
	{
		return;
	}

	CurrentState = ECultistState::Dead;
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	DetachFromControllerPendingDestroy();
	OnCultistDied.Broadcast(this);
	SetLifeSpan(10.0f);
}

void ACultistCharacter::OnAwarenessChanged(ERfsnAwarenessLevel NewLevel, ERfsnAwarenessLevel OldLevel)
{
	if (!AwarenessComponent)
	{
		return;
	}

	if (AwarenessComponent->CurrentTarget)
	{
		LastKnownPlayerLocation = AwarenessComponent->CurrentTarget->GetActorLocation();
	}
	else if (AwarenessComponent->LastKnownLocation != FVector::ZeroVector)
	{
		LastKnownPlayerLocation = AwarenessComponent->LastKnownLocation;
	}
}
