// RFSN Performance Metrics Implementation

#include "RfsnMetrics.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnDialogueManager.h"
#include "RfsnNpcConversation.h"
#include "RfsnLogging.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/World.h"

void URfsnMetrics::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetMetrics();

	RFSN_LOG("RFSN Metrics system initialized");
}

void URfsnMetrics::Deinitialize()
{
	Super::Deinitialize();
}

void URfsnMetrics::RecordDialogueLatency(float LatencyMs)
{
	const int32 MaxSamples = 100;
	LatencySamples.Add(LatencyMs);
	if (LatencySamples.Num() > MaxSamples)
	{
		LatencySamples.RemoveAt(0);
	}
	UpdateLatencyStats();
}

void URfsnMetrics::RecordSentenceReceived()
{
	ComponentMetrics.TotalSentencesReceived++;
}

void URfsnMetrics::RecordActionReceived()
{
	ComponentMetrics.TotalActionsReceived++;
}

void URfsnMetrics::ResetMetrics()
{
	ComponentMetrics = FRfsnComponentMetrics();
	PerformanceMetrics = FRfsnPerformanceMetrics();
	LatencySamples.Empty();
}

FString URfsnMetrics::GetMetricsString() const
{
	return FString::Printf(TEXT("RFSN Metrics\n") TEXT("────────────────\n") TEXT("Active NPCs: %d\n")
	                           TEXT("Active Dialogues: %d\n") TEXT("Active Convs: %d\n") TEXT("Total Sentences: %d\n")
	                               TEXT("Total Actions: %d\n") TEXT("────────────────\n") TEXT("Avg Latency: %.1fms\n")
	                                   TEXT("Min Latency: %.1fms\n") TEXT("Max Latency: %.1fms\n"),
	                       ComponentMetrics.ActiveNpcClients, ComponentMetrics.ActiveDialogues,
	                       ComponentMetrics.ActiveConversations, ComponentMetrics.TotalSentencesReceived,
	                       ComponentMetrics.TotalActionsReceived, PerformanceMetrics.AverageDialogueLatencyMs,
	                       PerformanceMetrics.MinDialogueLatencyMs, PerformanceMetrics.MaxDialogueLatencyMs);
}

void URfsnMetrics::UpdateMetrics()
{
	CollectMetrics();
	OnMetricsUpdated.Broadcast(ComponentMetrics, PerformanceMetrics);

	if (bLogMetrics)
	{
		RFSN_VERBOSE("%s", *GetMetricsString());
	}
}

void URfsnMetrics::CollectMetrics()
{
	// Count active NPC clients
	int32 NpcCount = 0;

	// This needs a world context - would be done differently in production
	// For now, just track what we can locally

	// Count active dialogues via RfsnDialogueManager
	// Count active conversations via RfsnNpcConversation
}

void URfsnMetrics::UpdateLatencyStats()
{
	if (LatencySamples.Num() == 0)
	{
		PerformanceMetrics.AverageDialogueLatencyMs = 0.0f;
		PerformanceMetrics.MinDialogueLatencyMs = 0.0f;
		PerformanceMetrics.MaxDialogueLatencyMs = 0.0f;
		return;
	}

	float Sum = 0.0f;
	float Min = LatencySamples[0];
	float Max = LatencySamples[0];

	for (float Sample : LatencySamples)
	{
		Sum += Sample;
		Min = FMath::Min(Min, Sample);
		Max = FMath::Max(Max, Sample);
	}

	PerformanceMetrics.AverageDialogueLatencyMs = Sum / LatencySamples.Num();
	PerformanceMetrics.MinDialogueLatencyMs = Min;
	PerformanceMetrics.MaxDialogueLatencyMs = Max;
}
