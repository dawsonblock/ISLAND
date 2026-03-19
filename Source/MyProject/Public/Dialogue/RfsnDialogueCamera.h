// RFSN Dialogue Camera
// Optional camera close-up during NPC conversations

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnDialogueCamera.generated.h"

class UCameraComponent;
class URfsnDialogueManager;

UENUM(BlueprintType)
enum class ERfsnCameraMode : uint8
{
	/** Keep player's normal camera */
	None,
	/** Focus on NPC */
	FocusNpc,
	/** Over-the-shoulder of player looking at NPC */
	OverShoulder,
	/** Side angle showing both */
	TwoShot
};

/**
 * Component that provides cinematic camera angles during dialogue.
 * Attach to player or use as a world actor.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnDialogueCamera : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnDialogueCamera();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Enable dialogue camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnabled = true;

	/** Default camera mode for dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	ERfsnCameraMode DefaultMode = ERfsnCameraMode::OverShoulder;

	/** Blend time to dialogue camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendInTime = 0.5f;

	/** Blend time back to player camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendOutTime = 0.3f;

	/** Distance from NPC for focus mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float FocusDistance = 150.0f;

	/** Height offset for focus camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float FocusHeightOffset = 20.0f;

	/** Shoulder offset for over-shoulder mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|OverShoulder")
	FVector ShoulderOffset = FVector(-100.0f, 50.0f, 20.0f);

	/** FOV during dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DialogueFOV = 70.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bDialogueCameraActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	ERfsnCameraMode CurrentMode = ERfsnCameraMode::None;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start dialogue camera for NPC */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void StartDialogueCamera(AActor* NpcActor, ERfsnCameraMode Mode);

	/** Stop dialogue camera and return to player view */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void StopDialogueCamera();

	/** Switch camera mode during dialogue */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraMode(ERfsnCameraMode NewMode);

	/** Get calculated camera transform for current mode */
	UFUNCTION(BlueprintPure, Category = "Camera")
	FTransform GetDialogueCameraTransform() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> DialogueNpc;

	UPROPERTY()
	TObjectPtr<UCameraComponent> DialogueCameraComponent;

	void UpdateCameraPosition();
	FTransform CalculateFocusTransform() const;
	FTransform CalculateOverShoulderTransform() const;
	FTransform CalculateTwoShotTransform() const;
};
