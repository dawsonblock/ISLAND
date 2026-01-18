#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IslandHUD.generated.h"

UCLASS()
class MYPROJECT_API AIslandHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
	float X = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
	float Y = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
	float Line = 18.0f;

	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowTutorialMessage(const FString& Message, float Duration);

private:
	FString CurrentTutorialMessage;
	float TutorialMessageExpireTime;
};
