// RFSN Lip Sync Component
// Drives facial animation from TTS audio analysis
// Provides viseme-based and amplitude-based lip sync

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnLipSync.generated.h"

class UAudioComponent;
class USkeletalMeshComponent;
class URfsnTtsAudioComponent;

/**
 * Viseme types for lip sync (based on common phoneme groups)
 */
UENUM(BlueprintType)
enum class ERfsnViseme : uint8
{
	Silence UMETA(DisplayName = "Silence"), // Mouth closed
	AA UMETA(DisplayName = "AA"),           // "ah" as in "father"
	AO UMETA(DisplayName = "AO"),           // "aw" as in "jaw"
	EE UMETA(DisplayName = "EE"),           // "ee" as in "see"
	EH UMETA(DisplayName = "EH"),           // "eh" as in "bed"
	IH UMETA(DisplayName = "IH"),           // "ih" as in "sit"
	OH UMETA(DisplayName = "OH"),           // "oh" as in "go"
	OO UMETA(DisplayName = "OO"),           // "oo" as in "too"
	UH UMETA(DisplayName = "UH"),           // "uh" as in "but"
	CDG UMETA(DisplayName = "CDG"),         // Hard consonants c,d,g,k,n,r,s,y,z
	FV UMETA(DisplayName = "FV"),           // "f" and "v" - lip against teeth
	L UMETA(DisplayName = "L"),             // "l" - tongue behind teeth
	MBP UMETA(DisplayName = "MBP"),         // "m", "b", "p" - lips together
	TH UMETA(DisplayName = "TH"),           // "th" - tongue between teeth
	WQ UMETA(DisplayName = "WQ")            // "w" and "q" - pursed lips
};

/**
 * Morph target mapping for a viseme
 */
USTRUCT(BlueprintType)
struct FRfsnVisemeMapping
{
	GENERATED_BODY()

	/** Viseme this maps to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync")
	ERfsnViseme Viseme = ERfsnViseme::Silence;

	/** Morph target name on the mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync")
	FName MorphTargetName;

	/** Weight multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync", meta = (ClampMin = "0", ClampMax = "2"))
	float WeightMultiplier = 1.0f;
};

/**
 * Current lip sync state
 */
USTRUCT(BlueprintType)
struct FRfsnLipSyncState
{
	GENERATED_BODY()

	/** Current audio amplitude (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	float Amplitude = 0.0f;

	/** Smoothed amplitude */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	float SmoothedAmplitude = 0.0f;

	/** Current viseme */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	ERfsnViseme CurrentViseme = ERfsnViseme::Silence;

	/** Is audio currently playing? */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	bool bIsPlaying = false;

	/** Jaw open amount (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	float JawOpen = 0.0f;

	/** Lips together amount (0-1) for M/B/P sounds */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync")
	float LipsTogether = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisemeChanged, ERfsnViseme, NewViseme);

/**
 * Lip Sync Component
 * Analyzes audio and drives facial morph targets for lip movement
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnLipSync : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnLipSync();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Target skeletal mesh (auto-finds if not set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config")
	USkeletalMeshComponent* TargetMesh;

	/** Viseme to morph target mappings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config")
	TArray<FRfsnVisemeMapping> VisemeMappings;

	/** Jaw open morph target name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config")
	FName JawOpenMorphTarget = FName("JawOpen");

	/** Smoothing factor (higher = smoother but laggier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config", meta = (ClampMin = "0", ClampMax = "1"))
	float SmoothingFactor = 0.3f;

	/** Amplitude multiplier for jaw movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config", meta = (ClampMin = "0", ClampMax = "3"))
	float JawAmplitudeScale = 1.5f;

	/** Minimum amplitude to trigger lip movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config", meta = (ClampMin = "0", ClampMax = "1"))
	float AmplitudeThreshold = 0.05f;

	/** Use simple amplitude-based jaw movement (vs viseme analysis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config")
	bool bUseSimpleMode = true;

	/** Viseme change speed (higher = faster transitions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Config", meta = (ClampMin = "1", ClampMax = "30"))
	float VisemeChangeSpeed = 12.0f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current lip sync state */
	UPROPERTY(BlueprintReadOnly, Category = "LipSync|State")
	FRfsnLipSyncState CurrentState;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when viseme changes */
	UPROPERTY(BlueprintAssignable, Category = "LipSync|Events")
	FOnVisemeChanged OnVisemeChanged;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start lip sync from audio component */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void StartLipSync(UAudioComponent* AudioSource);

	/** Stop lip sync */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void StopLipSync();

	/** Manually set amplitude (for external audio analysis) */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void SetAmplitude(float NewAmplitude);

	/** Manually set viseme (for external phoneme analysis) */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void SetViseme(ERfsnViseme Viseme);

	/** Get current amplitude */
	UFUNCTION(BlueprintPure, Category = "LipSync")
	float GetCurrentAmplitude() const { return CurrentState.SmoothedAmplitude; }

	/** Check if lip sync is active */
	UFUNCTION(BlueprintPure, Category = "LipSync")
	bool IsPlaying() const { return CurrentState.bIsPlaying; }

	/** Apply current state to mesh */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void ApplyToMesh();

	/** Setup default viseme mappings */
	UFUNCTION(BlueprintCallable, Category = "LipSync")
	void SetupDefaultMappings();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Current audio source */
	UPROPERTY()
	UAudioComponent* AudioSource;

	/** TTS component (if using RFSN TTS) */
	UPROPERTY()
	URfsnTtsAudioComponent* TtsComponent;

	/** Previous viseme weights for blending */
	TMap<ERfsnViseme, float> VisemeWeights;

	/** Update amplitude from audio */
	void UpdateAmplitude();

	/** Generate pseudo-viseme from amplitude pattern */
	ERfsnViseme GeneratePseudoViseme() const;

	/** Apply viseme to mesh */
	void ApplyViseme(ERfsnViseme Viseme, float Weight);

	/** Reset all viseme weights */
	void ResetVisemeWeights();
};
