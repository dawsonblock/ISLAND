// RFSN Audio Settings
// Distance-based audio attenuation for dialogue

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnAudioSettings.generated.h"

class URfsnTtsAudioComponent;

/**
 * Component for managing dialogue audio attenuation based on distance.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnAudioSettings : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnAudioSettings();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Enable distance-based attenuation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bEnableAttenuation = true;

	/** Inner radius - full volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Attenuation")
	float InnerRadius = 200.0f;

	/** Outer radius - minimum volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Attenuation")
	float OuterRadius = 1000.0f;

	/** Minimum volume at outer radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Attenuation",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinVolume = 0.1f;

	/** Maximum volume at inner radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Attenuation",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxVolume = 1.0f;

	/** Attenuation curve (1.0 = linear, 2.0 = quadratic falloff) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Attenuation")
	float AttenuationExponent = 1.5f;

	/** Enable occlusion (walls block sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Occlusion")
	bool bEnableOcclusion = true;

	/** Volume multiplier when occluded */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Occlusion",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OcclusionMultiplier = 0.3f;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get current volume based on player distance */
	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetDistanceVolume() const;

	/** Check if sound is occluded by geometry */
	UFUNCTION(BlueprintPure, Category = "Audio")
	bool IsOccluded() const;

	/** Get final volume (distance + occlusion) */
	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetFinalVolume() const;

	/** Apply current volume to TTS component */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ApplyToTtsComponent(URfsnTtsAudioComponent* TtsComponent);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	float GetDistanceToPlayer() const;
	bool DoOcclusionTrace() const;
};
