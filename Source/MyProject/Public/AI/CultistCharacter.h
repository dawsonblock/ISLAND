#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI/RfsnNpcAwareness.h"
#include "CultistCharacter.generated.h"

class UIslandVitalityComponent;
class URfsnNpcAwareness;
class UAnimMontage;

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

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;

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
	float AttackRadius = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackInterval = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackLungeDistance = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackLungeStrength = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float AttackFacingDotThreshold = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float PatrolRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float InvestigationAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float PatrolMoveSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float InvestigateMoveSpeed = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float ChaseMoveSpeed = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float GuardMoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult")
	float LoseTargetDistance = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cult|Animation")
	float AttackImpactDelay = 0.18f;

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

	UFUNCTION(BlueprintCallable, Category = "Cult")
	bool PerformAttack(AActor* Target, AController* InstigatorController);

	UFUNCTION(BlueprintCallable, Category = "Cult|Animation")
	void TriggerPendingAttackImpact();

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnCultStateChanged(ECultistState NewState, ECultistState OldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnAttackStarted(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnAttackConnected(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnAttackMissed(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnDamagedByActor(float Damage, AActor* DamageCauser, AController* EventInstigator);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cult")
	void OnDeathStarted(bool bFatal);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnAwarenessChanged(ERfsnAwarenessLevel NewLevel, ERfsnAwarenessLevel OldLevel);

	bool CanAttackTarget(AActor* Target) const;
	void ClearPendingAttack();

	FTimerHandle PendingAttackImpactTimer;

	UPROPERTY()
	TObjectPtr<AActor> PendingAttackTarget;

	UPROPERTY()
	TObjectPtr<AController> PendingAttackInstigator;

	bool bPendingAttackResolved = false;
};
