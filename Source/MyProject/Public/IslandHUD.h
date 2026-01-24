#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RfsnNpcClientComponent.h"
#include "IslandHUD.generated.h"

UCLASS()
class MYPROJECT_API AIslandHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float X = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float Y = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float Line = 18.0f;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowTutorialMessage(const FString& Message, float Duration);

private:
	FString CurrentTutorialMessage;
	float TutorialMessageExpireTime;

	// RFSN Dialogue
	FString CurrentDialogueNpcName;
	FString CurrentDialogueSentence;
	float DialogueExpireTime = 0.0f;

public:
	/** Display NPC dialogue sentence */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowNpcDialogue(const FString& NpcName, const FString& Sentence, float Duration = 5.0f);

	/** Clear NPC dialogue */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearNpcDialogue();

	/** Bind to RFSN client for automatic dialogue display */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToRfsnClient(URfsnNpcClientComponent* RfsnClient);

private:
	UFUNCTION()
	void OnRfsnSentence(const FRfsnSentence& Sentence);

	FString BoundNpcName;
};
