// RFSN NPC Config Asset
// Data asset for configuring RFSN NPCs in the editor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnAmbientChatter.h"
#include "RfsnNpcConfig.generated.h"

/**
 * Data asset for defining NPC personality, dialogue, and behavior.
 * Create in editor: Right-click → Miscellaneous → Data Asset → RfsnNpcConfig
 */
UCLASS(BlueprintType)
class MYPROJECT_API URfsnNpcConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Identity
	// ─────────────────────────────────────────────────────────────

	/** Unique identifier for this NPC type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString NpcId = TEXT("npc_default");

	/** Display name shown to player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString DisplayName = TEXT("NPC");

	/** Faction this NPC belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString FactionId = TEXT("survivors");

	// ─────────────────────────────────────────────────────────────
	// Personality
	// ─────────────────────────────────────────────────────────────

	/** Default mood (Neutral, Hostile, Friendly, Fearful, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	FString DefaultMood = TEXT("Neutral");

	/** Backstory/context for LLM */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (MultiLine = true))
	FString BackstoryContext;

	/** Personality traits for LLM */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	TArray<FString> PersonalityTraits;

	/** Speech style hints */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	FString SpeechStyle = TEXT("casual");

	// ─────────────────────────────────────────────────────────────
	// Relationship
	// ─────────────────────────────────────────────────────────────

	/** Initial relationship to player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	FString InitialRelationship = TEXT("Stranger");

	/** Initial affinity (-1 to 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float InitialAffinity = 0.0f;

	// ─────────────────────────────────────────────────────────────
	// Dialogue Behavior
	// ─────────────────────────────────────────────────────────────

	/** Enable proximity dialogue trigger */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bProximityDialogue = true;

	/** Dialogue trigger radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (EditCondition = "bProximityDialogue"))
	float DialogueRadius = 300.0f;

	/** Cooldown between dialogues */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	float DialogueCooldown = 10.0f;

	/** Opening line when player approaches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString GreetingLine;

	// ─────────────────────────────────────────────────────────────
	// Ambient Chatter
	// ─────────────────────────────────────────────────────────────

	/** Enable ambient chatter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	bool bEnableChatter = true;

	/** Pre-defined chatter lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chatter")
	TArray<FRfsnChatterLine> ChatterLines;

	// ─────────────────────────────────────────────────────────────
	// Visual
	// ─────────────────────────────────────────────────────────────

	/** Enable look-at behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	bool bLookAtPlayer = true;

	/** Enable dialogue camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	bool bDialogueCamera = false;

	// ─────────────────────────────────────────────────────────────
	// Audio
	// ─────────────────────────────────────────────────────────────

	/** TTS voice/engine */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FString TtsEngine = TEXT("kokoro");

	/** Voice pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float VoicePitch = 1.0f;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Apply this config to an NPC client component */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void ApplyToNpc(URfsnNpcClientComponent* Client);
};
