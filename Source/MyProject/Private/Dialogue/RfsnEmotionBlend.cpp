// RFSN Emotion Blending Implementation

#include "RfsnEmotionBlend.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "RfsnLogging.h"

// ─────────────────────────────────────────────────────────────
// FRfsnEmotionAxis Implementation
// ─────────────────────────────────────────────────────────────

FRfsnEmotionAxis FRfsnEmotionAxis::Lerp(const FRfsnEmotionAxis& A, const FRfsnEmotionAxis& B, float Alpha)
{
	FRfsnEmotionAxis Result;
	Result.Valence = FMath::Lerp(A.Valence, B.Valence, Alpha);
	Result.Arousal = FMath::Lerp(A.Arousal, B.Arousal, Alpha);
	Result.Dominance = FMath::Lerp(A.Dominance, B.Dominance, Alpha);
	return Result;
}

float FRfsnEmotionAxis::DistanceTo(const FRfsnEmotionAxis& Other) const
{
	float DV = Valence - Other.Valence;
	float DA = Arousal - Other.Arousal;
	float DD = Dominance - Other.Dominance;
	return FMath::Sqrt(DV * DV + DA * DA + DD * DD);
}

FRfsnEmotionAxis FRfsnEmotionAxis::FromCoreEmotion(ERfsnCoreEmotion Emotion)
{
	// VAD coordinates for each core emotion (based on psychological research)
	FRfsnEmotionAxis Result;

	switch (Emotion)
	{
	case ERfsnCoreEmotion::Joy:
		Result.Valence = 0.9f;
		Result.Arousal = 0.5f;
		Result.Dominance = 0.6f;
		break;
	case ERfsnCoreEmotion::Trust:
		Result.Valence = 0.6f;
		Result.Arousal = -0.2f;
		Result.Dominance = 0.3f;
		break;
	case ERfsnCoreEmotion::Fear:
		Result.Valence = -0.8f;
		Result.Arousal = 0.7f;
		Result.Dominance = -0.8f;
		break;
	case ERfsnCoreEmotion::Surprise:
		Result.Valence = 0.1f;
		Result.Arousal = 0.8f;
		Result.Dominance = -0.2f;
		break;
	case ERfsnCoreEmotion::Sadness:
		Result.Valence = -0.7f;
		Result.Arousal = -0.5f;
		Result.Dominance = -0.5f;
		break;
	case ERfsnCoreEmotion::Disgust:
		Result.Valence = -0.6f;
		Result.Arousal = 0.2f;
		Result.Dominance = 0.4f;
		break;
	case ERfsnCoreEmotion::Anger:
		Result.Valence = -0.8f;
		Result.Arousal = 0.8f;
		Result.Dominance = 0.7f;
		break;
	case ERfsnCoreEmotion::Anticipation:
		Result.Valence = 0.3f;
		Result.Arousal = 0.6f;
		Result.Dominance = 0.3f;
		break;
	case ERfsnCoreEmotion::Neutral:
	default:
		Result.Valence = 0.0f;
		Result.Arousal = 0.0f;
		Result.Dominance = 0.0f;
		break;
	}

	return Result;
}

// ─────────────────────────────────────────────────────────────
// URfsnEmotionBlend Implementation
// ─────────────────────────────────────────────────────────────

URfsnEmotionBlend::URfsnEmotionBlend()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f; // ~60 FPS for smooth blending
}

void URfsnEmotionBlend::BeginPlay()
{
	Super::BeginPlay();

	// Initialize to baseline
	CurrentEmotion = PersonalityBaseline;
	TargetEmotion = PersonalityBaseline;
	DominantEmotion = CalculateDominantEmotion();
	UpdateFacialExpression();

	RFSN_LOG(TEXT("EmotionBlend initialized for %s"), *GetOwner()->GetName());
}

void URfsnEmotionBlend::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateEmotionBlend(DeltaTime);
}

void URfsnEmotionBlend::UpdateEmotionBlend(float DeltaTime)
{
	// Blend current toward target
	float BlendAlpha = FMath::Clamp(BlendSpeed * DeltaTime * (1.0f - EmotionalInertia), 0.0f, 1.0f);
	CurrentEmotion = FRfsnEmotionAxis::Lerp(CurrentEmotion, TargetEmotion, BlendAlpha);

	// Decay target back toward baseline
	float DecayAlpha = FMath::Clamp(DecayRate * DeltaTime, 0.0f, 1.0f);
	TargetEmotion = FRfsnEmotionAxis::Lerp(TargetEmotion, PersonalityBaseline, DecayAlpha);

	// Check for dominant emotion change
	ERfsnCoreEmotion NewDominant = CalculateDominantEmotion();
	if (NewDominant != DominantEmotion)
	{
		ERfsnCoreEmotion OldDominant = DominantEmotion;
		DominantEmotion = NewDominant;
		OnDominantEmotionChanged.Broadcast(NewDominant, OldDominant);
	}

	// Update facial expression weights
	UpdateFacialExpression();
}

void URfsnEmotionBlend::UpdateFacialExpression()
{
	// Convert VAD to facial expression weights
	// Each expression maps to a region in VAD space

	float V = CurrentEmotion.Valence;
	float A = CurrentEmotion.Arousal;
	float D = CurrentEmotion.Dominance;

	// Joy: high valence, moderate arousal, moderate dominance
	FacialExpression.Joy = FMath::Max(0.0f, V) * (0.5f + 0.5f * FMath::Clamp(A + 0.5f, 0.0f, 1.0f));

	// Sadness: low valence, low arousal, low dominance
	FacialExpression.Sadness = FMath::Max(0.0f, -V) * FMath::Max(0.0f, -A * 0.5f + 0.5f);

	// Anger: low valence, high arousal, high dominance
	FacialExpression.Anger = FMath::Max(0.0f, -V) * FMath::Max(0.0f, A) * FMath::Max(0.0f, D * 0.5f + 0.5f);

	// Fear: low valence, high arousal, low dominance
	FacialExpression.Fear = FMath::Max(0.0f, -V) * FMath::Max(0.0f, A) * FMath::Max(0.0f, -D * 0.5f + 0.5f);

	// Surprise: neutral valence, high arousal
	FacialExpression.Surprise = FMath::Max(0.0f, A - 0.3f) * (1.0f - FMath::Abs(V) * 0.5f);

	// Disgust: low valence, moderate arousal, high dominance
	FacialExpression.Disgust = FMath::Max(0.0f, -V * 0.7f) * FMath::Max(0.0f, D * 0.5f + 0.5f);

	// Trust: positive valence, low arousal, neutral dominance
	FacialExpression.Trust = FMath::Max(0.0f, V) * FMath::Max(0.0f, -A * 0.5f + 0.5f);

	// Apply intensity multiplier
	FacialExpression.OverallIntensity = ExpressionIntensity;

	// Normalize to prevent over-saturation
	float Total = FacialExpression.Joy + FacialExpression.Sadness + FacialExpression.Anger + FacialExpression.Fear +
	              FacialExpression.Surprise + FacialExpression.Disgust + FacialExpression.Trust;

	if (Total > 1.5f)
	{
		float Scale = 1.5f / Total;
		FacialExpression.Joy *= Scale;
		FacialExpression.Sadness *= Scale;
		FacialExpression.Anger *= Scale;
		FacialExpression.Fear *= Scale;
		FacialExpression.Surprise *= Scale;
		FacialExpression.Disgust *= Scale;
		FacialExpression.Trust *= Scale;
	}
}

void URfsnEmotionBlend::ApplyStimulus(const FString& EmotionName, float Intensity)
{
	ERfsnCoreEmotion Emotion = StringToEmotion(EmotionName);
	ApplyStimulusEnum(Emotion, Intensity);
	OnEmotionStimulus.Broadcast(EmotionName);
}

void URfsnEmotionBlend::ApplyStimulusEnum(ERfsnCoreEmotion Emotion, float Intensity)
{
	FRfsnEmotionAxis EmotionVAD = FRfsnEmotionAxis::FromCoreEmotion(Emotion);

	// Scale by intensity and blend into target
	float Inertia = EmotionalInertia;
	TargetEmotion.Valence = FMath::Lerp(TargetEmotion.Valence, EmotionVAD.Valence, Intensity * (1.0f - Inertia));
	TargetEmotion.Arousal = FMath::Lerp(TargetEmotion.Arousal, EmotionVAD.Arousal, Intensity * (1.0f - Inertia));
	TargetEmotion.Dominance = FMath::Lerp(TargetEmotion.Dominance, EmotionVAD.Dominance, Intensity * (1.0f - Inertia));

	// Clamp to valid range
	TargetEmotion.Valence = FMath::Clamp(TargetEmotion.Valence, -1.0f, 1.0f);
	TargetEmotion.Arousal = FMath::Clamp(TargetEmotion.Arousal, -1.0f, 1.0f);
	TargetEmotion.Dominance = FMath::Clamp(TargetEmotion.Dominance, -1.0f, 1.0f);

	RFSN_LOG(TEXT("Applied %s stimulus (%.2f) -> Target VAD: (%.2f, %.2f, %.2f)"), *EmotionToString(Emotion), Intensity,
	         TargetEmotion.Valence, TargetEmotion.Arousal, TargetEmotion.Dominance);
}

void URfsnEmotionBlend::ApplyStimulusVAD(float Valence, float Arousal, float Dominance)
{
	float Inertia = EmotionalInertia;
	TargetEmotion.Valence = FMath::Lerp(TargetEmotion.Valence, Valence, 1.0f - Inertia);
	TargetEmotion.Arousal = FMath::Lerp(TargetEmotion.Arousal, Arousal, 1.0f - Inertia);
	TargetEmotion.Dominance = FMath::Lerp(TargetEmotion.Dominance, Dominance, 1.0f - Inertia);

	TargetEmotion.Valence = FMath::Clamp(TargetEmotion.Valence, -1.0f, 1.0f);
	TargetEmotion.Arousal = FMath::Clamp(TargetEmotion.Arousal, -1.0f, 1.0f);
	TargetEmotion.Dominance = FMath::Clamp(TargetEmotion.Dominance, -1.0f, 1.0f);
}

float URfsnEmotionBlend::GetEmotionIntensity(ERfsnCoreEmotion Emotion) const
{
	FRfsnEmotionAxis EmotionVAD = FRfsnEmotionAxis::FromCoreEmotion(Emotion);

	// Calculate how close current state is to this emotion
	float Distance = CurrentEmotion.DistanceTo(EmotionVAD);

	// Convert distance to intensity (closer = higher intensity)
	// Max distance in VAD space is sqrt(12) ≈ 3.46
	float MaxDistance = 3.46f;
	return FMath::Max(0.0f, 1.0f - (Distance / MaxDistance));
}

float URfsnEmotionBlend::GetEmotionIntensityByName(const FString& EmotionName) const
{
	return GetEmotionIntensity(StringToEmotion(EmotionName));
}

FString URfsnEmotionBlend::ToMoodString() const
{
	// Combine dominant emotion with intensity qualifier
	FString Mood = EmotionToString(DominantEmotion);

	// Add intensity qualifier based on arousal
	if (FMath::Abs(CurrentEmotion.Arousal) > 0.6f)
	{
		if (CurrentEmotion.Arousal > 0)
		{
			Mood = TEXT("Intensely ") + Mood;
		}
		else
		{
			Mood = TEXT("Deeply ") + Mood;
		}
	}
	else if (FMath::Abs(CurrentEmotion.Arousal) < 0.2f && DominantEmotion != ERfsnCoreEmotion::Neutral)
	{
		Mood = TEXT("Mildly ") + Mood;
	}

	return Mood;
}

FString URfsnEmotionBlend::ToDialogueTone() const
{
	// Generate a tone hint for the LLM
	TArray<FString> ToneModifiers;

	// Valence modifiers
	if (CurrentEmotion.Valence > 0.5f)
	{
		ToneModifiers.Add(TEXT("warm"));
	}
	else if (CurrentEmotion.Valence < -0.5f)
	{
		ToneModifiers.Add(TEXT("harsh"));
	}

	// Arousal modifiers
	if (CurrentEmotion.Arousal > 0.5f)
	{
		ToneModifiers.Add(TEXT("energetic"));
	}
	else if (CurrentEmotion.Arousal < -0.5f)
	{
		ToneModifiers.Add(TEXT("subdued"));
	}

	// Dominance modifiers
	if (CurrentEmotion.Dominance > 0.5f)
	{
		ToneModifiers.Add(TEXT("assertive"));
	}
	else if (CurrentEmotion.Dominance < -0.5f)
	{
		ToneModifiers.Add(TEXT("uncertain"));
	}

	if (ToneModifiers.Num() == 0)
	{
		return TEXT("neutral");
	}

	return FString::Join(ToneModifiers, TEXT(", "));
}

TMap<FString, float> URfsnEmotionBlend::GetAllEmotionWeights() const
{
	TMap<FString, float> Weights;

	Weights.Add(TEXT("Joy"), GetEmotionIntensity(ERfsnCoreEmotion::Joy));
	Weights.Add(TEXT("Trust"), GetEmotionIntensity(ERfsnCoreEmotion::Trust));
	Weights.Add(TEXT("Fear"), GetEmotionIntensity(ERfsnCoreEmotion::Fear));
	Weights.Add(TEXT("Surprise"), GetEmotionIntensity(ERfsnCoreEmotion::Surprise));
	Weights.Add(TEXT("Sadness"), GetEmotionIntensity(ERfsnCoreEmotion::Sadness));
	Weights.Add(TEXT("Disgust"), GetEmotionIntensity(ERfsnCoreEmotion::Disgust));
	Weights.Add(TEXT("Anger"), GetEmotionIntensity(ERfsnCoreEmotion::Anger));
	Weights.Add(TEXT("Anticipation"), GetEmotionIntensity(ERfsnCoreEmotion::Anticipation));

	return Weights;
}

void URfsnEmotionBlend::SetEmotionImmediate(ERfsnCoreEmotion Emotion, float Intensity)
{
	FRfsnEmotionAxis EmotionVAD = FRfsnEmotionAxis::FromCoreEmotion(Emotion);

	CurrentEmotion = FRfsnEmotionAxis::Lerp(PersonalityBaseline, EmotionVAD, Intensity);
	TargetEmotion = CurrentEmotion;

	ERfsnCoreEmotion NewDominant = CalculateDominantEmotion();
	if (NewDominant != DominantEmotion)
	{
		ERfsnCoreEmotion OldDominant = DominantEmotion;
		DominantEmotion = NewDominant;
		OnDominantEmotionChanged.Broadcast(NewDominant, OldDominant);
	}

	UpdateFacialExpression();
}

void URfsnEmotionBlend::ResetToBaseline()
{
	TargetEmotion = PersonalityBaseline;
}

TMap<FName, float> URfsnEmotionBlend::GetMorphTargetWeights() const
{
	TMap<FName, float> Weights;

	// Standard morph target names (adjust to match your mesh)
	Weights.Add(FName("Smile"), FacialExpression.Joy * FacialExpression.OverallIntensity);
	Weights.Add(FName("Frown"), FacialExpression.Sadness * FacialExpression.OverallIntensity);
	Weights.Add(FName("Anger"), FacialExpression.Anger * FacialExpression.OverallIntensity);
	Weights.Add(FName("Fear"), FacialExpression.Fear * FacialExpression.OverallIntensity);
	Weights.Add(FName("Surprise"), FacialExpression.Surprise * FacialExpression.OverallIntensity);
	Weights.Add(FName("Disgust"), FacialExpression.Disgust * FacialExpression.OverallIntensity);
	Weights.Add(FName("Trust"), FacialExpression.Trust * FacialExpression.OverallIntensity);

	// Eye expressions
	Weights.Add(FName("EyesWide"),
	            FMath::Max(FacialExpression.Fear, FacialExpression.Surprise) * FacialExpression.OverallIntensity);
	Weights.Add(FName("EyesNarrow"),
	            FMath::Max(FacialExpression.Anger, FacialExpression.Disgust) * FacialExpression.OverallIntensity);

	// Brow expressions
	Weights.Add(FName("BrowsUp"), FacialExpression.Surprise * FacialExpression.OverallIntensity);
	Weights.Add(FName("BrowsFurrow"),
	            FMath::Max(FacialExpression.Anger, FacialExpression.Sadness) * FacialExpression.OverallIntensity);

	return Weights;
}

void URfsnEmotionBlend::ApplyToSkeletalMesh(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	TMap<FName, float> Weights = GetMorphTargetWeights();

	for (const auto& Pair : Weights)
	{
		Mesh->SetMorphTarget(Pair.Key, Pair.Value);
	}
}

ERfsnCoreEmotion URfsnEmotionBlend::CalculateDominantEmotion() const
{
	// Find which emotion's VAD coordinates we're closest to
	float MinDistance = MAX_FLT;
	ERfsnCoreEmotion Closest = ERfsnCoreEmotion::Neutral;

	// Check all emotions
	TArray<ERfsnCoreEmotion> AllEmotions = {
	    ERfsnCoreEmotion::Joy,      ERfsnCoreEmotion::Trust,        ERfsnCoreEmotion::Fear,
	    ERfsnCoreEmotion::Surprise, ERfsnCoreEmotion::Sadness,      ERfsnCoreEmotion::Disgust,
	    ERfsnCoreEmotion::Anger,    ERfsnCoreEmotion::Anticipation, ERfsnCoreEmotion::Neutral};

	for (ERfsnCoreEmotion Emotion : AllEmotions)
	{
		float Distance = CurrentEmotion.DistanceTo(FRfsnEmotionAxis::FromCoreEmotion(Emotion));
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			Closest = Emotion;
		}
	}

	return Closest;
}

ERfsnCoreEmotion URfsnEmotionBlend::StringToEmotion(const FString& Name)
{
	FString Lower = Name.ToLower();

	if (Lower == TEXT("joy") || Lower == TEXT("happy") || Lower == TEXT("happiness"))
		return ERfsnCoreEmotion::Joy;
	if (Lower == TEXT("trust") || Lower == TEXT("calm"))
		return ERfsnCoreEmotion::Trust;
	if (Lower == TEXT("fear") || Lower == TEXT("scared") || Lower == TEXT("afraid"))
		return ERfsnCoreEmotion::Fear;
	if (Lower == TEXT("surprise") || Lower == TEXT("surprised") || Lower == TEXT("shock"))
		return ERfsnCoreEmotion::Surprise;
	if (Lower == TEXT("sadness") || Lower == TEXT("sad") || Lower == TEXT("sorrow"))
		return ERfsnCoreEmotion::Sadness;
	if (Lower == TEXT("disgust") || Lower == TEXT("disgusted"))
		return ERfsnCoreEmotion::Disgust;
	if (Lower == TEXT("anger") || Lower == TEXT("angry") || Lower == TEXT("rage"))
		return ERfsnCoreEmotion::Anger;
	if (Lower == TEXT("anticipation") || Lower == TEXT("excited") || Lower == TEXT("eager"))
		return ERfsnCoreEmotion::Anticipation;

	return ERfsnCoreEmotion::Neutral;
}

FString URfsnEmotionBlend::EmotionToString(ERfsnCoreEmotion Emotion)
{
	switch (Emotion)
	{
	case ERfsnCoreEmotion::Joy:
		return TEXT("Joy");
	case ERfsnCoreEmotion::Trust:
		return TEXT("Trust");
	case ERfsnCoreEmotion::Fear:
		return TEXT("Fear");
	case ERfsnCoreEmotion::Surprise:
		return TEXT("Surprise");
	case ERfsnCoreEmotion::Sadness:
		return TEXT("Sadness");
	case ERfsnCoreEmotion::Disgust:
		return TEXT("Disgust");
	case ERfsnCoreEmotion::Anger:
		return TEXT("Anger");
	case ERfsnCoreEmotion::Anticipation:
		return TEXT("Anticipation");
	case ERfsnCoreEmotion::Neutral:
	default:
		return TEXT("Neutral");
	}
}

// ─────────────────────────────────────────────────────────────
// Emotional Contagion Implementation
// ─────────────────────────────────────────────────────────────

void URfsnEmotionBlend::ApplyContagionFromNearby()
{
	if (!bEnableContagion || ContagionSusceptibility <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector MyLocation = GetOwner()->GetActorLocation();
	FRfsnEmotionAxis AggregatedEmotion;
	float TotalInfluence = 0.0f;
	int32 NearbyCount = 0;

	// Find all actors with EmotionBlend in range
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* OtherActor = *It;
		if (OtherActor == GetOwner())
		{
			continue;
		}

		URfsnEmotionBlend* OtherEmotion = OtherActor->FindComponentByClass<URfsnEmotionBlend>();
		if (!OtherEmotion)
		{
			continue;
		}

		float Distance = FVector::Dist(MyLocation, OtherActor->GetActorLocation());
		if (Distance > ContagionRadius)
		{
			continue;
		}

		// Calculate influence based on distance and other NPC's influence strength
		float DistanceFactor = 1.0f - (Distance / ContagionRadius);
		float Influence = DistanceFactor * OtherEmotion->ContagionInfluence;

		// Accumulate weighted emotion
		AggregatedEmotion.Valence += OtherEmotion->CurrentEmotion.Valence * Influence;
		AggregatedEmotion.Arousal += OtherEmotion->CurrentEmotion.Arousal * Influence;
		AggregatedEmotion.Dominance += OtherEmotion->CurrentEmotion.Dominance * Influence;
		TotalInfluence += Influence;
		NearbyCount++;
	}

	// Apply averaged emotional influence
	if (NearbyCount > 0 && TotalInfluence > 0.0f)
	{
		AggregatedEmotion.Valence /= TotalInfluence;
		AggregatedEmotion.Arousal /= TotalInfluence;
		AggregatedEmotion.Dominance /= TotalInfluence;

		// Blend toward aggregated emotion based on susceptibility
		float BlendAmount = ContagionSusceptibility * FMath::Min(TotalInfluence, 1.0f) * 0.1f;
		ApplyStimulusVAD(FMath::Lerp(CurrentEmotion.Valence, AggregatedEmotion.Valence, BlendAmount),
		                 FMath::Lerp(CurrentEmotion.Arousal, AggregatedEmotion.Arousal, BlendAmount),
		                 FMath::Lerp(CurrentEmotion.Dominance, AggregatedEmotion.Dominance, BlendAmount));

		RFSN_LOG(TEXT("Contagion: %s influenced by %d NPCs (blend: %.2f)"), *GetOwner()->GetName(), NearbyCount,
		         BlendAmount);
	}
}

// ─────────────────────────────────────────────────────────────
// Voice Modulation Implementation
// ─────────────────────────────────────────────────────────────

float URfsnEmotionBlend::GetVoicePitchModifier() const
{
	// Arousal raises pitch, low valence also raises pitch slightly
	float BasePitch = 1.0f;

	// High arousal = higher pitch
	BasePitch += CurrentEmotion.Arousal * 0.15f;

	// Negative emotions slightly raise pitch (tension)
	if (CurrentEmotion.Valence < 0)
	{
		BasePitch += FMath::Abs(CurrentEmotion.Valence) * 0.05f;
	}

	// Sadness lowers pitch
	if (DominantEmotion == ERfsnCoreEmotion::Sadness)
	{
		BasePitch -= 0.1f;
	}

	return FMath::Clamp(BasePitch, 0.8f, 1.2f);
}

float URfsnEmotionBlend::GetVoiceSpeedModifier() const
{
	// Arousal increases speed, sadness/trust slow it down
	float BaseSpeed = 1.0f;

	// High arousal = faster speech
	BaseSpeed += CurrentEmotion.Arousal * 0.15f;

	// Fear makes speech fast and rushed
	if (DominantEmotion == ERfsnCoreEmotion::Fear)
	{
		BaseSpeed += 0.1f;
	}

	// Sadness slows speech
	if (DominantEmotion == ERfsnCoreEmotion::Sadness)
	{
		BaseSpeed -= 0.15f;
	}

	// Trust is calm and measured
	if (DominantEmotion == ERfsnCoreEmotion::Trust)
	{
		BaseSpeed -= 0.1f;
	}

	return FMath::Clamp(BaseSpeed, 0.8f, 1.2f);
}

float URfsnEmotionBlend::GetVoiceVolumeModifier() const
{
	// Dominance and arousal increase volume
	float BaseVolume = 1.0f;

	// High dominance = louder
	BaseVolume += CurrentEmotion.Dominance * 0.15f;

	// High arousal = louder
	BaseVolume += CurrentEmotion.Arousal * 0.1f;

	// Anger is loud
	if (DominantEmotion == ERfsnCoreEmotion::Anger)
	{
		BaseVolume += 0.2f;
	}

	// Fear and sadness are quieter
	if (DominantEmotion == ERfsnCoreEmotion::Fear || DominantEmotion == ERfsnCoreEmotion::Sadness)
	{
		BaseVolume -= 0.15f;
	}

	return FMath::Clamp(BaseVolume, 0.7f, 1.3f);
}

// ─────────────────────────────────────────────────────────────
// Persistence Implementation
// ─────────────────────────────────────────────────────────────

#include "RfsnNpcClientComponent.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString URfsnEmotionBlend::GetNpcId() const
{
	if (URfsnNpcClientComponent* Client = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		return Client->NpcId;
	}
	return GetOwner()->GetName();
}

void URfsnEmotionBlend::SaveEmotionState()
{
	FString NpcId = GetNpcId();
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Emotions") / FString::Printf(TEXT("Emotion_%s.json"), *NpcId);

	TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetNumberField(TEXT("valence"), CurrentEmotion.Valence);
	JsonObj->SetNumberField(TEXT("arousal"), CurrentEmotion.Arousal);
	JsonObj->SetNumberField(TEXT("dominance"), CurrentEmotion.Dominance);
	JsonObj->SetNumberField(TEXT("target_valence"), TargetEmotion.Valence);
	JsonObj->SetNumberField(TEXT("target_arousal"), TargetEmotion.Arousal);
	JsonObj->SetNumberField(TEXT("target_dominance"), TargetEmotion.Dominance);
	JsonObj->SetStringField(TEXT("dominant_emotion"), EmotionToString(DominantEmotion));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObj, Writer);

	// Ensure directory exists
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);
	FFileHelper::SaveStringToFile(OutputString, *SavePath);

	RFSN_LOG(TEXT("Saved emotion state for %s"), *NpcId);
}

bool URfsnEmotionBlend::LoadEmotionState()
{
	FString NpcId = GetNpcId();
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Emotions") / FString::Printf(TEXT("Emotion_%s.json"), *NpcId);

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *SavePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		return false;
	}

	CurrentEmotion.Valence = JsonObj->GetNumberField(TEXT("valence"));
	CurrentEmotion.Arousal = JsonObj->GetNumberField(TEXT("arousal"));
	CurrentEmotion.Dominance = JsonObj->GetNumberField(TEXT("dominance"));
	TargetEmotion.Valence = JsonObj->GetNumberField(TEXT("target_valence"));
	TargetEmotion.Arousal = JsonObj->GetNumberField(TEXT("target_arousal"));
	TargetEmotion.Dominance = JsonObj->GetNumberField(TEXT("target_dominance"));

	FString EmotionStr = JsonObj->GetStringField(TEXT("dominant_emotion"));
	DominantEmotion = StringToEmotion(EmotionStr);

	UpdateFacialExpression();

	RFSN_LOG(TEXT("Loaded emotion state for %s: %s"), *NpcId, *EmotionStr);
	return true;
}
