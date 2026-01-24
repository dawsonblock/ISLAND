// RFSN Quest Integration Implementation

#include "RfsnQuestIntegration.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"

URfsnQuestIntegration::URfsnQuestIntegration()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnQuestIntegration::BeginPlay()
{
	Super::BeginPlay();
	RFSN_LOG(TEXT("QuestIntegration initialized for %s with %d quests"), *GetOwner()->GetName(), OfferedQuests.Num());
}

bool URfsnQuestIntegration::StartQuest(const FString& QuestId)
{
	FRfsnQuest* Quest = FindQuest(QuestId);
	if (!Quest)
	{
		return false;
	}

	if (Quest->Status != ERfsnQuestStatus::Unknown && Quest->Status != ERfsnQuestStatus::Available)
	{
		return false;
	}

	Quest->Status = ERfsnQuestStatus::Active;
	OnQuestStatusChanged.Broadcast(QuestId, ERfsnQuestStatus::Active);

	RFSN_LOG(TEXT("Quest started: %s"), *Quest->DisplayName);
	return true;
}

bool URfsnQuestIntegration::CompleteQuest(const FString& QuestId)
{
	FRfsnQuest* Quest = FindQuest(QuestId);
	if (!Quest || Quest->Status != ERfsnQuestStatus::Active)
	{
		return false;
	}

	if (!Quest->IsComplete())
	{
		return false;
	}

	Quest->Status = ERfsnQuestStatus::Completed;
	OnQuestStatusChanged.Broadcast(QuestId, ERfsnQuestStatus::Completed);

	RFSN_LOG(TEXT("Quest completed: %s"), *Quest->DisplayName);
	return true;
}

bool URfsnQuestIntegration::FailQuest(const FString& QuestId)
{
	FRfsnQuest* Quest = FindQuest(QuestId);
	if (!Quest || Quest->Status != ERfsnQuestStatus::Active)
	{
		return false;
	}

	Quest->Status = ERfsnQuestStatus::Failed;
	OnQuestStatusChanged.Broadcast(QuestId, ERfsnQuestStatus::Failed);

	RFSN_LOG(TEXT("Quest failed: %s"), *Quest->DisplayName);
	return true;
}

bool URfsnQuestIntegration::UpdateObjective(const FString& QuestId, const FString& ObjectiveId, int32 Progress)
{
	FRfsnQuest* Quest = FindQuest(QuestId);
	if (!Quest || Quest->Status != ERfsnQuestStatus::Active)
	{
		return false;
	}

	for (FRfsnQuestObjective& Obj : Quest->Objectives)
	{
		if (Obj.ObjectiveId == ObjectiveId)
		{
			Obj.CurrentProgress = FMath::Min(Obj.CurrentProgress + Progress, Obj.RequiredProgress);
			OnObjectiveProgress.Broadcast(QuestId, ObjectiveId);

			// Check for quest completion
			if (Quest->IsComplete())
			{
				CompleteQuest(QuestId);
			}

			return true;
		}
	}

	return false;
}

ERfsnQuestStatus URfsnQuestIntegration::GetQuestStatus(const FString& QuestId) const
{
	const FRfsnQuest* Quest = FindQuest(QuestId);
	return Quest ? Quest->Status : ERfsnQuestStatus::Unknown;
}

FRfsnQuest URfsnQuestIntegration::GetQuest(const FString& QuestId) const
{
	const FRfsnQuest* Quest = FindQuest(QuestId);
	return Quest ? *Quest : FRfsnQuest();
}

TArray<FRfsnQuest> URfsnQuestIntegration::GetAvailableQuests() const
{
	TArray<FRfsnQuest> Result;
	for (const FRfsnQuest& Quest : OfferedQuests)
	{
		if (Quest.Status == ERfsnQuestStatus::Available || Quest.Status == ERfsnQuestStatus::Unknown)
		{
			Result.Add(Quest);
		}
	}
	return Result;
}

TArray<FRfsnQuest> URfsnQuestIntegration::GetActiveQuests() const
{
	TArray<FRfsnQuest> Result;
	for (const FRfsnQuest& Quest : OfferedQuests)
	{
		if (Quest.Status == ERfsnQuestStatus::Active)
		{
			Result.Add(Quest);
		}
	}
	return Result;
}

FString URfsnQuestIntegration::GetQuestContext() const
{
	FString Context;

	// Available quests
	TArray<FRfsnQuest> Available = GetAvailableQuests();
	if (Available.Num() > 0)
	{
		Context += TEXT("NPC can offer quests: ");
		for (const FRfsnQuest& Quest : Available)
		{
			Context += FString::Printf(TEXT("[%s] "), *Quest.DisplayName);
		}
	}

	// Active quests
	TArray<FRfsnQuest> Active = GetActiveQuests();
	if (Active.Num() > 0)
	{
		Context += TEXT("Active quests from this NPC: ");
		for (const FRfsnQuest& Quest : Active)
		{
			int32 Complete = 0, Total = 0;
			for (const FRfsnQuestObjective& Obj : Quest.Objectives)
			{
				if (!Obj.bOptional)
				{
					Total++;
					if (Obj.IsComplete())
						Complete++;
				}
			}
			Context += FString::Printf(TEXT("[%s: %d/%d objectives] "), *Quest.DisplayName, Complete, Total);
		}
	}

	// Quest knowledge
	for (const FRfsnNpcQuestKnowledge& Knowledge : QuestKnowledge)
	{
		if (Knowledge.bCanProvideInfo && !Knowledge.DialogueHint.IsEmpty())
		{
			Context += FString::Printf(TEXT("Knows about quest '%s'. "), *Knowledge.QuestId);
		}
	}

	return Context;
}

FString URfsnQuestIntegration::GetQuestDialogueHint(const FString& QuestId) const
{
	const FRfsnQuest* Quest = FindQuest(QuestId);
	if (!Quest)
	{
		// Check knowledge
		const FRfsnNpcQuestKnowledge* Knowledge = FindQuestKnowledge(QuestId);
		if (Knowledge && !Knowledge->DialogueHint.IsEmpty())
		{
			return Knowledge->DialogueHint;
		}
		return TEXT("");
	}

	switch (Quest->Status)
	{
	case ERfsnQuestStatus::Available:
	case ERfsnQuestStatus::Unknown:
		return Quest->AvailableHint;
	case ERfsnQuestStatus::Active:
		return Quest->ActiveHint;
	case ERfsnQuestStatus::Completed:
		return Quest->CompletionHint;
	default:
		return TEXT("");
	}
}

bool URfsnQuestIntegration::HasAvailableQuests() const
{
	return GetAvailableQuests().Num() > 0;
}

bool URfsnQuestIntegration::HasActiveQuests() const
{
	return GetActiveQuests().Num() > 0;
}

bool URfsnQuestIntegration::KnowsAboutQuest(const FString& QuestId) const
{
	if (FindQuest(QuestId))
	{
		return true;
	}
	return FindQuestKnowledge(QuestId) != nullptr;
}

FString URfsnQuestIntegration::GetQuestInfo(const FString& QuestId) const
{
	const FRfsnQuest* Quest = FindQuest(QuestId);
	if (Quest)
	{
		return Quest->Description;
	}

	const FRfsnNpcQuestKnowledge* Knowledge = FindQuestKnowledge(QuestId);
	if (Knowledge && Knowledge->bCanProvideInfo)
	{
		return Knowledge->DialogueHint;
	}

	return TEXT("");
}

TArray<FString> URfsnQuestIntegration::DetectQuestTopics(const FString& PlayerDialogue) const
{
	TArray<FString> Topics;
	FString Lower = PlayerDialogue.ToLower();

	// Check for quest keywords
	static TArray<FString> QuestKeywords = {TEXT("quest"),   TEXT("mission"), TEXT("task"),        TEXT("job"),
	                                        TEXT("help"),    TEXT("need"),    TEXT("looking for"), TEXT("find"),
	                                        TEXT("deliver"), TEXT("kill"),    TEXT("collect"),     TEXT("retrieve")};

	for (const FString& Keyword : QuestKeywords)
	{
		if (Lower.Contains(Keyword))
		{
			Topics.AddUnique(TEXT("Quest"));
			break;
		}
	}

	// Check for specific quest mentions
	for (const FRfsnQuest& Quest : OfferedQuests)
	{
		if (Lower.Contains(Quest.DisplayName.ToLower()) || Lower.Contains(Quest.QuestId.ToLower()))
		{
			Topics.AddUnique(Quest.QuestId);
		}
	}

	return Topics;
}

FRfsnQuest* URfsnQuestIntegration::FindQuest(const FString& QuestId)
{
	return OfferedQuests.FindByPredicate([&QuestId](const FRfsnQuest& Quest)
	                                     { return Quest.QuestId.Equals(QuestId, ESearchCase::IgnoreCase); });
}

const FRfsnQuest* URfsnQuestIntegration::FindQuest(const FString& QuestId) const
{
	return OfferedQuests.FindByPredicate([&QuestId](const FRfsnQuest& Quest)
	                                     { return Quest.QuestId.Equals(QuestId, ESearchCase::IgnoreCase); });
}

const FRfsnNpcQuestKnowledge* URfsnQuestIntegration::FindQuestKnowledge(const FString& QuestId) const
{
	return QuestKnowledge.FindByPredicate([&QuestId](const FRfsnNpcQuestKnowledge& Knowledge)
	                                      { return Knowledge.QuestId.Equals(QuestId, ESearchCase::IgnoreCase); });
}
