// RFSN Temporal Context Memory
// Short-term memory over recent state-action-outcome triplets
// Enables anticipatory scoring: "this feels like last time"

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnTemporalMemory.generated.h"

/**
 * State-action-outcome triplet with context embedding
 */
USTRUCT(BlueprintType)
struct FRfsnMemoryTrace
{
	GENERATED_BODY()

	/** Encoded state hash (mood + relationship + affinity bucket) */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	int32 StateHash = 0;

	/** Action taken */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	ERfsnNpcAction Action = ERfsnNpcAction::Talk;

	/** Outcome signal from player response (-1 to 1) */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float Outcome = 0.0f;

	/** Context features at time of decision */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	FString PlayerSignal;

	/** Timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float Timestamp = 0.0f;
};

/**
 * Prior bias derived from context similarity
 */
USTRUCT(BlueprintType)
struct FRfsnActionBias
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Bias")
	ERfsnNpcAction Action = ERfsnNpcAction::Talk;

	/** Additive bias to scoring (-1 to 1) */
	UPROPERTY(BlueprintReadWrite, Category = "Bias")
	float Bias = 0.0f;

	/** Confidence (0 to 1, based on sample count) */
	UPROPERTY(BlueprintReadWrite, Category = "Bias")
	float Confidence = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryRecorded, const FRfsnMemoryTrace&, Trace);

/**
 * Temporal memory component that tracks recent state-action-outcome history.
 * Provides prior biases based on context similarity before action selection.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnTemporalMemory : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnTemporalMemory();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum memory traces to retain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
	int32 MaxTraces = 50;

	/** Weight for recent traces vs older (decay factor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
	float RecencyWeight = 0.85f;

	/** Minimum confidence to apply bias */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
	float MinConfidenceThreshold = 0.2f;

	/** State similarity threshold (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
	float SimilarityThreshold = 0.7f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Memory|Events")
	FOnMemoryRecorded OnMemoryRecorded;

	// ─────────────────────────────────────────────────────────────
	// Recording API
	// ─────────────────────────────────────────────────────────────

	/** Record state-action-outcome after receiving player response */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void RecordOutcome(ERfsnNpcAction Action, float Outcome, const FString& Mood, const FString& Relationship,
	                   float Affinity, const FString& PlayerSignal);

	/** Record from RFSN client state */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void RecordFromClient(URfsnNpcClientComponent* Client, float Outcome, const FString& PlayerSignal);

	// ─────────────────────────────────────────────────────────────
	// Retrieval API (call BEFORE action selection)
	// ─────────────────────────────────────────────────────────────

	/** Get action biases for current context */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	TArray<FRfsnActionBias> GetActionBiases(const FString& Mood, const FString& Relationship, float Affinity,
	                                        const FString& PlayerSignal);

	/** Get bias for specific action */
	UFUNCTION(BlueprintPure, Category = "Memory")
	float GetActionBias(ERfsnNpcAction Action, const FString& Mood, const FString& Relationship, float Affinity);

	/** Check if context feels similar to a recent negative outcome */
	UFUNCTION(BlueprintPure, Category = "Memory")
	bool HasNegativeMemory(const FString& Mood, const FString& Relationship, float Affinity);

	/** Get most recent traces */
	UFUNCTION(BlueprintPure, Category = "Memory")
	TArray<FRfsnMemoryTrace> GetRecentTraces(int32 Count = 10) const;

	/** Clear all memory */
	UFUNCTION(BlueprintCallable, Category = "Memory")
	void ClearMemory();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FRfsnMemoryTrace> Traces;

	int32 ComputeStateHash(const FString& Mood, const FString& Relationship, float Affinity) const;
	float ComputeSimilarity(int32 HashA, int32 HashB) const;
	float ComputeSignalSimilarity(const FString& SignalA, const FString& SignalB) const;
};
