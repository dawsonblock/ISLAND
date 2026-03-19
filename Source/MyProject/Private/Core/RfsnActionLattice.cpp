// RFSN Action Lattice Implementation

#include "RfsnActionLattice.h"
#include "RfsnLogging.h"

// ─────────────────────────────────────────────────────────────
// FRfsnExpandedAction
// ─────────────────────────────────────────────────────────────

FString FRfsnExpandedAction::ToPromptHint() const
{
	FString Result = URfsnActionLattice::ActionToString(BaseAction);

	// Add modifiers
	FString IntensityMod = URfsnActionLattice::IntensityToModifier(Intensity);
	FString ComplianceMod = URfsnActionLattice::ComplianceToModifier(Compliance);
	FString MotiveMod = URfsnActionLattice::MotiveToModifier(Motive);

	if (!IntensityMod.IsEmpty())
	{
		Result = IntensityMod + TEXT(" ") + Result;
	}

	if (!ComplianceMod.IsEmpty())
	{
		Result += TEXT(", ") + ComplianceMod;
	}

	if (!MotiveMod.IsEmpty())
	{
		Result += TEXT(" (") + MotiveMod + TEXT(")");
	}

	if (!Qualifier.IsEmpty())
	{
		Result += TEXT(" ") + Qualifier;
	}

	return Result;
}

FRfsnExpandedAction FRfsnExpandedAction::Simple(ERfsnNpcAction Action)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = Action;
	return Result;
}

FRfsnExpandedAction FRfsnExpandedAction::Hesitant(ERfsnNpcAction Action)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = Action;
	Result.Intensity = ERfsnActionIntensity::Subdued;
	Result.Qualifier = TEXT("with hesitation");
	return Result;
}

FRfsnExpandedAction FRfsnExpandedAction::Reluctant(ERfsnNpcAction Action)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = Action;
	Result.Compliance = ERfsnActionCompliance::Reluctant;
	Result.Motive = ERfsnActionMotive::Conflicted;
	return Result;
}

FRfsnExpandedAction FRfsnExpandedAction::Calculated(ERfsnNpcAction Action)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = Action;
	Result.Motive = ERfsnActionMotive::Calculated;
	Result.Intensity = ERfsnActionIntensity::Normal;
	return Result;
}

FRfsnExpandedAction FRfsnExpandedAction::Conflicted(ERfsnNpcAction Action, ERfsnNpcAction AlternateInclination)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = Action;
	Result.Motive = ERfsnActionMotive::Conflicted;
	Result.Compliance = ERfsnActionCompliance::Partial;
	Result.Qualifier = FString::Printf(TEXT("while wanting to %s"),
	                                   *URfsnActionLattice::ActionToString(AlternateInclination).ToLower());
	return Result;
}

// ─────────────────────────────────────────────────────────────
// URfsnActionLattice
// ─────────────────────────────────────────────────────────────

FRfsnExpandedAction URfsnActionLattice::BuildAction(ERfsnNpcAction BaseAction, float Affinity, float ActionBias,
                                                    bool bHasNegativeMemory)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = BaseAction;

	// Determine intensity based on affinity
	if (Affinity > 0.5f)
	{
		Result.Intensity = ERfsnActionIntensity::Emphatic;
	}
	else if (Affinity < -0.5f)
	{
		Result.Intensity = ERfsnActionIntensity::Subdued;
	}
	else
	{
		Result.Intensity = ERfsnActionIntensity::Normal;
	}

	// Negative memory adds hesitation
	if (bHasNegativeMemory)
	{
		if (Result.Intensity == ERfsnActionIntensity::Normal)
		{
			Result.Intensity = ERfsnActionIntensity::Subdued;
		}
		Result.Motive = ERfsnActionMotive::Guarded;
	}

	// Strong negative bias suggests reluctance
	if (ActionBias < -0.3f)
	{
		Result.Compliance = ERfsnActionCompliance::Reluctant;
		Result.Motive = ERfsnActionMotive::Conflicted;
	}
	// Strong positive bias suggests sincerity
	else if (ActionBias > 0.3f)
	{
		Result.Motive = ERfsnActionMotive::Sincere;
	}

	return Result;
}

FRfsnExpandedAction URfsnActionLattice::ApplyContextModifiers(ERfsnNpcAction BaseAction, const FString& Mood,
                                                              const FString& Relationship, float Affinity,
                                                              const TArray<FRfsnActionBias>& Biases)
{
	FRfsnExpandedAction Result;
	Result.BaseAction = BaseAction;

	// Find bias for this action
	float Bias = 0.0f;
	for (const FRfsnActionBias& B : Biases)
	{
		if (B.Action == BaseAction)
		{
			Bias = B.Bias * B.Confidence;
			break;
		}
	}

	// Mood-based modifiers
	if (Mood.Contains(TEXT("Hostile")) || Mood.Contains(TEXT("Angry")))
	{
		Result.Intensity = ERfsnActionIntensity::Emphatic;
		if (BaseAction == ERfsnNpcAction::Help || BaseAction == ERfsnNpcAction::Offer)
		{
			Result.Motive = ERfsnActionMotive::Calculated;
		}
	}
	else if (Mood.Contains(TEXT("Fearful")) || Mood.Contains(TEXT("Cautious")))
	{
		Result.Intensity = ERfsnActionIntensity::Subdued;
		Result.Motive = ERfsnActionMotive::Guarded;
	}

	// Relationship-based modifiers
	if (Relationship == TEXT("Enemy"))
	{
		if (BaseAction == ERfsnNpcAction::Trade || BaseAction == ERfsnNpcAction::Help)
		{
			Result.Compliance = ERfsnActionCompliance::Reluctant;
		}
	}
	else if (Relationship == TEXT("Stranger"))
	{
		if (FMath::Abs(Affinity) < 0.2f)
		{
			Result.Motive = ERfsnActionMotive::Guarded;
		}
	}

	// Bias-based adjustments
	if (Bias < -0.2f)
	{
		Result.Compliance = ERfsnActionCompliance::Partial;
	}

	return Result;
}

TArray<FRfsnExpandedAction> URfsnActionLattice::GetValidActions(const FString& Mood, const FString& Relationship,
                                                                float Affinity, const FString& PlayerSignal)
{
	TArray<FRfsnExpandedAction> Actions;

	// Core social actions always available
	TArray<ERfsnNpcAction> BaseActions = {ERfsnNpcAction::Talk, ERfsnNpcAction::Greet, ERfsnNpcAction::Explain,
	                                      ERfsnNpcAction::Inquire};

	// Add context-appropriate actions
	if (Affinity > 0.0f)
	{
		BaseActions.Add(ERfsnNpcAction::Help);
		BaseActions.Add(ERfsnNpcAction::Offer);
		BaseActions.Add(ERfsnNpcAction::Agree);
	}

	if (Affinity < 0.0f || Mood.Contains(TEXT("Hostile")))
	{
		BaseActions.Add(ERfsnNpcAction::Warn);
		BaseActions.Add(ERfsnNpcAction::Threaten);
		BaseActions.Add(ERfsnNpcAction::Disagree);
		BaseActions.Add(ERfsnNpcAction::Refuse);
	}

	if (Relationship == TEXT("Merchant") || Relationship == TEXT("Trader"))
	{
		BaseActions.Add(ERfsnNpcAction::Trade);
	}

	// Generate expanded actions with variations
	for (ERfsnNpcAction BaseAction : BaseActions)
	{
		// Standard version
		Actions.Add(FRfsnExpandedAction::Simple(BaseAction));

		// Hesitant version for uncertain contexts
		if (FMath::Abs(Affinity) < 0.3f)
		{
			Actions.Add(FRfsnExpandedAction::Hesitant(BaseAction));
		}

		// Reluctant version for low affinity helpful actions
		if (Affinity < 0.0f && (BaseAction == ERfsnNpcAction::Help || BaseAction == ERfsnNpcAction::Trade))
		{
			Actions.Add(FRfsnExpandedAction::Reluctant(BaseAction));
		}
	}

	return Actions;
}

float URfsnActionLattice::ScoreAction(const FRfsnExpandedAction& Action, const FString& Mood, float Affinity)
{
	float Score = 0.5f; // Base score

	// Affinity alignment
	bool bPositiveAction = (Action.BaseAction == ERfsnNpcAction::Help || Action.BaseAction == ERfsnNpcAction::Greet ||
	                        Action.BaseAction == ERfsnNpcAction::Offer || Action.BaseAction == ERfsnNpcAction::Agree);

	if (bPositiveAction)
	{
		Score += Affinity * 0.3f;
	}
	else
	{
		Score -= Affinity * 0.2f;
	}

	// Intensity appropriateness
	if (Action.Intensity == ERfsnActionIntensity::Subdued && Affinity < 0.0f)
	{
		Score += 0.1f; // Subdued is appropriate for low affinity
	}
	else if (Action.Intensity == ERfsnActionIntensity::Emphatic && Affinity > 0.5f)
	{
		Score += 0.1f; // Emphatic is appropriate for high affinity
	}

	// Mood alignment
	if (Mood.Contains(TEXT("Friendly")) && bPositiveAction)
	{
		Score += 0.15f;
	}
	else if (Mood.Contains(TEXT("Hostile")) && !bPositiveAction)
	{
		Score += 0.15f;
	}

	return FMath::Clamp(Score, 0.0f, 1.0f);
}

FString URfsnActionLattice::ActionToString(ERfsnNpcAction Action)
{
	switch (Action)
	{
	case ERfsnNpcAction::Greet:
		return TEXT("Greet");
	case ERfsnNpcAction::Warn:
		return TEXT("Warn");
	case ERfsnNpcAction::Idle:
		return TEXT("Idle");
	case ERfsnNpcAction::Flee:
		return TEXT("Flee");
	case ERfsnNpcAction::Attack:
		return TEXT("Attack");
	case ERfsnNpcAction::Trade:
		return TEXT("Trade");
	case ERfsnNpcAction::Offer:
		return TEXT("Offer");
	case ERfsnNpcAction::Talk:
		return TEXT("Talk");
	case ERfsnNpcAction::Apologize:
		return TEXT("Apologize");
	case ERfsnNpcAction::Threaten:
		return TEXT("Threaten");
	case ERfsnNpcAction::Explain:
		return TEXT("Explain");
	case ERfsnNpcAction::Answer:
		return TEXT("Answer");
	case ERfsnNpcAction::Inquire:
		return TEXT("Inquire");
	case ERfsnNpcAction::Help:
		return TEXT("Help");
	case ERfsnNpcAction::Request:
		return TEXT("Request");
	case ERfsnNpcAction::Agree:
		return TEXT("Agree");
	case ERfsnNpcAction::Disagree:
		return TEXT("Disagree");
	case ERfsnNpcAction::Accept:
		return TEXT("Accept");
	case ERfsnNpcAction::Refuse:
		return TEXT("Refuse");
	case ERfsnNpcAction::Ignore:
		return TEXT("Ignore");
	default:
		return TEXT("Talk");
	}
}

FString URfsnActionLattice::IntensityToModifier(ERfsnActionIntensity Intensity)
{
	switch (Intensity)
	{
	case ERfsnActionIntensity::Subdued:
		return TEXT("hesitantly");
	case ERfsnActionIntensity::Emphatic:
		return TEXT("emphatically");
	default:
		return TEXT("");
	}
}

FString URfsnActionLattice::ComplianceToModifier(ERfsnActionCompliance Compliance)
{
	switch (Compliance)
	{
	case ERfsnActionCompliance::Partial:
		return TEXT("only partially");
	case ERfsnActionCompliance::Reluctant:
		return TEXT("reluctantly");
	case ERfsnActionCompliance::Deferred:
		return TEXT("promising to do so later");
	default:
		return TEXT("");
	}
}

FString URfsnActionLattice::MotiveToModifier(ERfsnActionMotive Motive)
{
	switch (Motive)
	{
	case ERfsnActionMotive::Guarded:
		return TEXT("guarded");
	case ERfsnActionMotive::Calculated:
		return TEXT("with ulterior motive");
	case ERfsnActionMotive::Conflicted:
		return TEXT("conflicted");
	default:
		return TEXT("");
	}
}
