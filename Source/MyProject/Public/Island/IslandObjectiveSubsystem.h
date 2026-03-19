#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandObjectiveSubsystem.generated.h"

UCLASS()
class MYPROJECT_API UIslandObjectiveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Objective")
	void SetObjectiveActive(bool bActive, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void SetObjectiveText(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category="Objective")
	bool IsObjectiveActive() const { return bObjectiveActive; }

	UFUNCTION(BlueprintCallable, Category="Objective")
	FVector GetObjectiveLocation() const { return ObjectiveLocation; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	const FText& GetObjectiveText() const { return CurrentObjectiveText; }

private:
	UPROPERTY(BlueprintReadOnly, Category = "Objective", meta = (AllowPrivateAccess = "true"))
	FText CurrentObjectiveText;

	bool bObjectiveActive = false;
	FVector ObjectiveLocation = FVector::ZeroVector;
};
