// RFSN NPC Needs Implementation

#include "RfsnNpcNeeds.h"
#include "RfsnEmotionBlend.h"
#include "RfsnLogging.h"

URfsnNpcNeeds::URfsnNpcNeeds()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Update once per second

	// Default decay rates (per game hour)
	Hunger.DecayRate = 4.0f;  // Gets hungry every ~25 game hours
	Energy.DecayRate = 4.2f;  // Needs sleep every ~24 game hours
	Social.DecayRate = 2.0f;  // Needs interaction every ~50 game hours
	Safety.DecayRate = 1.0f;  // Slow safety decay
	Purpose.DecayRate = 3.0f; // Needs activity regularly
}

void URfsnNpcNeeds::BeginPlay()
{
	Super::BeginPlay();
	RFSN_LOG(TEXT("NpcNeeds initialized for %s"), *GetOwner()->GetName());
}

void URfsnNpcNeeds::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled)
	{
		return;
	}

	// Accumulate time and convert to game hours
	TimeAccumulator += DeltaTime * TimeScale;

	// Process in 1-minute game time chunks
	float GameMinutes = TimeAccumulator / 60.0f;
	if (GameMinutes >= 1.0f)
	{
		float GameHours = GameMinutes / 60.0f;
		TimeAccumulator = 0.0f;
		UpdateNeeds(GameHours);
	}
}

void URfsnNpcNeeds::UpdateNeeds(float GameHours)
{
	// Decay all needs
	Hunger.Decay(GameHours);
	Energy.Decay(GameHours);
	Social.Decay(GameHours);
	Safety.Decay(GameHours);
	Purpose.Decay(GameHours);

	// Check for critical needs
	if (Hunger.IsCritical())
	{
		OnNeedCritical.Broadcast(FName("Hunger"));
	}
	if (Energy.IsCritical())
	{
		OnNeedCritical.Broadcast(FName("Energy"));
	}
	if (Social.IsCritical())
	{
		OnNeedCritical.Broadcast(FName("Social"));
	}

	// Update state
	ERfsnNeedState NewState = CalculateState();
	if (NewState != CurrentState)
	{
		ERfsnNeedState OldState = CurrentState;
		CurrentState = NewState;
		OnNeedStateChanged.Broadcast(NewState, OldState);
	}

	UpdateWellbeing();
}

void URfsnNpcNeeds::Feed(float Amount)
{
	Hunger.Satisfy(Amount);
	RFSN_LOG(TEXT("%s fed (Hunger: %.1f)"), *GetOwner()->GetName(), Hunger.Value);
}

void URfsnNpcNeeds::Rest(float Amount)
{
	Energy.Satisfy(Amount);
	RFSN_LOG(TEXT("%s rested (Energy: %.1f)"), *GetOwner()->GetName(), Energy.Value);
}

void URfsnNpcNeeds::Socialize(float Amount)
{
	Social.Satisfy(Amount);
	RFSN_LOG(TEXT("%s socialized (Social: %.1f)"), *GetOwner()->GetName(), Social.Value);
}

void URfsnNpcNeeds::FeelSafe(float Amount)
{
	Safety.Satisfy(Amount);
}

void URfsnNpcNeeds::FeelThreatened(float Amount)
{
	Safety.Value = FMath::Max(0.0f, Safety.Value - Amount);
}

void URfsnNpcNeeds::Accomplish(float Amount)
{
	Purpose.Satisfy(Amount);
}

float URfsnNpcNeeds::GetNeedValue(FName NeedName) const
{
	if (NeedName == FName("Hunger"))
		return Hunger.Value;
	if (NeedName == FName("Energy"))
		return Energy.Value;
	if (NeedName == FName("Social"))
		return Social.Value;
	if (NeedName == FName("Safety"))
		return Safety.Value;
	if (NeedName == FName("Purpose"))
		return Purpose.Value;
	return 100.0f;
}

bool URfsnNpcNeeds::HasCriticalNeed() const
{
	return Hunger.IsCritical() || Energy.IsCritical() || Social.IsCritical() || Safety.IsCritical();
}

FName URfsnNpcNeeds::GetMostPressingNeed() const
{
	FName MostPressing = NAME_None;
	float LowestValue = 100.0f;

	if (Hunger.Value < LowestValue)
	{
		LowestValue = Hunger.Value;
		MostPressing = FName("Hunger");
	}
	if (Energy.Value < LowestValue)
	{
		LowestValue = Energy.Value;
		MostPressing = FName("Energy");
	}
	if (Social.Value < LowestValue)
	{
		LowestValue = Social.Value;
		MostPressing = FName("Social");
	}
	if (Safety.Value < LowestValue)
	{
		LowestValue = Safety.Value;
		MostPressing = FName("Safety");
	}
	if (Purpose.Value < LowestValue)
	{
		MostPressing = FName("Purpose");
	}

	return MostPressing;
}

float URfsnNpcNeeds::GetBehaviorModifier() const
{
	// Returns -1 (very negative behavior) to 1 (positive behavior)
	float Modifier = 0.0f;

	if (Hunger.IsCritical())
		Modifier -= 0.3f;
	if (Energy.IsCritical())
		Modifier -= 0.3f;
	if (Social.IsCritical())
		Modifier -= 0.2f;
	if (Safety.IsCritical())
		Modifier -= 0.4f;

	// Bonus for high wellbeing
	if (OverallWellbeing > 80.0f)
		Modifier += 0.2f;

	return FMath::Clamp(Modifier, -1.0f, 1.0f);
}

FString URfsnNpcNeeds::GetNeedsToneModifier() const
{
	TArray<FString> Modifiers;

	if (Hunger.IsCritical())
	{
		Modifiers.Add(TEXT("hungry and distracted"));
	}
	else if (Hunger.NeedsSeeking())
	{
		Modifiers.Add(TEXT("thinking about food"));
	}

	if (Energy.IsCritical())
	{
		Modifiers.Add(TEXT("exhausted"));
	}
	else if (Energy.NeedsSeeking())
	{
		Modifiers.Add(TEXT("tired"));
	}

	if (Social.IsCritical())
	{
		Modifiers.Add(TEXT("desperate for company"));
	}
	else if (Social.NeedsSeeking())
	{
		Modifiers.Add(TEXT("eager to talk"));
	}

	if (Safety.IsCritical())
	{
		Modifiers.Add(TEXT("anxious and paranoid"));
	}

	if (Modifiers.Num() == 0)
	{
		return TEXT("content");
	}

	return FString::Join(Modifiers, TEXT(", "));
}

FString URfsnNpcNeeds::GetNeedsContext() const
{
	FString Context = FString::Printf(TEXT("Physical state: %s. "), *GetNeedsToneModifier());

	if (HasCriticalNeed())
	{
		Context += FString::Printf(TEXT("Urgently needs %s. "), *GetMostPressingNeed().ToString());
	}

	return Context;
}

void URfsnNpcNeeds::ApplyToEmotionBlend()
{
	URfsnEmotionBlend* EmotionBlend = GetOwner()->FindComponentByClass<URfsnEmotionBlend>();
	if (!EmotionBlend)
	{
		return;
	}

	// Apply mood effects based on needs
	if (Hunger.IsCritical())
	{
		EmotionBlend->ApplyStimulus(TEXT("Anger"), 0.2f);
	}

	if (Energy.IsCritical())
	{
		EmotionBlend->ApplyStimulus(TEXT("Sadness"), 0.3f);
	}

	if (Social.IsCritical())
	{
		EmotionBlend->ApplyStimulus(TEXT("Joy"), 0.2f); // Happy to see someone
	}

	if (Safety.IsCritical())
	{
		EmotionBlend->ApplyStimulus(TEXT("Fear"), 0.4f);
	}

	if (OverallWellbeing > 80.0f)
	{
		EmotionBlend->ApplyStimulus(TEXT("Joy"), 0.15f);
	}
}

ERfsnNeedState URfsnNpcNeeds::CalculateState() const
{
	int32 CriticalCount = 0;
	if (Hunger.IsCritical())
		CriticalCount++;
	if (Energy.IsCritical())
		CriticalCount++;
	if (Social.IsCritical())
		CriticalCount++;
	if (Safety.IsCritical())
		CriticalCount++;

	if (CriticalCount >= 2)
	{
		return ERfsnNeedState::Desperate;
	}

	if (CriticalCount >= 1)
	{
		int32 LowCount = 0;
		if (Hunger.NeedsSeeking())
			LowCount++;
		if (Energy.NeedsSeeking())
			LowCount++;
		if (Social.NeedsSeeking())
			LowCount++;

		if (LowCount >= 2)
		{
			return ERfsnNeedState::Stressed;
		}
	}

	// Single need issues
	if (Hunger.IsCritical() || Hunger.NeedsSeeking())
	{
		return ERfsnNeedState::Hungry;
	}

	if (Energy.IsCritical() || Energy.NeedsSeeking())
	{
		return ERfsnNeedState::Tired;
	}

	if (Social.IsCritical() || Social.NeedsSeeking())
	{
		return ERfsnNeedState::Lonely;
	}

	return ERfsnNeedState::Content;
}

void URfsnNpcNeeds::UpdateWellbeing()
{
	OverallWellbeing = (Hunger.Value + Energy.Value + Social.Value + Safety.Value + Purpose.Value) / 5.0f;
}
