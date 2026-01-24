// RFSN NPC Memory Implementation

#include "RfsnNpcMemory.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Kismet/GameplayStatics.h"

URfsnNpcMemory::URfsnNpcMemory()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnNpcMemory::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoSave)
	{
		LoadMemories();
	}

	RFSN_LOG(TEXT("NpcMemory initialized for %s with %d memories"), *GetOwner()->GetName(), Memories.Num());
}

FGuid URfsnNpcMemory::CreateMemory(ERfsnMemoryType Type, const FString& Summary, float EmotionalImpact,
                                   float Importance)
{
	FRfsnMemoryEntry Memory;
	Memory.Type = Type;
	Memory.Summary = Summary;
	Memory.EmotionalImpact = FMath::Clamp(EmotionalImpact, -1.0f, 1.0f);
	Memory.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
	Memory.GameTimeWhenOccurred = UGameplayStatics::GetTimeSeconds(GetWorld());
	Memory.Topics = DetectTopics(Summary);

	Memories.Add(Memory);
	TrimMemories();

	OnMemoryCreated.Broadcast(Memory);

	if (bAutoSave)
	{
		SaveMemories();
	}

	RFSN_LOG(TEXT("Created memory: %s"), *Summary);
	return Memory.MemoryId;
}

void URfsnNpcMemory::AddTopicToMemory(const FGuid& MemoryId, const FString& Topic)
{
	for (FRfsnMemoryEntry& Memory : Memories)
	{
		if (Memory.MemoryId == MemoryId)
		{
			Memory.Topics.AddUnique(Topic);
			return;
		}
	}
}

TArray<FRfsnMemoryEntry> URfsnNpcMemory::RecallByTopic(const FString& Topic) const
{
	TArray<FRfsnMemoryEntry> Results;
	FString LowerTopic = Topic.ToLower();

	for (const FRfsnMemoryEntry& Memory : Memories)
	{
		for (const FString& MemTopic : Memory.Topics)
		{
			if (MemTopic.ToLower().Contains(LowerTopic))
			{
				Results.Add(Memory);
				break;
			}
		}
	}

	// Sort by strength
	Results.Sort([](const FRfsnMemoryEntry& A, const FRfsnMemoryEntry& B) { return A.Strength > B.Strength; });

	return Results;
}

TArray<FRfsnMemoryEntry> URfsnNpcMemory::RecallByType(ERfsnMemoryType Type) const
{
	TArray<FRfsnMemoryEntry> Results;

	for (const FRfsnMemoryEntry& Memory : Memories)
	{
		if (Memory.Type == Type)
		{
			Results.Add(Memory);
		}
	}

	return Results;
}

TArray<FRfsnMemoryEntry> URfsnNpcMemory::GetRecentMemories(int32 Count) const
{
	TArray<FRfsnMemoryEntry> Results;
	int32 StartIdx = FMath::Max(0, Memories.Num() - Count);

	for (int32 i = Memories.Num() - 1; i >= StartIdx; --i)
	{
		Results.Add(Memories[i]);
	}

	return Results;
}

TArray<FRfsnMemoryEntry> URfsnNpcMemory::GetStrongestMemories(int32 Count) const
{
	TArray<FRfsnMemoryEntry> Sorted = Memories;
	Sorted.Sort([](const FRfsnMemoryEntry& A, const FRfsnMemoryEntry& B)
	            { return A.Strength * A.Importance > B.Strength * B.Importance; });

	TArray<FRfsnMemoryEntry> Results;
	for (int32 i = 0; i < FMath::Min(Count, Sorted.Num()); ++i)
	{
		Results.Add(Sorted[i]);
	}

	return Results;
}

FRfsnMemoryEntry URfsnNpcMemory::FindMemory(const FGuid& MemoryId) const
{
	for (const FRfsnMemoryEntry& Memory : Memories)
	{
		if (Memory.MemoryId == MemoryId)
		{
			return Memory;
		}
	}
	return FRfsnMemoryEntry();
}

void URfsnNpcMemory::ReinforceMemory(const FGuid& MemoryId)
{
	for (FRfsnMemoryEntry& Memory : Memories)
	{
		if (Memory.MemoryId == MemoryId)
		{
			Memory.ReinforcementCount++;
			Memory.Strength = FMath::Min(1.0f, Memory.Strength + 0.1f);
			OnMemoryRecalled.Broadcast(Memory);
			return;
		}
	}
}

void URfsnNpcMemory::StartConversation()
{
	bInConversation = true;
	CurrentConversation = FRfsnConversationSnapshot();
	CurrentConversation.StartTime = FDateTime::Now();
}

void URfsnNpcMemory::RecordPlayerStatement(const FString& Statement)
{
	if (bInConversation)
	{
		CurrentConversation.AddPlayerStatement(Statement);

		// Detect and add topics
		TArray<FString> Topics = DetectTopics(Statement);
		for (const FString& Topic : Topics)
		{
			CurrentConversation.DetectedTopics.AddUnique(Topic);
		}
	}
}

void URfsnNpcMemory::RecordNpcResponse(const FString& Response)
{
	if (bInConversation)
	{
		CurrentConversation.AddNpcResponse(Response);
	}
}

FGuid URfsnNpcMemory::EndConversation()
{
	if (!bInConversation)
	{
		return FGuid();
	}

	bInConversation = false;

	// Create summary from conversation
	FString Summary;
	if (CurrentConversation.PlayerStatements.Num() > 0)
	{
		Summary = TEXT("Talked about ");
		if (CurrentConversation.DetectedTopics.Num() > 0)
		{
			Summary += FString::Join(CurrentConversation.DetectedTopics, TEXT(", "));
		}
		else
		{
			Summary += TEXT("various topics");
		}
	}
	else
	{
		Summary = TEXT("Brief interaction");
	}

	// Create memory from conversation
	FGuid MemoryId = CreateMemory(ERfsnMemoryType::Conversation, Summary, CurrentConversation.OverallSentiment, 0.5f);

	// Add detected topics to memory
	for (const FString& Topic : CurrentConversation.DetectedTopics)
	{
		AddTopicToMemory(MemoryId, Topic);
	}

	return MemoryId;
}

FString URfsnNpcMemory::GetMemoryContext(int32 InMaxMemories) const
{
	TArray<FRfsnMemoryEntry> StrongMemories = GetStrongestMemories(InMaxMemories);

	if (StrongMemories.Num() == 0)
	{
		return TEXT("No prior interactions to remember.");
	}

	FString Context = TEXT("Past interactions: ");

	for (const FRfsnMemoryEntry& Memory : StrongMemories)
	{
		FString Sentiment = Memory.EmotionalImpact > 0.3f    ? TEXT("positive")
		                    : Memory.EmotionalImpact < -0.3f ? TEXT("negative")
		                                                     : TEXT("neutral");

		Context +=
		    FString::Printf(TEXT("[%s, %s: %s] "), *Sentiment,
		                    *StaticEnum<ERfsnMemoryType>()->GetNameStringByValue((int64)Memory.Type), *Memory.Summary);
	}

	return Context;
}

FString URfsnNpcMemory::GetConversationHistory() const
{
	if (!bInConversation || CurrentConversation.PlayerStatements.Num() == 0)
	{
		return TEXT("");
	}

	FString History = TEXT("Recent conversation:\n");

	int32 MaxLines = FMath::Min(CurrentConversation.PlayerStatements.Num(), CurrentConversation.NpcResponses.Num());

	for (int32 i = 0; i < MaxLines; ++i)
	{
		History += FString::Printf(TEXT("Player: %s\n"), *CurrentConversation.PlayerStatements[i]);
		if (i < CurrentConversation.NpcResponses.Num())
		{
			History += FString::Printf(TEXT("NPC: %s\n"), *CurrentConversation.NpcResponses[i]);
		}
	}

	return History;
}

bool URfsnNpcMemory::HasMetPlayer() const
{
	return RecallByType(ERfsnMemoryType::FirstMeeting).Num() > 0 ||
	       RecallByType(ERfsnMemoryType::Conversation).Num() > 0;
}

void URfsnNpcMemory::DecayMemories(float GameHoursElapsed)
{
	if (MemoryDecayRate <= 0.0f)
	{
		return;
	}

	float DecayAmount = MemoryDecayRate * GameHoursElapsed;

	for (int32 i = Memories.Num() - 1; i >= 0; --i)
	{
		// Important memories decay slower
		float AdjustedDecay = DecayAmount * (1.0f - Memories[i].Importance * 0.5f);

		// Reinforced memories decay slower
		AdjustedDecay /= (1.0f + Memories[i].ReinforcementCount * 0.2f);

		Memories[i].Strength -= AdjustedDecay;

		if (Memories[i].Strength < ForgetThreshold)
		{
			RFSN_LOG(TEXT("Forgot memory: %s"), *Memories[i].Summary);
			Memories.RemoveAt(i);
		}
	}
}

void URfsnNpcMemory::TrimMemories()
{
	if (Memories.Num() <= MaxMemories)
	{
		return;
	}

	// Sort by strength * importance
	Memories.Sort([](const FRfsnMemoryEntry& A, const FRfsnMemoryEntry& B)
	              { return A.Strength * A.Importance > B.Strength * B.Importance; });

	// Remove weakest
	while (Memories.Num() > MaxMemories)
	{
		Memories.RemoveAt(Memories.Num() - 1);
	}
}

FString URfsnNpcMemory::GetSavePath() const
{
	FString NpcId = TEXT("unknown");
	if (URfsnNpcClientComponent* Client = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		NpcId = Client->NpcId;
	}
	return FPaths::ProjectSavedDir() / TEXT("Memories") / FString::Printf(TEXT("Memory_%s.json"), *NpcId);
}

void URfsnNpcMemory::SaveMemories()
{
	FString SavePath = GetSavePath();

	TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> MemoriesArray;

	for (const FRfsnMemoryEntry& Memory : Memories)
	{
		TSharedRef<FJsonObject> MemObj = MakeShared<FJsonObject>();
		MemObj->SetStringField(TEXT("id"), Memory.MemoryId.ToString());
		MemObj->SetNumberField(TEXT("type"), (int32)Memory.Type);
		MemObj->SetStringField(TEXT("summary"), Memory.Summary);
		MemObj->SetNumberField(TEXT("impact"), Memory.EmotionalImpact);
		MemObj->SetNumberField(TEXT("importance"), Memory.Importance);
		MemObj->SetNumberField(TEXT("strength"), Memory.Strength);
		MemObj->SetNumberField(TEXT("reinforcement"), Memory.ReinforcementCount);

		TArray<TSharedPtr<FJsonValue>> TopicsArray;
		for (const FString& Topic : Memory.Topics)
		{
			TopicsArray.Add(MakeShared<FJsonValueString>(Topic));
		}
		MemObj->SetArrayField(TEXT("topics"), TopicsArray);

		MemoriesArray.Add(MakeShared<FJsonValueObject>(MemObj));
	}

	RootObj->SetArrayField(TEXT("memories"), MemoriesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObj, Writer);

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);
	FFileHelper::SaveStringToFile(OutputString, *SavePath);
}

bool URfsnNpcMemory::LoadMemories()
{
	FString SavePath = GetSavePath();
	FString JsonString;

	if (!FFileHelper::LoadFileToString(JsonString, *SavePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		return false;
	}

	Memories.Empty();

	const TArray<TSharedPtr<FJsonValue>>* MemoriesArray;
	if (RootObj->TryGetArrayField(TEXT("memories"), MemoriesArray))
	{
		for (const TSharedPtr<FJsonValue>& MemVal : *MemoriesArray)
		{
			const TSharedPtr<FJsonObject>* MemObj;
			if (MemVal->TryGetObject(MemObj))
			{
				FRfsnMemoryEntry Memory;
				FGuid::Parse((*MemObj)->GetStringField(TEXT("id")), Memory.MemoryId);
				Memory.Type = (ERfsnMemoryType)(*MemObj)->GetIntegerField(TEXT("type"));
				Memory.Summary = (*MemObj)->GetStringField(TEXT("summary"));
				Memory.EmotionalImpact = (*MemObj)->GetNumberField(TEXT("impact"));
				Memory.Importance = (*MemObj)->GetNumberField(TEXT("importance"));
				Memory.Strength = (*MemObj)->GetNumberField(TEXT("strength"));
				Memory.ReinforcementCount = (*MemObj)->GetIntegerField(TEXT("reinforcement"));

				const TArray<TSharedPtr<FJsonValue>>* TopicsArray;
				if ((*MemObj)->TryGetArrayField(TEXT("topics"), TopicsArray))
				{
					for (const TSharedPtr<FJsonValue>& TopicVal : *TopicsArray)
					{
						Memory.Topics.Add(TopicVal->AsString());
					}
				}

				Memories.Add(Memory);
			}
		}
	}

	RFSN_LOG(TEXT("Loaded %d memories"), Memories.Num());
	return true;
}

TArray<FString> URfsnNpcMemory::DetectTopics(const FString& Text) const
{
	TArray<FString> Topics;
	FString LowerText = Text.ToLower();

	// Common topic keywords
	static TArray<TPair<FString, FString>> TopicKeywords = {{TEXT("quest"), TEXT("Quests")},
	                                                        {TEXT("mission"), TEXT("Quests")},
	                                                        {TEXT("help"), TEXT("Aid")},
	                                                        {TEXT("trade"), TEXT("Trade")},
	                                                        {TEXT("buy"), TEXT("Trade")},
	                                                        {TEXT("sell"), TEXT("Trade")},
	                                                        {TEXT("money"), TEXT("Trade")},
	                                                        {TEXT("weapon"), TEXT("Weapons")},
	                                                        {TEXT("armor"), TEXT("Equipment")},
	                                                        {TEXT("food"), TEXT("Supplies")},
	                                                        {TEXT("water"), TEXT("Supplies")},
	                                                        {TEXT("danger"), TEXT("Danger")},
	                                                        {TEXT("threat"), TEXT("Danger")},
	                                                        {TEXT("bandit"), TEXT("Bandits")},
	                                                        {TEXT("survivor"), TEXT("Survivors")},
	                                                        {TEXT("military"), TEXT("Military")},
	                                                        {TEXT("family"), TEXT("Personal")},
	                                                        {TEXT("home"), TEXT("Personal")},
	                                                        {TEXT("weather"), TEXT("Environment")},
	                                                        {TEXT("island"), TEXT("Location")}};

	for (const auto& Pair : TopicKeywords)
	{
		if (LowerText.Contains(Pair.Key))
		{
			Topics.AddUnique(Pair.Value);
		}
	}

	return Topics;
}
