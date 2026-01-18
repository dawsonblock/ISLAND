#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "IslandExtractionZone.generated.h"

class USoundBase;

UCLASS()
class MYPROJECT_API AIslandExtractionZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AIslandExtractionZone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class UBoxComponent> ExtractionVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	float HoldTimeSeconds = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category="State")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category="State")
	float ActiveUntilTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TObjectPtr<UNiagaraSystem> ActiveEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TObjectPtr<UNiagaraSystem> successEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> ActiveLoopSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> SuccessSound;

	UFUNCTION(BlueprintCallable, Category="Extraction")
	void SetActive(bool bInActive, float WindowSeconds);

	UFUNCTION(BlueprintCallable, Category="Extraction")
	float GetRemainingSeconds() const;

	UFUNCTION(BlueprintCallable, Category="Extraction")
	bool IsPawnEligible(APawn* Pawn) const;

	UFUNCTION(BlueprintPure, Category="Extraction")
	float GetHoldProgress(APawn* Pawn) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TMap<TObjectPtr<APawn>, float> HoldTimers;

	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> ActiveNiagaraComp;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ActiveAudioComp;

	void TriggerWin(APawn* Pawn);
};
