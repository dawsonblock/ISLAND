#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IslandHUDWidget.generated.h"

USTRUCT(BlueprintType)
struct FIslandHUDPartState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bCollected = false;
};

USTRUCT(BlueprintType)
struct FIslandHUDState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText ObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float HealthNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float StaminaNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float HungerNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bHasVitality = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float ThreatPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText RadioStateText;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float RepairProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bShowRepairProgress = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float TransmitProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bShowTransmitProgress = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float NoisePercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float VisibilityPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bFlashlightOn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bExtractionActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float ExtractionRemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	float ExtractionHoldProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText InteractPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bShowTutorial = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText TutorialMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	bool bShowDialogue = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText DialogueSpeaker;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FText DialogueSentence;

	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	TArray<FIslandHUDPartState> TowerParts;
};

UCLASS(Blueprintable)
class MYPROJECT_API UIslandHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Island HUD")
	FIslandHUDState CurrentState;

	UFUNCTION(BlueprintCallable, Category = "Island HUD")
	void ApplyHudState(const FIslandHUDState& NewState);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Island HUD",
	          meta = (DisplayName = "HUD State Updated"))
	void BP_HudStateUpdated();
};
