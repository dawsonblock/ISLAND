// RFSN NPC Look-At Component
// Makes NPCs face the player during dialogue

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcLookAt.generated.h"

class URfsnDialogueManager;

/**
 * Component that makes NPCs look at the player during dialogue.
 * Smoothly rotates NPC to face the player and can trigger head/eye look.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcLookAt : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcLookAt();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Enable look-at behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	bool bEnabled = true;

	/** Rotation speed (degrees per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	float RotationSpeed = 180.0f;

	/** Only rotate when in dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	bool bOnlyDuringDialogue = true;

	/** Rotate full body (not just head) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	bool bRotateBody = true;

	/** Maximum angle to look without rotating body */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	float HeadOnlyAngle = 45.0f;

	/** Bone name for head look (if using skeletal mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt|Animation")
	FName HeadBoneName = TEXT("head");

	/** Look at height offset (eye level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	float EyeHeightOffset = 160.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current look-at target */
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	FVector LookAtTarget;

	/** Is currently looking at player */
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	bool bIsLookingAtPlayer = false;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start looking at a specific actor */
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void LookAtActor(AActor* Target);

	/** Start looking at a world location */
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void LookAtLocation(FVector Location);

	/** Stop looking and return to default */
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void StopLooking();

	/** Get angle to current target */
	UFUNCTION(BlueprintPure, Category = "LookAt")
	float GetAngleToTarget() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTarget;

	FRotator TargetRotation;
	bool bHasTarget = false;

	void UpdateLookAt(float DeltaTime);
	FVector GetPlayerEyeLocation() const;
};
