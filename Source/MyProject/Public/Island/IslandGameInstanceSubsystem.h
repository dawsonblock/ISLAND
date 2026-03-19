#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IslandGameInstanceSubsystem.generated.h"

class UIslandRunSaveGame;

UENUM(BlueprintType)
enum class EIslandRunEndReason : uint8
{
	Escaped,
	KilledByCult,
	Starved,
	Quit,
	Unknown
};

UCLASS()
class MYPROJECT_API UIslandGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Run")
	float RunTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Run")
	int32 CurrentSeed = 0;

	UFUNCTION(BlueprintCallable, Category="Run")
	void StartRun(int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category="Run")
	void EndRun(bool bEscaped, EIslandRunEndReason Reason = EIslandRunEndReason::Unknown);

	UFUNCTION(BlueprintCallable, Category="Run")
	UIslandRunSaveGame* GetSave() const { return Save; }

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	EIslandRunEndReason LastRunEndReason = EIslandRunEndReason::Unknown;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	UPROPERTY()
	TObjectPtr<UIslandRunSaveGame> Save;

	FString SlotName = TEXT("IslandSave");
	int32 UserIndex = 0;

	void LoadOrCreate();
	void SaveToDisk();
	void UnlockClue(FName ClueId);
};
