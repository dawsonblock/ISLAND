#include "CultistAIController.h"

#include "CultistCharacter.h"
#include "GameFramework/Character.h"
#include "IslandDirectorSubsystem.h"
#include "IslandLifeStateInterface.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ACultistAIController::ACultistAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACultistAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledCultist = Cast<ACultistCharacter>(InPawn);
	if (!ControlledCultist)
	{
		return;
	}

	HomeLocation = ControlledCultist->GetActorLocation();
	if (ControlledCultist->AwarenessComponent)
	{
		ControlledCultist->AwarenessComponent->OnAwarenessChanged.AddDynamic(
		    this, &ACultistAIController::OnAwarenessChanged);
		ControlledCultist->AwarenessComponent->OnTargetDetected.AddDynamic(
		    this, &ACultistAIController::OnTargetDetected);
		ControlledCultist->AwarenessComponent->OnSuspiciousSound.AddDynamic(
		    this, &ACultistAIController::OnSuspiciousSound);
	}
}

void ACultistAIController::OnUnPossess()
{
	if (ControlledCultist && ControlledCultist->AwarenessComponent)
	{
		ControlledCultist->AwarenessComponent->OnAwarenessChanged.RemoveDynamic(
		    this, &ACultistAIController::OnAwarenessChanged);
		ControlledCultist->AwarenessComponent->OnTargetDetected.RemoveDynamic(
		    this, &ACultistAIController::OnTargetDetected);
		ControlledCultist->AwarenessComponent->OnSuspiciousSound.RemoveDynamic(
		    this, &ACultistAIController::OnSuspiciousSound);
	}

	ControlledCultist = nullptr;
	Super::OnUnPossess();
}

void ACultistAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBehavior();
}

void ACultistAIController::SetCurrentTarget(AActor* Target)
{
	CurrentTarget = Target;
	ClearInvestigationLocation();
	SearchEndTime = 0.0f;
}

void ACultistAIController::ClearCurrentTarget()
{
	CurrentTarget = nullptr;
}

void ACultistAIController::SetInvestigationLocation(const FVector& Location)
{
	InvestigationLocation = Location;
	bHasInvestigationLocation = true;
	SearchEndTime = 0.0f;
	NextSearchMoveTime = 0.0f;
}

void ACultistAIController::ClearInvestigationLocation()
{
	bHasInvestigationLocation = false;
	InvestigationLocation = FVector::ZeroVector;
	SearchEndTime = 0.0f;
	NextSearchMoveTime = 0.0f;
}

void ACultistAIController::SetGuardLocation(const FVector& Location)
{
	GuardLocation = Location;
	bHasGuardLocation = true;
}

void ACultistAIController::ClearGuardLocation()
{
	bHasGuardLocation = false;
	GuardLocation = FVector::ZeroVector;
}

void ACultistAIController::OnAwarenessChanged(ERfsnAwarenessLevel NewLevel, ERfsnAwarenessLevel OldLevel)
{
	if (!ControlledCultist)
	{
		return;
	}

	switch (NewLevel)
	{
	case ERfsnAwarenessLevel::Suspicious:
		ControlledCultist->SetCultState(ECultistState::Suspicious);
		break;
	case ERfsnAwarenessLevel::Investigating:
		SetInvestigationLocation(ControlledCultist->AwarenessComponent->GetInvestigationLocation());
		break;
	case ERfsnAwarenessLevel::Alerted:
	case ERfsnAwarenessLevel::Hostile:
		if (ControlledCultist->AwarenessComponent->CurrentTarget)
		{
			SetCurrentTarget(ControlledCultist->AwarenessComponent->CurrentTarget);
			if (UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>())
			{
				Director->AddAlertFromCultSpotting(12.0f);
			}
		}
		break;
	case ERfsnAwarenessLevel::Unaware:
	default:
		if (!CurrentTarget && !bHasInvestigationLocation)
		{
			ControlledCultist->SetCultState(bHasGuardLocation ? ECultistState::GuardTower
			                                                  : ECultistState::ReturnToRoute);
		}
		break;
	}
}

void ACultistAIController::OnTargetDetected(AActor* Target)
{
	SetCurrentTarget(Target);
	if (UIslandDirectorSubsystem* Director = GetWorld()->GetSubsystem<UIslandDirectorSubsystem>())
	{
		Director->AddAlertFromCultSpotting(15.0f);
	}
}

void ACultistAIController::OnSuspiciousSound(FVector Location)
{
	if (!CurrentTarget)
	{
		SetInvestigationLocation(Location);
	}
}

void ACultistAIController::UpdateBehavior()
{
	if (!ControlledCultist || ControlledCultist->IsDead())
	{
		StopMovement();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (CurrentTarget && CurrentTarget->GetClass()->ImplementsInterface(UIslandLifeStateInterface::StaticClass()))
	{
		if (IIslandLifeStateInterface::Execute_IsDead(CurrentTarget) ||
		    IIslandLifeStateInterface::Execute_IsDowned(CurrentTarget))
		{
			ClearCurrentTarget();
		}
	}

	if (CurrentTarget)
	{
		const float DistanceToTrackedTarget =
		    FVector::Dist(CurrentTarget->GetActorLocation(), ControlledCultist->GetActorLocation());
		if (DistanceToTrackedTarget > ControlledCultist->LoseTargetDistance &&
		    ControlledCultist->AwarenessComponent &&
		    !ControlledCultist->AwarenessComponent->bCanSeeTarget)
		{
			ControlledCultist->LoseTarget();
			return;
		}

		if (ControlledCultist->AwarenessComponent &&
		    ControlledCultist->AwarenessComponent->CurrentAwareness < ERfsnAwarenessLevel::Alerted &&
		    !ControlledCultist->AwarenessComponent->bCanSeeTarget)
		{
			if (ControlledCultist->AwarenessComponent->LastKnownLocation != FVector::ZeroVector)
			{
				SetInvestigationLocation(ControlledCultist->AwarenessComponent->LastKnownLocation);
			}
			ClearCurrentTarget();
		}
	}

	if (CurrentTarget)
	{
		const float DistanceToTarget = FVector::Dist(CurrentTarget->GetActorLocation(),
		                                             ControlledCultist->GetActorLocation());
		if (DistanceToTarget <= ControlledCultist->AttackRange)
		{
			StopMovement();
			ControlledCultist->SetCultState(ECultistState::Attack);
			if (World->GetTimeSeconds() >= NextAttackTime)
			{
				ControlledCultist->PerformAttack(CurrentTarget, this);
				NextAttackTime = World->GetTimeSeconds() + ControlledCultist->AttackInterval;
			}
		}
		else
		{
			MoveToActor(CurrentTarget, ControlledCultist->AttackRange * 0.75f);
			ControlledCultist->SetCultState(ECultistState::Chase);
		}
		return;
	}

	if (bHasInvestigationLocation)
	{
		const float DistanceToInvestigation = FVector::Dist(ControlledCultist->GetActorLocation(),
		                                                    InvestigationLocation);
		if (DistanceToInvestigation <= ControlledCultist->InvestigationAcceptanceRadius)
		{
			if (SearchEndTime <= 0.0f)
			{
				const float SearchDuration =
				    GetWorld()->GetSubsystem<UIslandDirectorSubsystem>()
				        ? GetWorld()->GetSubsystem<UIslandDirectorSubsystem>()->GetCurrentSearchDuration()
				        : 6.0f;
				SearchEndTime = World->GetTimeSeconds() + SearchDuration;
			}

			ControlledCultist->SetCultState(ECultistState::Search);
			if (World->GetTimeSeconds() >= NextSearchMoveTime)
			{
				MoveAroundLocation(InvestigationLocation, ControlledCultist->PatrolRadius * 0.35f);
				NextSearchMoveTime = World->GetTimeSeconds() + 1.75f;
			}
			if (World->GetTimeSeconds() >= SearchEndTime)
			{
				ClearInvestigationLocation();
				ControlledCultist->SetCultState(bHasGuardLocation ? ECultistState::GuardTower
				                                                  : ECultistState::ReturnToRoute);
				NextPatrolMoveTime = 0.0f;
			}
		}
		else
		{
			MoveToLocation(InvestigationLocation, ControlledCultist->InvestigationAcceptanceRadius);
			ControlledCultist->SetCultState(ECultistState::Investigate);
		}
		return;
	}

	if (bHasGuardLocation)
	{
		const float DistanceToGuard = FVector::Dist(ControlledCultist->GetActorLocation(), GuardLocation);
		ControlledCultist->SetCultState(ECultistState::GuardTower);
		if (DistanceToGuard > ControlledCultist->PatrolRadius * 0.75f)
		{
			MoveToLocation(GuardLocation, ControlledCultist->InvestigationAcceptanceRadius);
		}
		else if (World->GetTimeSeconds() >= NextPatrolMoveTime)
		{
			MoveAroundLocation(GuardLocation, ControlledCultist->PatrolRadius * 0.4f);
			NextPatrolMoveTime = World->GetTimeSeconds() + 4.0f;
		}
		return;
	}

	if (World->GetTimeSeconds() >= NextPatrolMoveTime)
	{
		MoveToPatrolPoint();
		ControlledCultist->SetCultState(ECultistState::Patrol);
		NextPatrolMoveTime = World->GetTimeSeconds() + 6.0f;
	}
}

void ACultistAIController::MoveToPatrolPoint()
{
	if (!ControlledCultist)
	{
		return;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation PatrolPoint;
		if (NavSys->GetRandomReachablePointInRadius(HomeLocation, ControlledCultist->PatrolRadius, PatrolPoint))
		{
			MoveToLocation(PatrolPoint.Location, ControlledCultist->InvestigationAcceptanceRadius);
		}
	}
}

void ACultistAIController::MoveAroundLocation(const FVector& Location, float Radius)
{
	if (!ControlledCultist)
	{
		return;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation SearchPoint;
		if (NavSys->GetRandomReachablePointInRadius(Location, Radius, SearchPoint))
		{
			MoveToLocation(SearchPoint.Location, ControlledCultist->InvestigationAcceptanceRadius);
		}
	}
}
