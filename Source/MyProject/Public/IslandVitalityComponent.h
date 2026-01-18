#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IslandVitalityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalityChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalityDepleted, bool, bIsFatal);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UIslandVitalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIslandVitalityComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float MaxHunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float HungerDecayRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float HungerDamageAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float StaminaRegenRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitality")
	float StaminaRegenDelay = 2.0f;

	UPROPERTY(BlueprintAssignable, Category = "Vitality")
	FOnVitalityChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitality")
	FOnVitalityChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitality")
	FOnVitalityChanged OnHungerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitality")
	FOnVitalityDepleted OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Vitality")
	void ModifyHealth(float Change);

	UFUNCTION(BlueprintCallable, Category = "Vitality")
	void ModifyStamina(float Change);

	UFUNCTION(BlueprintCallable, Category = "Vitality")
	void ModifyHunger(float Change);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Vitality")
	float GetHealthNormalized() const { return CurrentHealth / MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Vitality")
	float GetStaminaNormalized() const { return CurrentStamina / MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Vitality")
	float GetHungerNormalized() const { return CurrentHunger / MaxHunger; }

private:
	float CurrentHealth;
	float CurrentStamina;
	float CurrentHunger;
	float TimeSinceLastStaminaUse = 0.0f;

	void UpdateHunger(float DeltaTime);
	void UpdateStamina(float DeltaTime);
};
