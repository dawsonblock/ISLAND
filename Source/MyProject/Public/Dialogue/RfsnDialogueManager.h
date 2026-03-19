// RFSN Dialogue Manager Subsystem
// Coordinates NPC dialogue sessions for the player

#pragma once

#include "CoreMinimal.h"
#include "RfsnNpcClientComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "RfsnDialogueManager.generated.h"

class URfsnPlayerInputWidget;
class AIslandHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStarted, AActor*, NpcActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);

/**
 * World Subsystem that manages active RFSN dialogue sessions.
 * Coordinates between player input, NPC clients, and HUD display.
 */
UCLASS()
class MYPROJECT_API URfsnDialogueManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start dialogue with an NPC */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue")
	bool StartDialogue(AActor* NpcActor);

	/** End current dialogue session */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue")
	void EndDialogue();

	/** Send player message to current NPC */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue")
	void SendPlayerMessage(const FString& Message);

	/** Check if dialogue is active */
	UFUNCTION(BlueprintPure, Category = "RFSN|Dialogue")
	bool IsDialogueActive() const { return ActiveNpc != nullptr; }

	/** Get currently engaged NPC */
	UFUNCTION(BlueprintPure, Category = "RFSN|Dialogue")
	AActor* GetActiveNpc() const { return ActiveNpc; }

	/** Find nearest NPC with RFSN client in range */
	UFUNCTION(BlueprintCallable, Category = "RFSN|Dialogue")
	AActor* FindNearestRfsnNpc(const FVector& Location, float MaxDistance = 300.0f) const;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnDialogueStarted OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnDialogueEnded OnDialogueEnded;

protected:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TObjectPtr<AActor> ActiveNpc;

	UPROPERTY()
	TObjectPtr<URfsnNpcClientComponent> ActiveClient;

	UFUNCTION()
	void OnSentenceReceived(const FRfsnSentence& Sentence);

	UFUNCTION()
	void OnDialogueComplete();

	UFUNCTION()
	void OnNpcAction(ERfsnNpcAction Action);
};
