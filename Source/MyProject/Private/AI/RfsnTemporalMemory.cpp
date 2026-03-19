// RFSN Temporal Context Memory Implementation

#include "RfsnTemporalMemory.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"

URfsnTemporalMemory::URfsnTemporalMemory()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnTemporalMemory::BeginPlay()
{
	Super::BeginPlay();
}

void URfsnTemporalMemory::RecordOutcome(ERfsnNpcAction Action, float Outcome, const FString& Mood,
                                        const FString& Relationship, float Affinity, const FString& PlayerSignal)
{
	FRfsnMemoryTrace Trace;
	Trace.StateHash = ComputeStateHash(Mood, Relationship, Affinity);
	Trace.Action = Action;
	Trace.Outcome = FMath::Clamp(Outcome, -1.0f, 1.0f);
	Trace.PlayerSignal = PlayerSignal;
	Trace.Timestamp = UGameplayStatics::GetTimeSeconds(GetWorld());

	Traces.Add(Trace);

	// Enforce max traces (FIFO)
	while (Traces.Num() > MaxTraces)
	{
		Traces.RemoveAt(0);
	}

	OnMemoryRecorded.Broadcast(Trace);

	RFSN_VERBOSE(TEXT("Memory recorded: Action=%d, Outcome=%.2f, StateHash=%d"), static_cast<int32>(Action), Outcome,
	             Trace.StateHash);
}

void URfsnTemporalMemory::RecordFromClient(URfsnNpcClientComponent* Client, float Outcome, const FString& PlayerSignal)
{
	if (!Client)
	{
		return;
	}

	RecordOutcome(Client->GetLastNpcAction(), Outcome, Client->Mood, Client->Relationship, Client->Affinity,
	              PlayerSignal);
}

TArray<FRfsnActionBias> URfsnTemporalMemory::GetActionBiases(const FString& Mood, const FString& Relationship,
                                                             float Affinity, const FString& PlayerSignal)
{
	TArray<FRfsnActionBias> Biases;

	int32 CurrentStateHash = ComputeStateHash(Mood, Relationship, Affinity);
	float CurrentTime = UGameplayStatics::GetTimeSeconds(GetWorld());

	// Accumulate outcome-weighted biases per action
	TMap<ERfsnNpcAction, float> ActionSums;
	TMap<ERfsnNpcAction, float> ActionWeights;

	for (int32 i = Traces.Num() - 1; i >= 0; --i)
	{
		const FRfsnMemoryTrace& Trace = Traces[i];

		// Compute context similarity
		float StateSim = ComputeSimilarity(CurrentStateHash, Trace.StateHash);
		float SignalSim = ComputeSignalSimilarity(PlayerSignal, Trace.PlayerSignal);
		float TotalSim = StateSim * 0.7f + SignalSim * 0.3f;

		if (TotalSim < SimilarityThreshold)
		{
			continue;
		}

		// Recency decay
		float Age = CurrentTime - Trace.Timestamp;
		float RecencyFactor = FMath::Pow(RecencyWeight, Age / 60.0f); // Decay per minute

		float Weight = TotalSim * RecencyFactor;

		// Accumulate
		if (!ActionSums.Contains(Trace.Action))
		{
			ActionSums.Add(Trace.Action, 0.0f);
			ActionWeights.Add(Trace.Action, 0.0f);
		}

		ActionSums[Trace.Action] += Trace.Outcome * Weight;
		ActionWeights[Trace.Action] += Weight;
	}

	// Convert to biases
	for (auto& Pair : ActionSums)
	{
		float TotalWeight = ActionWeights[Pair.Key];
		if (TotalWeight < 0.01f)
		{
			continue;
		}

		FRfsnActionBias Bias;
		Bias.Action = Pair.Key;
		Bias.Bias = Pair.Value / TotalWeight;
		Bias.Confidence = FMath::Min(TotalWeight / 3.0f, 1.0f); // 3 samples = full confidence

		if (Bias.Confidence >= MinConfidenceThreshold)
		{
			Biases.Add(Bias);
		}
	}

	return Biases;
}

float URfsnTemporalMemory::GetActionBias(ERfsnNpcAction Action, const FString& Mood, const FString& Relationship,
                                         float Affinity)
{
	TArray<FRfsnActionBias> AllBiases = GetActionBiases(Mood, Relationship, Affinity, TEXT(""));

	for (const FRfsnActionBias& Bias : AllBiases)
	{
		if (Bias.Action == Action)
		{
			return Bias.Bias * Bias.Confidence;
		}
	}

	return 0.0f;
}

bool URfsnTemporalMemory::HasNegativeMemory(const FString& Mood, const FString& Relationship, float Affinity)
{
	int32 CurrentHash = ComputeStateHash(Mood, Relationship, Affinity);

	for (int32 i = Traces.Num() - 1; i >= FMath::Max(0, Traces.Num() - 10); --i)
	{
		const FRfsnMemoryTrace& Trace = Traces[i];
		if (Trace.Outcome < -0.3f && ComputeSimilarity(CurrentHash, Trace.StateHash) > SimilarityThreshold)
		{
			return true;
		}
	}

	return false;
}

TArray<FRfsnMemoryTrace> URfsnTemporalMemory::GetRecentTraces(int32 Count) const
{
	TArray<FRfsnMemoryTrace> Result;
	int32 Start = FMath::Max(0, Traces.Num() - Count);
	for (int32 i = Start; i < Traces.Num(); ++i)
	{
		Result.Add(Traces[i]);
	}
	return Result;
}

void URfsnTemporalMemory::ClearMemory()
{
	Traces.Empty();
	RFSN_LOG(TEXT("Temporal memory cleared"));
}

int32 URfsnTemporalMemory::ComputeStateHash(const FString& Mood, const FString& Relationship, float Affinity) const
{
	// Simple hash: bucket affinity, combine with mood/relationship
	int32 AffinityBucket = FMath::Clamp(FMath::RoundToInt((Affinity + 1.0f) * 2.5f), 0, 5);
	int32 MoodHash = GetTypeHash(Mood) % 100;
	int32 RelHash = GetTypeHash(Relationship) % 100;

	return AffinityBucket * 10000 + MoodHash * 100 + RelHash;
}

float URfsnTemporalMemory::ComputeSimilarity(int32 HashA, int32 HashB) const
{
	if (HashA == HashB)
	{
		return 1.0f;
	}

	// Extract components
	int32 AffinityA = HashA / 10000;
	int32 AffinityB = HashB / 10000;
	int32 MoodA = (HashA / 100) % 100;
	int32 MoodB = (HashB / 100) % 100;
	int32 RelA = HashA % 100;
	int32 RelB = HashB % 100;

	float AffinitySim = 1.0f - FMath::Abs(AffinityA - AffinityB) / 5.0f;
	float MoodSim = (MoodA == MoodB) ? 1.0f : 0.3f;
	float RelSim = (RelA == RelB) ? 1.0f : 0.3f;

	return AffinitySim * 0.4f + MoodSim * 0.3f + RelSim * 0.3f;
}

float URfsnTemporalMemory::ComputeSignalSimilarity(const FString& SignalA, const FString& SignalB) const
{
	if (SignalA.IsEmpty() || SignalB.IsEmpty())
	{
		return 0.5f; // Neutral if missing
	}

	if (SignalA == SignalB)
	{
		return 1.0f;
	}

	// Simple keyword matching
	TArray<FString> KeywordsA;
	TArray<FString> KeywordsB;
	SignalA.ToLower().ParseIntoArray(KeywordsA, TEXT(" "));
	SignalB.ToLower().ParseIntoArray(KeywordsB, TEXT(" "));

	int32 Matches = 0;
	for (const FString& KwA : KeywordsA)
	{
		if (KeywordsB.Contains(KwA))
		{
			Matches++;
		}
	}

	int32 Total = FMath::Max(KeywordsA.Num(), KeywordsB.Num());
	return Total > 0 ? static_cast<float>(Matches) / Total : 0.5f;
}
