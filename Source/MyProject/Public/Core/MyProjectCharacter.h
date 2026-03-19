// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Island/IslandLifeStateInterface.h"
#include "Logging/LogMacros.h"
#include "MyProjectCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UIslandInteractorComponent;
class UIslandInventoryComponent;
class UIslandStealthComponent;
class UIslandVitalityComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(config = Game, Blueprintable)
class MYPROJECT_API AMyProjectCharacter : public ACharacter, public IIslandLifeStateInterface
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* MouseLookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* SprintAction;

	/** Dialogue/Talk Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* DialogueAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* FlashlightAction;

	/** Vitality Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitality", meta = (AllowPrivateAccess = "true"))
	UIslandVitalityComponent* VitalityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Island", meta = (AllowPrivateAccess = "true"))
	UIslandInteractorComponent* InteractorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Island", meta = (AllowPrivateAccess = "true"))
	UIslandInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Island", meta = (AllowPrivateAccess = "true"))
	UIslandStealthComponent* StealthComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Life")
	bool bDowned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Life")
	bool bDead = false;

public:
	AMyProjectCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual bool IsDowned_Implementation() const override;
	virtual bool IsDead_Implementation() const override;

protected:
	virtual void BeginPlay() override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** Handles sprint start inputs */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintStart();

	/** Handles sprint end inputs */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintEnd();

	/** Try to start dialogue with nearest NPC */
	UFUNCTION(BlueprintCallable, Category = "RFSN")
	virtual void TryStartDialogue();

	UFUNCTION()
	void HandleDeath(bool bIsFatal);

	UFUNCTION(BlueprintCallable, Category = "Island")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Island")
	void ToggleFlashlight();

	void EmitMovementNoise(float DeltaTime);
	float GetCurrentNoiseLoudness() const;

protected:
	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Island")
	UIslandInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

	UFUNCTION(BlueprintPure, Category = "Island")
	UIslandInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Island")
	UIslandStealthComponent* GetStealthComponent() const { return StealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Vitality")
	UIslandVitalityComponent* GetVitalityComponent() const { return VitalityComponent; }

private:
	bool bIsSprinting = false;
	float MovementNoiseCooldown = 0.0f;
};
