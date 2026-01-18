#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandInteractableInterface.h"
#include "NiagaraSystem.h"
#include "IslandRadioTower.generated.h"

class UPointLightComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ERadioTowerState : uint8
{
	Broken,
	Unpowered,
	Powered,
	Transmitting,
	ExtractWindow,
	Cooldown
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRadioTowerStateChanged, ERadioTowerState, NewState);

UCLASS()
class MYPROJECT_API AIslandRadioTower : public AActor, public IIslandInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AIslandRadioTower();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> TowerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(BlueprintReadOnly, Category="State")
	ERadioTowerState State = ERadioTowerState::Unpowered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float TransmitDurationSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float ExtractWindowSeconds = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float CooldownSeconds = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float RequiredRepairTime = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category="State")
	float RepairProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float PulseInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TObjectPtr<UNiagaraSystem> PulseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TObjectPtr<UNiagaraSystem> TransmitFinishedEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> PowerOnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> PulseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> TransmitCompleteSound;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FRadioTowerStateChanged OnStateChanged;

	UFUNCTION(BlueprintCallable, Category="Tower")
	void PowerOn();

	UFUNCTION(BlueprintCallable, Category="Tower")
	void StartTransmit();

	UFUNCTION(BlueprintCallable, Category="Tower")
	void Repair();

	UFUNCTION(BlueprintPure, Category="Tower")
	float GetTransmitProgress() const;

	// IIslandInteractableInterface
	virtual bool CanInteract_Implementation(const FIslandInteractContext& Ctx) const override;
	virtual void Interact_Implementation(const FIslandInteractContext& Ctx) override;
	virtual FText GetInteractPrompt_Implementation(const FIslandInteractContext& Ctx) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	FTimerHandle TransmitTimer;
	FTimerHandle PulseTimer;
	FTimerHandle CooldownTimer;

	float TransmitStartTime;

	void SetState(ERadioTowerState NewState);
	void OnTransmitComplete();
	void OnCooldownComplete();
	void SendPulse();
	void UpdateVisuals();
};
