#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RfsnNpcAwareness.h"
#include "CultistAIController.generated.h"

class ACultistCharacter;

UCLASS()
class MYPROJECT_API ACultistAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACultistAIController();

	UPROPERTY(BlueprintReadOnly, Category = "Cult")
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Cult")
	FVector InvestigationLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Cult")
	bool bHasInvestigationLocation = false;

	void SetCurrentTarget(AActor* Target);
	void ClearCurrentTarget();
	void SetInvestigationLocation(const FVector& Location);
	void ClearInvestigationLocation();
	void SetGuardLocation(const FVector& Location);
	void ClearGuardLocation();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<ACultistCharacter> ControlledCultist;

	FVector HomeLocation = FVector::ZeroVector;
	FVector GuardLocation = FVector::ZeroVector;
	float SearchEndTime = 0.0f;
	float NextPatrolMoveTime = 0.0f;
	float NextAttackTime = 0.0f;
	float NextSearchMoveTime = 0.0f;
	bool bHasGuardLocation = false;

	UFUNCTION()
	void OnAwarenessChanged(ERfsnAwarenessLevel NewLevel, ERfsnAwarenessLevel OldLevel);

	UFUNCTION()
	void OnTargetDetected(AActor* Target);

	UFUNCTION()
	void OnSuspiciousSound(FVector Location);

	void UpdateBehavior();
	void MoveToPatrolPoint();
	void MoveAroundLocation(const FVector& Location, float Radius);
};
