// RFSN NPC Portrait Implementation

#include "RfsnNpcPortrait.h"
#include "RfsnBackstoryGenerator.h"
#include "RfsnEmotionBlend.h"
#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

URfsnNpcPortrait::URfsnNpcPortrait()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Update 10x per second max
}

void URfsnNpcPortrait::BeginPlay()
{
	Super::BeginPlay();

	// Cache sibling components
	NpcClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>();
	EmotionBlend = GetOwner()->FindComponentByClass<URfsnEmotionBlend>();
	BackstoryGen = GetOwner()->FindComponentByClass<URfsnBackstoryGenerator>();

	// Initial data refresh
	RefreshPortraitData();

	RFSN_LOG(TEXT("NpcPortrait initialized for %s"), *GetOwner()->GetName());
}

void URfsnNpcPortrait::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUpdateRealtime)
	{
		RefreshPortraitData();
	}
}

void URfsnNpcPortrait::RefreshPortraitData()
{
	FRfsnPortraitData NewData;

	// Get name from NPC client
	if (NpcClient)
	{
		NewData.NpcName = NpcClient->NpcName;
		NewData.RelationshipTier = NpcClient->Relationship;
		NewData.Affinity = NpcClient->Affinity;
		NewData.Mood = NpcClient->Mood;
	}
	else
	{
		NewData.NpcName = GetOwner()->GetName();
	}

	// Get emotion data
	if (EmotionBlend)
	{
		NewData.Mood = EmotionBlend->ToMoodString();
		NewData.DominantEmotion = EmotionBlend->EmotionToString(EmotionBlend->DominantEmotion);
	}

	// Get backstory data
	if (BackstoryGen && BackstoryGen->HasBackstory())
	{
		NewData.Occupation = BackstoryGen->CachedBackstory.Occupation;
		NewData.ShortContext = BackstoryGen->GetShortContext();
	}

	// Get faction info
	NewData.FactionName = FactionId;
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (URfsnFactionSystem* FactionSys = GI->GetSubsystem<URfsnFactionSystem>())
		{
			NewData.RelationshipTier = FactionSys->GetReputationTier(FactionId);
		}
	}

	// Set portrait
	NewData.Portrait = PortraitTexture;

	// Calculate border color
	if (bUseFactionColor)
	{
		NewData.BorderColor = GetFactionColor(FactionId);
	}
	else
	{
		NewData.BorderColor = CustomBorderColor;
	}

	// Blend emotion color into border
	FLinearColor EmotionCol = GetEmotionColor();
	NewData.BorderColor = FLinearColor::LerpUsingHSV(NewData.BorderColor, EmotionCol, 0.3f);

	// Check if data changed
	bool bChanged = (NewData.NpcName != CachedData.NpcName) || (NewData.Mood != CachedData.Mood) ||
	                (NewData.DominantEmotion != CachedData.DominantEmotion) ||
	                (NewData.RelationshipTier != CachedData.RelationshipTier);

	CachedData = NewData;

	if (bChanged)
	{
		OnPortraitUpdated.Broadcast(CachedData);
	}
}

FLinearColor URfsnNpcPortrait::GetFactionColor(const FString& InFactionId) const
{
	FString Lower = InFactionId.ToLower();

	if (Lower == TEXT("survivors"))
	{
		return FLinearColor(0.2f, 0.6f, 0.3f); // Green
	}
	if (Lower == TEXT("bandits"))
	{
		return FLinearColor(0.7f, 0.2f, 0.2f); // Dark Red
	}
	if (Lower == TEXT("military"))
	{
		return FLinearColor(0.2f, 0.3f, 0.6f); // Navy Blue
	}
	if (Lower == TEXT("merchants"))
	{
		return FLinearColor(0.7f, 0.6f, 0.2f); // Gold
	}
	if (Lower == TEXT("cultists"))
	{
		return FLinearColor(0.5f, 0.2f, 0.6f); // Purple
	}

	return FLinearColor(0.5f, 0.5f, 0.5f); // Gray for unknown
}

FLinearColor URfsnNpcPortrait::GetEmotionColor() const
{
	if (!EmotionBlend)
	{
		return FLinearColor::White;
	}

	switch (EmotionBlend->DominantEmotion)
	{
	case ERfsnCoreEmotion::Joy:
		return FLinearColor(1.0f, 0.9f, 0.3f); // Yellow
	case ERfsnCoreEmotion::Trust:
		return FLinearColor(0.3f, 0.8f, 0.4f); // Light Green
	case ERfsnCoreEmotion::Fear:
		return FLinearColor(0.4f, 0.2f, 0.5f); // Dark Purple
	case ERfsnCoreEmotion::Surprise:
		return FLinearColor(0.3f, 0.7f, 0.9f); // Cyan
	case ERfsnCoreEmotion::Sadness:
		return FLinearColor(0.3f, 0.4f, 0.7f); // Blue
	case ERfsnCoreEmotion::Disgust:
		return FLinearColor(0.5f, 0.6f, 0.2f); // Olive
	case ERfsnCoreEmotion::Anger:
		return FLinearColor(0.9f, 0.2f, 0.2f); // Red
	case ERfsnCoreEmotion::Anticipation:
		return FLinearColor(0.9f, 0.6f, 0.2f); // Orange
	case ERfsnCoreEmotion::Neutral:
	default:
		return FLinearColor(0.7f, 0.7f, 0.7f); // Gray
	}
}
