// RFSN TTS Audio Component
// Handles Text-to-Speech audio playback for NPC dialogue

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnTtsAudioComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTtsAudioStarted, const FString&, Sentence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTtsAudioFinished);

/**
 * Component for playing RFSN TTS audio.
 * Can receive audio data from RFSN orchestrator and play it through Unreal's
 * audio system.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnTtsAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnTtsAudioComponent();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Volume multiplier for TTS playback */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TTS|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.0f;

	/** Pitch multiplier for TTS playback */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TTS|Audio", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float PitchMultiplier = 1.0f;

	/** Attenuation settings for 3D audio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TTS|Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	/** Enable audio queue (play sentences in order) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TTS|Audio")
	bool bEnableQueue = true;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "TTS|Events")
	FOnTtsAudioStarted OnAudioStarted;

	UPROPERTY(BlueprintAssignable, Category = "TTS|Events")
	FOnTtsAudioFinished OnAudioFinished;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Bind to RFSN client for automatic audio playback */
	UFUNCTION(BlueprintCallable, Category = "TTS")
	void BindToRfsnClient(URfsnNpcClientComponent* RfsnClient);

	/** Play audio from raw PCM data (16-bit signed, mono) */
	UFUNCTION(BlueprintCallable, Category = "TTS")
	void PlayAudioFromPCM(const TArray<uint8>& PCMData, int32 SampleRate = 22050);

	/** Stop current audio playback */
	UFUNCTION(BlueprintCallable, Category = "TTS")
	void StopAudio();

	/** Check if audio is currently playing */
	UFUNCTION(BlueprintPure, Category = "TTS")
	bool IsPlaying() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent;

	TArray<TPair<FString, TArray<uint8>>> AudioQueue;
	bool bIsPlaying = false;

	UFUNCTION()
	void OnRfsnSentence(const FRfsnSentence& Sentence);

	void ProcessNextInQueue();
	void OnAudioPlaybackFinished();
	void RequestTtsFromServer(const FString& Text);
};
