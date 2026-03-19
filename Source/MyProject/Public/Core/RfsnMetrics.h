// RFSN Performance Metrics
// Tracks and reports performance statistics for the RFSN system

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RfsnMetrics.generated.h"

USTRUCT(BlueprintType)
struct FRfsnComponentMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 ActiveNpcClients = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 ActiveDialogues = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 ActiveConversations = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 TotalSentencesReceived = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 TotalActionsReceived = 0;
};

USTRUCT(BlueprintType)
struct FRfsnPerformanceMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float AverageDialogueLatencyMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float MinDialogueLatencyMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float MaxDialogueLatencyMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float AverageTokensPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float MemoryUsageMb = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetricsUpdated, const FRfsnComponentMetrics&, Components,
                                             const FRfsnPerformanceMetrics&, Performance);

/**
 * Subsystem for tracking RFSN performance metrics.
 */
UCLASS()
class MYPROJECT_API URfsnMetrics : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Update metrics every N seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float UpdateInterval = 1.0f;

	/** Log metrics to console */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	bool bLogMetrics = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Metrics|Events")
	FOnMetricsUpdated OnMetricsUpdated;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get current component metrics */
	UFUNCTION(BlueprintPure, Category = "Metrics")
	FRfsnComponentMetrics GetComponentMetrics() const { return ComponentMetrics; }

	/** Get current performance metrics */
	UFUNCTION(BlueprintPure, Category = "Metrics")
	FRfsnPerformanceMetrics GetPerformanceMetrics() const { return PerformanceMetrics; }

	/** Record dialogue latency sample */
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void RecordDialogueLatency(float LatencyMs);

	/** Record sentence received */
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void RecordSentenceReceived();

	/** Record action received */
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void RecordActionReceived();

	/** Reset all metrics */
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void ResetMetrics();

	/** Get formatted metrics string for display */
	UFUNCTION(BlueprintPure, Category = "Metrics")
	FString GetMetricsString() const;

	/** Force immediate metrics update */
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void UpdateMetrics();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	FRfsnComponentMetrics ComponentMetrics;
	FRfsnPerformanceMetrics PerformanceMetrics;
	TArray<float> LatencySamples;

	FTimerHandle MetricsTimerHandle;

	void CollectMetrics();
	void UpdateLatencyStats();
};
