// Horror NPC with RFSN Dialogue
// Stealth-focused AI character with LLM-driven conversation

#pragma once

#include "CoreMinimal.h"
#include "MyProjectCharacter.h"
#include "RfsnNpcClientComponent.h"
#include "HorrorNPC.generated.h"

class URfsnNpcClientComponent;
class URfsnDialogueWidget;
class URfsnNpcDialogueTrigger;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHorrorNpcDeathDelegate);

/**
 * Horror variant NPC with RFSN LLM dialogue integration.
 * Designed for stealth-focused encounters with conversation abilities.
 */
UCLASS()
class MYPROJECT_API AHorrorNPC : public AMyProjectCharacter
{
	GENERATED_BODY()

public:
	AHorrorNPC();

	// ─────────────────────────────────────────────────────────────
	// Components
	// ─────────────────────────────────────────────────────────────

	/** RFSN client for LLM dialogue */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RFSN")
	TObjectPtr<URfsnNpcClientComponent> RfsnClient;

	/** Dialogue trigger */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RFSN")
	TObjectPtr<URfsnNpcDialogueTrigger> DialogueTrigger;

	/** Audio component for TTS playback */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UAudioComponent> VoiceAudio;

	// ─────────────────────────────────────────────────────────────
	// Properties
	// ─────────────────────────────────────────────────────────────

	/** NPC display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FString NpcDisplayName = TEXT("Stranger");

	/** Max HP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float MaxHP = 100.0f;

	/** Current HP */
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float CurrentHP = 100.0f;

	/** Is this NPC hostile? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bIsHostile = false;

	/** Aggro radius for detecting player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float AggroRadius = 500.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "NPC|Events")
	FHorrorNpcDeathDelegate OnDeath;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Initiate dialogue with player input */
	UFUNCTION(BlueprintCallable, Category = "RFSN")
	void SpeakToPlayer(const FString& PlayerUtterance);

	/** Apply damage to this NPC */
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void TakeDamageAmount(float Damage);

protected:
	virtual void BeginPlay() override;

	/** Handle RFSN action selection */
	UFUNCTION()
	void OnRfsnNpcAction(ERfsnNpcAction Action);

	/** Handle sentence received for potential audio */
	UFUNCTION()
	void OnRfsnSentence(const FRfsnSentence& Sentence);
};
