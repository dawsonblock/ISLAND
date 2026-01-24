// RFSN Action Lattice
// Expanded action representation with modifiers for nuanced intent
// Keeps complexity in the action space, not the LLM prompt

#pragma once

#include "CoreMinimal.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnActionLattice.generated.h"

/**
 * Action intensity level
 */
UENUM(BlueprintType)
enum class ERfsnActionIntensity : uint8
{
	Subdued UMETA(DisplayName = "Subdued"),  // Reluctant, hesitant
	Normal UMETA(DisplayName = "Normal"),    // Standard execution
	Emphatic UMETA(DisplayName = "Emphatic") // Strong, confident
};

/**
 * Action compliance level
 */
UENUM(BlueprintType)
enum class ERfsnActionCompliance : uint8
{
	Full UMETA(DisplayName = "Full"),           // Complete action
	Partial UMETA(DisplayName = "Partial"),     // Do some, hedge
	Reluctant UMETA(DisplayName = "Reluctant"), // Do but show displeasure
	Deferred UMETA(DisplayName = "Deferred")    // Promise to do later
};

/**
 * Internal motive coloring
 */
UENUM(BlueprintType)
enum class ERfsnActionMotive : uint8
{
	Sincere UMETA(DisplayName = "Sincere"),       // Genuine intent
	Guarded UMETA(DisplayName = "Guarded"),       // Holding back
	Calculated UMETA(DisplayName = "Calculated"), // Strategic choice
	Conflicted UMETA(DisplayName = "Conflicted")  // Mixed feelings
};

/**
 * Extended action with modifiers
 * This is what gets sent to the LLM for tone rendering
 */
USTRUCT(BlueprintType)
struct FRfsnExpandedAction
{
	GENERATED_BODY()

	/** Base action (what to do) */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	ERfsnNpcAction BaseAction = ERfsnNpcAction::Talk;

	/** How strongly to execute */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	ERfsnActionIntensity Intensity = ERfsnActionIntensity::Normal;

	/** How fully to comply */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	ERfsnActionCompliance Compliance = ERfsnActionCompliance::Full;

	/** Underlying motive */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	ERfsnActionMotive Motive = ERfsnActionMotive::Sincere;

	/** Optional qualifier (e.g., "with suspicion", "reluctantly") */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	FString Qualifier;

	/** Convert to prompt hint for LLM */
	FString ToPromptHint() const;

	/** Create simple action (backwards compatible) */
	static FRfsnExpandedAction Simple(ERfsnNpcAction Action);

	/** Create hesitant action */
	static FRfsnExpandedAction Hesitant(ERfsnNpcAction Action);

	/** Create reluctant action */
	static FRfsnExpandedAction Reluctant(ERfsnNpcAction Action);

	/** Create calculated action */
	static FRfsnExpandedAction Calculated(ERfsnNpcAction Action);

	/** Create conflicted action */
	static FRfsnExpandedAction Conflicted(ERfsnNpcAction Action, ERfsnNpcAction AlternateInClination);
};

/**
 * Static helper for action lattice operations
 */
UCLASS(BlueprintType)
class MYPROJECT_API URfsnActionLattice : public UObject
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Action Construction
	// ─────────────────────────────────────────────────────────────

	/** Build expanded action from context */
	UFUNCTION(BlueprintCallable, Category = "ActionLattice")
	static FRfsnExpandedAction BuildAction(ERfsnNpcAction BaseAction, float Affinity, float ActionBias,
	                                       bool bHasNegativeMemory);

	/** Apply context modifiers to base action */
	UFUNCTION(BlueprintCallable, Category = "ActionLattice")
	static FRfsnExpandedAction ApplyContextModifiers(ERfsnNpcAction BaseAction, const FString& Mood,
	                                                 const FString& Relationship, float Affinity,
	                                                 const TArray<FRfsnActionBias>& Biases);

	// ─────────────────────────────────────────────────────────────
	// Action Scoring
	// ─────────────────────────────────────────────────────────────

	/** Get all valid expanded actions for state */
	UFUNCTION(BlueprintCallable, Category = "ActionLattice")
	static TArray<FRfsnExpandedAction> GetValidActions(const FString& Mood, const FString& Relationship, float Affinity,
	                                                   const FString& PlayerSignal);

	/** Score an expanded action (higher = more appropriate) */
	UFUNCTION(BlueprintPure, Category = "ActionLattice")
	static float ScoreAction(const FRfsnExpandedAction& Action, const FString& Mood, float Affinity);

	// ─────────────────────────────────────────────────────────────
	// Conversion
	// ─────────────────────────────────────────────────────────────

	/** Convert base action enum to string */
	UFUNCTION(BlueprintPure, Category = "ActionLattice")
	static FString ActionToString(ERfsnNpcAction Action);

	/** Convert intensity to prompt modifier */
	UFUNCTION(BlueprintPure, Category = "ActionLattice")
	static FString IntensityToModifier(ERfsnActionIntensity Intensity);

	/** Convert compliance to prompt modifier */
	UFUNCTION(BlueprintPure, Category = "ActionLattice")
	static FString ComplianceToModifier(ERfsnActionCompliance Compliance);

	/** Convert motive to prompt modifier */
	UFUNCTION(BlueprintPure, Category = "ActionLattice")
	static FString MotiveToModifier(ERfsnActionMotive Motive);
};
