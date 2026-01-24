// RFSN Director Integration
// Bridges RFSN Orchestrator with IslandDirectorSubsystem for AI-driven game
// pacing

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnDirectorBridge.generated.h"

class UIslandDirectorSubsystem;

/**
 * Bridges the RFSN Orchestrator with the Island Director system.
 * Allows RFSN to influence game pacing, spawning, and intensity.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnDirectorBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnDirectorBridge();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** URL of the RFSN Director control endpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Director")
	FString DirectorUrl = TEXT("http://127.0.0.1:8000/api/director/control");

	/** How often to poll RFSN for director commands (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Director",
	          meta = (ClampMin = "0.5", ClampMax = "30.0"))
	float PollInterval = 5.0f;

	/** Enable automatic polling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Director")
	bool bAutoPolling = false;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Send current game state to RFSN and request a director command */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Director")
	void RequestDirectorCommand();

	/** Map RFSN NPC action to alert level change */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Director")
	void ApplyNpcActionToDirector(ERfsnNpcAction Action);

	/** Start automatic polling */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Director")
	void StartPolling();

	/** Stop automatic polling */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Director")
	void StopPolling();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle PollTimer;

	UPROPERTY()
	TObjectPtr<UIslandDirectorSubsystem> DirectorSubsystem;

	void OnPollTick();
	void OnDirectorResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	/** Map NPC actions to alert level modifications */
	float GetAlertModifierForAction(ERfsnNpcAction Action) const;
};
