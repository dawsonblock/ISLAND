#include "IslandVitalityComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"

UIslandVitalityComponent::UIslandVitalityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
	CurrentHunger = MaxHunger;
}

void UIslandVitalityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
	CurrentHunger = MaxHunger;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UIslandVitalityComponent::HandleTakeAnyDamage);
	}
}

void UIslandVitalityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentHealth <= 0.0f) return;

	UpdateHunger(DeltaTime);
	UpdateStamina(DeltaTime);
}

void UIslandVitalityComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage > 0.0f)
	{
		ModifyHealth(-Damage);
	}
}

void UIslandVitalityComponent::ModifyHealth(float Change)
{
	CurrentHealth = FMath::Clamp(CurrentHealth + Change, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth);

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast(true);
	}
}

void UIslandVitalityComponent::ModifyStamina(float Change)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + Change, 0.0f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina);

	if (Change < 0.0f)
	{
		TimeSinceLastStaminaUse = 0.0f;
	}
}

void UIslandVitalityComponent::ModifyHunger(float Change)
{
	CurrentHunger = FMath::Clamp(CurrentHunger + Change, 0.0f, MaxHunger);
	OnHungerChanged.Broadcast(CurrentHunger);
}

void UIslandVitalityComponent::UpdateHunger(float DeltaTime)
{
	if (CurrentHunger > 0.0f)
	{
		ModifyHunger(-HungerDecayRate * DeltaTime);
	}
	else
	{
		// Damage health when starving
		ModifyHealth(-HungerDamageAmount * DeltaTime);
	}
}

void UIslandVitalityComponent::UpdateStamina(float DeltaTime)
{
	TimeSinceLastStaminaUse += DeltaTime;

	if (CurrentStamina < MaxStamina && 
		TimeSinceLastStaminaUse >= StaminaRegenDelay && 
		CurrentHunger > 0.0f) 
	{
		ModifyStamina(StaminaRegenRate * DeltaTime);
	}
}
