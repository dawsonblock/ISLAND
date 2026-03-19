// RFSN Emotion Blending System
// Continuous emotional state using VAD (Valence-Arousal-Dominance) model
// Drives both facial animations and dialogue tone

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnEmotionBlend.generated.h"

/**
 * Core emotion types based on Plutchik's wheel
 */
UENUM(BlueprintType)
enum class ERfsnCoreEmotion : uint8
{
	Joy UMETA(DisplayName = "Joy"),
	Trust UMETA(DisplayName = "Trust"),
	Fear UMETA(DisplayName = "Fear"),
	Surprise UMETA(DisplayName = "Surprise"),
	Sadness UMETA(DisplayName = "Sadness"),
	Disgust UMETA(DisplayName = "Disgust"),
	Anger UMETA(DisplayName = "Anger"),
	Anticipation UMETA(DisplayName = "Anticipation"),
	Neutral UMETA(DisplayName = "Neutral")
};

/**
 * VAD (Valence-Arousal-Dominance) emotion axis
 * Standard psychological model for continuous emotion representation
 */
USTRUCT(BlueprintType)
struct FRfsnEmotionAxis
{
	GENERATED_BODY()

	/** Valence: Negative (-1) ↔ Positive (+1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Valence = 0.0f;

	/** Arousal: Calm (-1) ↔ Excited (+1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Arousal = 0.0f;

	/** Dominance: Submissive (-1) ↔ Dominant (+1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Dominance = 0.0f;

	/** Linear interpolation between two axes */
	static FRfsnEmotionAxis Lerp(const FRfsnEmotionAxis& A, const FRfsnEmotionAxis& B, float Alpha);

	/** Get distance between two emotion states */
	float DistanceTo(const FRfsnEmotionAxis& Other) const;

	/** Convert core emotion to VAD coordinates */
	static FRfsnEmotionAxis FromCoreEmotion(ERfsnCoreEmotion Emotion);
};

/**
 * Animation blend target for a specific emotion
 */
USTRUCT(BlueprintType)
struct FRfsnEmotionAnimationTarget
{
	GENERATED_BODY()

	/** Name of the morph target or animation parameter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FName TargetName;

	/** Current blend weight (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Weight = 0.0f;
};

/**
 * Facial expression blend weights
 */
USTRUCT(BlueprintType)
struct FRfsnFacialExpression
{
	GENERATED_BODY()

	/** Joy/Smile intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Joy = 0.0f;

	/** Sadness/Frown intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Sadness = 0.0f;

	/** Anger/Furrow intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Anger = 0.0f;

	/** Fear/Wide eyes intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Fear = 0.0f;

	/** Surprise/Raised brows intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Surprise = 0.0f;

	/** Disgust/Nose scrunch intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Disgust = 0.0f;

	/** Trust/Soft eyes intensity (0-1) */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float Trust = 0.0f;

	/** Overall expression intensity multiplier */
	UPROPERTY(BlueprintReadWrite, Category = "Expression")
	float OverallIntensity = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEmotionChanged, ERfsnCoreEmotion, NewDominant, ERfsnCoreEmotion,
                                             PreviousDominant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmotionStimulus, const FString&, EmotionName);

/**
 * Emotion blending component
 * Manages continuous emotional state with smooth transitions
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnEmotionBlend : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnEmotionBlend();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Personality baseline - NPC returns to this over time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Personality")
	FRfsnEmotionAxis PersonalityBaseline;

	/** How fast emotions transition (higher = faster) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Config",
	          meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float BlendSpeed = 2.0f;

	/** Resistance to emotional change (higher = more stable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Config",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EmotionalInertia = 0.3f;

	/** How fast emotions decay back to baseline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Config",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DecayRate = 0.1f;

	/** Expression intensity multiplier for animations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Animation",
	          meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ExpressionIntensity = 1.0f;

	// ─────────────────────────────────────────────────────────────
	// State (Read Only)
	// ─────────────────────────────────────────────────────────────

	/** Current position in emotion space */
	UPROPERTY(BlueprintReadOnly, Category = "Emotion|State")
	FRfsnEmotionAxis CurrentEmotion;

	/** Target emotion to blend toward */
	UPROPERTY(BlueprintReadOnly, Category = "Emotion|State")
	FRfsnEmotionAxis TargetEmotion;

	/** Current dominant emotion */
	UPROPERTY(BlueprintReadOnly, Category = "Emotion|State")
	ERfsnCoreEmotion DominantEmotion = ERfsnCoreEmotion::Neutral;

	/** Current facial expression weights */
	UPROPERTY(BlueprintReadOnly, Category = "Emotion|Animation")
	FRfsnFacialExpression FacialExpression;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when dominant emotion changes */
	UPROPERTY(BlueprintAssignable, Category = "Emotion|Events")
	FOnEmotionChanged OnDominantEmotionChanged;

	/** Called when an emotional stimulus is applied */
	UPROPERTY(BlueprintAssignable, Category = "Emotion|Events")
	FOnEmotionStimulus OnEmotionStimulus;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Apply emotional stimulus by name (Joy, Anger, Fear, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	void ApplyStimulus(const FString& EmotionName, float Intensity = 1.0f);

	/** Apply emotional stimulus by enum */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	void ApplyStimulusEnum(ERfsnCoreEmotion Emotion, float Intensity = 1.0f);

	/** Apply stimulus directly in VAD space */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	void ApplyStimulusVAD(float Valence, float Arousal, float Dominance);

	/** Get current intensity of a specific emotion (0-1) */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	float GetEmotionIntensity(ERfsnCoreEmotion Emotion) const;

	/** Get intensity by name */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	float GetEmotionIntensityByName(const FString& EmotionName) const;

	/** Convert current state to mood string for LLM prompts */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	FString ToMoodString() const;

	/** Get dialogue tone modifier for LLM */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	FString ToDialogueTone() const;

	/** Get all emotion weights as a map */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	TMap<FString, float> GetAllEmotionWeights() const;

	/** Force immediate emotion state (no blending) */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	void SetEmotionImmediate(ERfsnCoreEmotion Emotion, float Intensity = 1.0f);

	/** Reset to personality baseline */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	void ResetToBaseline();

	// ─────────────────────────────────────────────────────────────
	// Animation Helpers
	// ─────────────────────────────────────────────────────────────

	/** Get facial expression blend weights for animation */
	UFUNCTION(BlueprintPure, Category = "Emotion|Animation")
	FRfsnFacialExpression GetFacialExpression() const { return FacialExpression; }

	/** Get morph target weights (for direct mesh blending) */
	UFUNCTION(BlueprintPure, Category = "Emotion|Animation")
	TMap<FName, float> GetMorphTargetWeights() const;

	/** Apply expression weights to a skeletal mesh */
	UFUNCTION(BlueprintCallable, Category = "Emotion|Animation")
	void ApplyToSkeletalMesh(USkeletalMeshComponent* Mesh);

	// ─────────────────────────────────────────────────────────────
	// Emotional Contagion
	// ─────────────────────────────────────────────────────────────

	/** Enable emotional influence from nearby NPCs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Contagion")
	bool bEnableContagion = true;

	/** Max distance to be affected by other NPCs' emotions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Contagion",
	          meta = (ClampMin = "0", ClampMax = "2000"))
	float ContagionRadius = 500.0f;

	/** How strongly this NPC is affected by others (0 = immune, 1 = highly susceptible) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Contagion", meta = (ClampMin = "0", ClampMax = "1"))
	float ContagionSusceptibility = 0.3f;

	/** How strongly this NPC's emotions affect others */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion|Contagion", meta = (ClampMin = "0", ClampMax = "1"))
	float ContagionInfluence = 0.5f;

	/** Manually trigger contagion check */
	UFUNCTION(BlueprintCallable, Category = "Emotion|Contagion")
	void ApplyContagionFromNearby();

	// ─────────────────────────────────────────────────────────────
	// Voice Modulation (for TTS)
	// ─────────────────────────────────────────────────────────────

	/** Get pitch modifier based on emotion (0.8 - 1.2 range) */
	UFUNCTION(BlueprintPure, Category = "Emotion|Voice")
	float GetVoicePitchModifier() const;

	/** Get speed modifier based on emotion (0.8 - 1.2 range) */
	UFUNCTION(BlueprintPure, Category = "Emotion|Voice")
	float GetVoiceSpeedModifier() const;

	/** Get volume modifier based on emotion (0.7 - 1.3 range) */
	UFUNCTION(BlueprintPure, Category = "Emotion|Voice")
	float GetVoiceVolumeModifier() const;

	// ─────────────────────────────────────────────────────────────
	// Persistence
	// ─────────────────────────────────────────────────────────────

	/** Save current emotion state */
	UFUNCTION(BlueprintCallable, Category = "Emotion|Persistence")
	void SaveEmotionState();

	/** Load saved emotion state */
	UFUNCTION(BlueprintCallable, Category = "Emotion|Persistence")
	bool LoadEmotionState();

	/** Get NPC ID for saving (from sibling RfsnNpcClientComponent) */
	UFUNCTION(BlueprintPure, Category = "Emotion|Persistence")
	FString GetNpcId() const;

	/** Convert enum to string */
	UFUNCTION(BlueprintPure, Category = "Emotion")
	static FString EmotionToString(ERfsnCoreEmotion Emotion);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Update emotion blending each frame */
	void UpdateEmotionBlend(float DeltaTime);

	/** Update facial expression from current emotion */
	void UpdateFacialExpression();

	/** Calculate dominant emotion from VAD coordinates */
	ERfsnCoreEmotion CalculateDominantEmotion() const;

	/** Convert emotion name to enum */
	static ERfsnCoreEmotion StringToEmotion(const FString& Name);
};
