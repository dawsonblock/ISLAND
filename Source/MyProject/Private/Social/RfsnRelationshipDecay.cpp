// RFSN Relationship Decay Implementation

#include "RfsnRelationshipDecay.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"

URfsnRelationshipDecay::URfsnRelationshipDecay()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnRelationshipDecay::BeginPlay()
{
	Super::BeginPlay();
	UpdateTier();
	RFSN_LOG(TEXT("RelationshipDecay initialized for %s (value: %.1f)"), *GetOwner()->GetName(), CurrentValue);
}

void URfsnRelationshipDecay::ModifyRelationship(float Amount, const FString& Reason)
{
	float OldValue = CurrentValue;
	CurrentValue = FMath::Clamp(CurrentValue + Amount, MinValue, MaxValue);

	// Check for lock
	if (CurrentValue >= LockThreshold)
	{
		bIsLocked = true;
	}

	// Record bonus
	if (Amount != 0.0f && !Reason.IsEmpty())
	{
		FRfsnRelationshipBonus Bonus;
		Bonus.Reason = Reason;
		Bonus.Amount = Amount;
		Bonus.Timestamp = FDateTime::Now();
		RecentBonuses.Add(Bonus);

		// Keep only recent bonuses
		while (RecentBonuses.Num() > 10)
		{
			RecentBonuses.RemoveAt(0);
		}
	}

	UpdateTier();

	if (Amount < 0 && FMath::Abs(Amount) > 1.0f)
	{
		OnRelationshipDecayed.Broadcast(OldValue, CurrentValue);
	}

	RFSN_LOG(TEXT("%s relationship: %.1f -> %.1f (%s)"), *GetOwner()->GetName(), OldValue, CurrentValue, *Reason);
}

void URfsnRelationshipDecay::RecordInteraction()
{
	HoursSinceInteraction = 0.0f;
}

void URfsnRelationshipDecay::GiveGift(float Value)
{
	ModifyRelationship(Value, TEXT("Gift"));
	RecordInteraction();
}

void URfsnRelationshipDecay::DoFavor(float Value)
{
	ModifyRelationship(Value, TEXT("Favor"));
	RecordInteraction();
}

void URfsnRelationshipDecay::Insult(float Value)
{
	ModifyRelationship(Value, TEXT("Insult"));
	RecordInteraction();
}

void URfsnRelationshipDecay::Betray(float Value)
{
	ModifyRelationship(Value, TEXT("Betrayal"));
	bIsLocked = false; // Betrayal breaks lock
	RecordInteraction();
}

void URfsnRelationshipDecay::TickDecay(float GameHoursElapsed)
{
	if (bIsLocked)
	{
		return; // No decay for best friends
	}

	HoursSinceInteraction += GameHoursElapsed;

	// Check grace period
	if (HoursSinceInteraction < DecayGracePeriodHours)
	{
		return;
	}

	// Calculate decay
	float DaysElapsed = GameHoursElapsed / 24.0f;
	float DecayRate = CalculateDecayRate();
	float DecayAmount = DecayRate * DaysElapsed;

	if (DecayAmount > 0.0f)
	{
		float OldValue = CurrentValue;

		// Decay toward neutral
		if (CurrentValue > 0)
		{
			CurrentValue = FMath::Max(0.0f, CurrentValue - DecayAmount);
		}
		else if (CurrentValue < 0)
		{
			CurrentValue = FMath::Min(0.0f, CurrentValue + DecayAmount);
		}

		if (FMath::Abs(CurrentValue - OldValue) >= 1.0f)
		{
			UpdateTier();
			OnRelationshipDecayed.Broadcast(OldValue, CurrentValue);
		}
	}
}

ERfsnRelationshipTier URfsnRelationshipDecay::ValueToTier(float Value)
{
	if (Value >= 100.0f)
		return ERfsnRelationshipTier::BestFriend;
	if (Value >= 60.0f)
		return ERfsnRelationshipTier::Trusted;
	if (Value >= 20.0f)
		return ERfsnRelationshipTier::Friendly;
	if (Value >= -20.0f)
		return ERfsnRelationshipTier::Neutral;
	if (Value >= -60.0f)
		return ERfsnRelationshipTier::Unfriendly;
	return ERfsnRelationshipTier::Hostile;
}

FString URfsnRelationshipDecay::TierToString(ERfsnRelationshipTier Tier)
{
	switch (Tier)
	{
	case ERfsnRelationshipTier::Hostile:
		return TEXT("Hostile");
	case ERfsnRelationshipTier::Unfriendly:
		return TEXT("Unfriendly");
	case ERfsnRelationshipTier::Neutral:
		return TEXT("Neutral");
	case ERfsnRelationshipTier::Friendly:
		return TEXT("Friendly");
	case ERfsnRelationshipTier::Trusted:
		return TEXT("Trusted");
	case ERfsnRelationshipTier::BestFriend:
		return TEXT("Best Friend");
	default:
		return TEXT("Unknown");
	}
}

FString URfsnRelationshipDecay::GetRelationshipContext() const
{
	FString Context = FString::Printf(TEXT("Relationship: %s (%.0f). "), *TierToString(CurrentTier), CurrentValue);

	if (bIsLocked)
	{
		Context += TEXT("This is a deep friendship that won't fade. ");
	}
	else if (HoursSinceInteraction > DecayGracePeriodHours * 2)
	{
		Context += TEXT("We haven't talked in a long time. ");
	}

	if (RecentBonuses.Num() > 0)
	{
		const FRfsnRelationshipBonus& Recent = RecentBonuses.Last();
		if (Recent.Amount > 0)
		{
			Context += FString::Printf(TEXT("Recently had a positive interaction (%s). "), *Recent.Reason);
		}
		else if (Recent.Amount < 0)
		{
			Context += FString::Printf(TEXT("Recently had a conflict (%s). "), *Recent.Reason);
		}
	}

	return Context;
}

FString URfsnRelationshipDecay::GetDecayWarning() const
{
	if (bIsLocked)
	{
		return TEXT("");
	}

	if (HoursSinceInteraction > DecayGracePeriodHours)
	{
		float DaysWithoutContact = (HoursSinceInteraction - DecayGracePeriodHours) / 24.0f;
		if (DaysWithoutContact > 3.0f)
		{
			return FString::Printf(TEXT("%s relationship is fading (%.0f days without contact)."),
			                       *TierToString(CurrentTier), DaysWithoutContact);
		}
	}

	return TEXT("");
}

void URfsnRelationshipDecay::SetInitialValue(float Value)
{
	CurrentValue = FMath::Clamp(Value, MinValue, MaxValue);
	if (CurrentValue >= LockThreshold)
	{
		bIsLocked = true;
	}
	UpdateTier();
}

void URfsnRelationshipDecay::UpdateTier()
{
	ERfsnRelationshipTier NewTier = ValueToTier(CurrentValue);
	if (NewTier != CurrentTier)
	{
		ERfsnRelationshipTier OldTier = CurrentTier;
		CurrentTier = NewTier;
		OnTierChanged.Broadcast(OldTier, NewTier);

		RFSN_LOG(TEXT("%s tier changed: %s -> %s"), *GetOwner()->GetName(), *TierToString(OldTier),
		         *TierToString(NewTier));
	}
}

float URfsnRelationshipDecay::CalculateDecayRate() const
{
	float Rate = DecayRatePerDay;

	if (CurrentValue > 0)
	{
		// Positive relationships decay slower
		Rate *= PositiveDecayMultiplier;
	}
	else if (CurrentValue < 0)
	{
		// Negative relationships move toward neutral faster
		Rate *= NegativeDecayMultiplier;
	}

	// Higher tier = slower decay
	switch (CurrentTier)
	{
	case ERfsnRelationshipTier::Trusted:
		Rate *= 0.5f;
		break;
	case ERfsnRelationshipTier::Friendly:
		Rate *= 0.75f;
		break;
	default:
		break;
	}

	return Rate;
}
