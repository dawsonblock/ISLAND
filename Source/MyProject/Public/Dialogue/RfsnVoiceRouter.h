// RFSN Voice Router System
// Intelligent TTS model selection based on narrative weight
// Routes to Chatterbox (full) or Chatterbox-Turbo automatically

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnVoiceRouter.generated.h"

class URfsnEmotionBlend;
class URfsnNpcClientComponent;
class URfsnTtsAudioComponent;

/**
 * Voice intensity level - drives TTS model selection
 */
UENUM(BlueprintType)
enum class ERfsnVoiceIntensity : uint8
{
	Low UMETA(DisplayName = "Low"),       // Turbo - barks, ambience
	Medium UMETA(DisplayName = "Medium"), // Turbo + style boost
	High UMETA(DisplayName = "High")      // Full Chatterbox
};

/**
 * TTS backend type
 */
UENUM(BlueprintType)
enum class ERfsnTtsBackend : uint8
{
	ChatterboxFull UMETA(DisplayName = "Chatterbox Full"),
	ChatterboxTurbo UMETA(DisplayName = "Chatterbox Turbo"),
	Qwen UMETA(DisplayName = "Qwen3-TTS"),
	Kokoro UMETA(DisplayName = "Kokoro")
};

/**
 * Voice style parameters for TTS
 */
USTRUCT(BlueprintType)
struct FRfsnVoiceStyle
{
	GENERATED_BODY()

	/** Emotion tag (anger, joy, sadness, etc) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	FString Emotion = TEXT("neutral");

	/** Intensity 0.0-1.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "1"))
	float Intensity = 0.5f;

	/** Speaking pace modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float PaceModifier = 1.0f;

	/** Pitch modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float PitchModifier = 1.0f;

	/** Voice reference audio path (for cloning) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	FString VoiceReferencePath;
};

/**
 * TTS request for routing
 */
USTRUCT(BlueprintType)
struct FRfsnTtsRequest
{
	GENERATED_BODY()

	/** Text to synthesize */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	FString Text;

	/** NPC ID */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	FString NpcId;

	/** Narrative weight (0-2) */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	ERfsnVoiceIntensity Intensity = ERfsnVoiceIntensity::Low;

	/** Voice style parameters */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	FRfsnVoiceStyle Style;

	/** Force specific backend (optional) */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	ERfsnTtsBackend ForcedBackend = ERfsnTtsBackend::ChatterboxTurbo;

	/** Use forced backend? */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	bool bUseForced = false;

	/** Is this a bark/one-liner? */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	bool bIsBark = false;

	/** Is this story-critical dialogue? */
	UPROPERTY(BlueprintReadWrite, Category = "Request")
	bool bIsStoryCritical = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTtsRouted, ERfsnTtsBackend, Backend, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTtsComplete, const FString&, AudioPath);

/**
 * Voice Router Component
 * Automatically selects optimal TTS backend based on context
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnVoiceRouter : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnVoiceRouter();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Chatterbox Full endpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Endpoints")
	FString ChatterboxFullEndpoint = TEXT("http://localhost:8001/synthesize");

	/** Chatterbox Turbo endpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Endpoints")
	FString ChatterboxTurboEndpoint = TEXT("http://localhost:8002/synthesize");

	/** Qwen TTS endpoint (fallback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Endpoints")
	FString QwenEndpoint = TEXT("http://localhost:8003/synthesize");

	/** Default voice reference for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Voice")
	FString DefaultVoiceReference;

	/** Default emotion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Voice")
	FString DefaultEmotion = TEXT("neutral");

	/** Emotion intensity threshold for Full Chatterbox */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Thresholds")
	float HighIntensityThreshold = 0.7f;

	/** Arousal threshold for Full Chatterbox */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Thresholds")
	float HighArousalThreshold = 0.6f;

	/** Always use Full for story-critical lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Behavior")
	bool bAlwaysFullForStoryCritical = true;

	/** Always use Turbo for barks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Behavior")
	bool bAlwaysTurboForBarks = true;

	/** Enable auto-routing from emotion blend */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Router|Behavior")
	bool bAutoRouteFromEmotion = true;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Last used backend */
	UPROPERTY(BlueprintReadOnly, Category = "Router|State")
	ERfsnTtsBackend LastUsedBackend = ERfsnTtsBackend::ChatterboxTurbo;

	/** Requests sent to Full Chatterbox */
	UPROPERTY(BlueprintReadOnly, Category = "Router|Stats")
	int32 FullRequestCount = 0;

	/** Requests sent to Turbo */
	UPROPERTY(BlueprintReadOnly, Category = "Router|Stats")
	int32 TurboRequestCount = 0;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Router|Events")
	FOnTtsRouted OnTtsRouted;

	UPROPERTY(BlueprintAssignable, Category = "Router|Events")
	FOnTtsComplete OnTtsComplete;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Route and synthesize speech */
	UFUNCTION(BlueprintCallable, Category = "Router")
	void Synthesize(const FRfsnTtsRequest& Request);

	/** Quick synthesize with auto-routing */
	UFUNCTION(BlueprintCallable, Category = "Router")
	void SynthesizeAuto(const FString& Text, ERfsnVoiceIntensity Intensity = ERfsnVoiceIntensity::Low);

	/** Synthesize a bark (always Turbo) */
	UFUNCTION(BlueprintCallable, Category = "Router")
	void SynthesizeBark(const FString& Text);

	/** Synthesize story dialogue (always Full) */
	UFUNCTION(BlueprintCallable, Category = "Router")
	void SynthesizeStoryCritical(const FString& Text);

	/** Determine optimal backend for request */
	UFUNCTION(BlueprintPure, Category = "Router")
	ERfsnTtsBackend DetermineBackend(const FRfsnTtsRequest& Request) const;

	/** Get intensity from emotion blend */
	UFUNCTION(BlueprintPure, Category = "Router")
	ERfsnVoiceIntensity GetIntensityFromEmotion() const;

	/** Build voice style from emotion blend */
	UFUNCTION(BlueprintPure, Category = "Router")
	FRfsnVoiceStyle BuildStyleFromEmotion() const;

	/** Get backend name as string */
	UFUNCTION(BlueprintPure, Category = "Router")
	static FString BackendToString(ERfsnTtsBackend Backend);

	/** Get usage stats */
	UFUNCTION(BlueprintPure, Category = "Router")
	FString GetUsageStats() const;

protected:
	virtual void BeginPlay() override;

private:
	/** Cached emotion blend reference */
	UPROPERTY()
	URfsnEmotionBlend* EmotionBlend = nullptr;

	/** Make HTTP request to TTS backend */
	void SendToBackend(ERfsnTtsBackend Backend, const FString& Text, const FRfsnVoiceStyle& Style);

	/** Get endpoint URL for backend */
	FString GetBackendEndpoint(ERfsnTtsBackend Backend) const;
};
