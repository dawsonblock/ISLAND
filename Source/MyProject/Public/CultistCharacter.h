#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RfsnNpcAwareness.h"
#include "CultistCharacter.generated.h"

class UIslandVitalityComponent;
class URfsnNpcAwareness;

UENUM(BlueprintType)
enum class ECultistState : uint8
{
	Idle,
	Patrol,
	Investigate,
	Suspicious,
	Search,
	Chase,
	Attack,
	GuardTower,
	ReturnToRoute,
	Dead
};

class ACultistCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCultistDeathSignature, ACultistCharacter*, Cultist);

UCLASS()
class MYPROJECT_API ACultistCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACultistCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cult")
	TObjectPtr<URfsnNpcAwareness> AwarenessComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cult")
	TObjectPtr<UIslandVitalityComponent> VitalityComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Cult")
	ECultistState CurrentState = ECultistState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Cult")
	FVector LastKnownPlayerLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackInterval = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float PatrolRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float InvestigationAcceptanceRadius = 120.0f;

	UPROPERTY(BlueprintAssignable, Category = "Cult")
	FCultistDeathSignature OnCultistDied;

	UFUNCTION(BlueprintCallable, Category = "Cult")
	void SetCultState(ECultistState NewState);

	UFUNCTION(BlueprintCallable, Category = "Cult")
	void HearNoiseAt(const FVector& Location, float Loudness, AActor* Source);

	UFUNCTION(BlueprintCallable, Category = "Cult")
	void ForceInvestigateLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Cult")
	void StartChase(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Cult")
	void LoseTarget();

	UFUNCTION()
	void HandleDeath(bool bFatal);

	UFUNCTION(BlueprintPure, Category = "Cult")
	bool IsDead() const { return CurrentState == ECultistState::Dead; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnAwarenessChanged(ERfsnAwarenessLevel NewLevel, ERfsnAwarenessLevel OldLevel);
};
