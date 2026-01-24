// RFSN Lip Sync Implementation

#include "RfsnLipSync.h"
#include "RfsnLogging.h"
#include "RfsnTtsAudioComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"

URfsnLipSync::URfsnLipSync()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f; // ~60 FPS for smooth animation
}

void URfsnLipSync::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find skeletal mesh if not set
	if (!TargetMesh)
	{
		TargetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}

	// Find TTS component
	TtsComponent = GetOwner()->FindComponentByClass<URfsnTtsAudioComponent>();

	// Setup default mappings if empty
	if (VisemeMappings.Num() == 0)
	{
		SetupDefaultMappings();
	}

	// Initialize viseme weights
	ResetVisemeWeights();

	RFSN_LOG(TEXT("LipSync initialized for %s"), *GetOwner()->GetName());
}

void URfsnLipSync::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentState.bIsPlaying)
	{
		// Decay to silence
		CurrentState.SmoothedAmplitude = FMath::FInterpTo(CurrentState.SmoothedAmplitude, 0.0f, DeltaTime, 8.0f);
		CurrentState.JawOpen = CurrentState.SmoothedAmplitude;
		ApplyToMesh();
		return;
	}

	// Update amplitude from audio source
	UpdateAmplitude();

	// Smooth the amplitude
	CurrentState.SmoothedAmplitude = FMath::FInterpTo(CurrentState.SmoothedAmplitude, CurrentState.Amplitude, DeltaTime,
	                                                  (1.0f - SmoothingFactor) * 20.0f);

	// Calculate jaw opening
	if (CurrentState.SmoothedAmplitude > AmplitudeThreshold)
	{
		CurrentState.JawOpen = FMath::Clamp(CurrentState.SmoothedAmplitude * JawAmplitudeScale, 0.0f, 1.0f);
	}
	else
	{
		CurrentState.JawOpen = FMath::FInterpTo(CurrentState.JawOpen, 0.0f, DeltaTime, 10.0f);
	}

	// Generate viseme if not using simple mode
	if (!bUseSimpleMode)
	{
		ERfsnViseme NewViseme = GeneratePseudoViseme();
		if (NewViseme != CurrentState.CurrentViseme)
		{
			ERfsnViseme OldViseme = CurrentState.CurrentViseme;
			CurrentState.CurrentViseme = NewViseme;
			OnVisemeChanged.Broadcast(NewViseme);
		}
	}

	// Apply to mesh
	ApplyToMesh();
}

void URfsnLipSync::StartLipSync(UAudioComponent* InAudioSource)
{
	AudioSource = InAudioSource;
	CurrentState.bIsPlaying = true;
	CurrentState.CurrentViseme = ERfsnViseme::Silence;

	RFSN_LOG(TEXT("LipSync started"));
}

void URfsnLipSync::StopLipSync()
{
	CurrentState.bIsPlaying = false;
	AudioSource = nullptr;

	RFSN_LOG(TEXT("LipSync stopped"));
}

void URfsnLipSync::SetAmplitude(float NewAmplitude)
{
	CurrentState.Amplitude = FMath::Clamp(NewAmplitude, 0.0f, 1.0f);
}

void URfsnLipSync::SetViseme(ERfsnViseme Viseme)
{
	if (Viseme != CurrentState.CurrentViseme)
	{
		CurrentState.CurrentViseme = Viseme;
		OnVisemeChanged.Broadcast(Viseme);
	}
}

void URfsnLipSync::ApplyToMesh()
{
	if (!TargetMesh)
	{
		return;
	}

	// Apply jaw opening
	TargetMesh->SetMorphTarget(JawOpenMorphTarget, CurrentState.JawOpen);

	if (bUseSimpleMode)
	{
		// Simple mode: just use amplitude for basic open/close
		// Apply to common lip shapes
		TargetMesh->SetMorphTarget(FName("MouthOpen"), CurrentState.JawOpen);
		TargetMesh->SetMorphTarget(FName("AA"), CurrentState.JawOpen * 0.8f);
	}
	else
	{
		// Viseme mode: blend between viseme shapes
		for (auto& Pair : VisemeWeights)
		{
			float TargetWeight = (Pair.Key == CurrentState.CurrentViseme) ? 1.0f : 0.0f;
			Pair.Value = FMath::FInterpTo(Pair.Value, TargetWeight, GetWorld()->GetDeltaSeconds(), VisemeChangeSpeed);
		}

		// Apply all viseme weights
		for (const FRfsnVisemeMapping& Mapping : VisemeMappings)
		{
			float* Weight = VisemeWeights.Find(Mapping.Viseme);
			if (Weight)
			{
				TargetMesh->SetMorphTarget(Mapping.MorphTargetName, (*Weight) * Mapping.WeightMultiplier);
			}
		}
	}
}

void URfsnLipSync::SetupDefaultMappings()
{
	VisemeMappings.Empty();

	// Common morph target names - adjust to match your character mesh
	auto AddMapping = [this](ERfsnViseme Viseme, const FName& MorphName, float Weight = 1.0f)
	{
		FRfsnVisemeMapping Mapping;
		Mapping.Viseme = Viseme;
		Mapping.MorphTargetName = MorphName;
		Mapping.WeightMultiplier = Weight;
		VisemeMappings.Add(Mapping);
	};

	AddMapping(ERfsnViseme::Silence, FName("MouthClosed"), 1.0f);
	AddMapping(ERfsnViseme::AA, FName("AA"), 1.0f);
	AddMapping(ERfsnViseme::AO, FName("AO"), 1.0f);
	AddMapping(ERfsnViseme::EE, FName("EE"), 1.0f);
	AddMapping(ERfsnViseme::EH, FName("EH"), 1.0f);
	AddMapping(ERfsnViseme::IH, FName("IH"), 1.0f);
	AddMapping(ERfsnViseme::OH, FName("OH"), 1.0f);
	AddMapping(ERfsnViseme::OO, FName("OO"), 1.0f);
	AddMapping(ERfsnViseme::UH, FName("UH"), 1.0f);
	AddMapping(ERfsnViseme::CDG, FName("CDG"), 0.8f);
	AddMapping(ERfsnViseme::FV, FName("FV"), 0.9f);
	AddMapping(ERfsnViseme::L, FName("L"), 0.7f);
	AddMapping(ERfsnViseme::MBP, FName("MBP"), 1.0f);
	AddMapping(ERfsnViseme::TH, FName("TH"), 0.8f);
	AddMapping(ERfsnViseme::WQ, FName("WQ"), 0.9f);
}

void URfsnLipSync::UpdateAmplitude()
{
	if (!AudioSource || !AudioSource->IsPlaying())
	{
		CurrentState.Amplitude = 0.0f;
		CurrentState.bIsPlaying = false;
		return;
	}

	// Get audio envelope/amplitude
	// In a real implementation, you'd analyze the audio buffer
	// For now, we simulate with randomized amplitude when playing

	// Check if we can get real audio data
	// UE5 provides audio analysis through the AudioMixer module

	// Fallback: generate pseudo-random amplitude based on time
	// This creates a natural-looking mouth movement
	float Time = GetWorld()->GetTimeSeconds();
	float BaseAmplitude = 0.5f + 0.5f * FMath::Sin(Time * 15.0f);
	float Variation = 0.3f * FMath::Sin(Time * 37.0f) + 0.2f * FMath::Sin(Time * 23.0f);

	CurrentState.Amplitude = FMath::Clamp(BaseAmplitude + Variation, 0.0f, 1.0f);
}

ERfsnViseme URfsnLipSync::GeneratePseudoViseme() const
{
	// Generate pseudo-viseme based on amplitude pattern
	// In production, this would come from phoneme analysis

	float Amp = CurrentState.SmoothedAmplitude;

	if (Amp < AmplitudeThreshold)
	{
		return ERfsnViseme::Silence;
	}

	// Pseudo-random viseme selection weighted by amplitude
	float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	int32 Seed = FMath::FloorToInt(Time * 10.0f);

	// Weight toward vowels for high amplitude, consonants for low
	if (Amp > 0.7f)
	{
		// Vowel sounds
		TArray<ERfsnViseme> Vowels = {ERfsnViseme::AA, ERfsnViseme::EE, ERfsnViseme::OH, ERfsnViseme::OO};
		return Vowels[Seed % Vowels.Num()];
	}
	else if (Amp > 0.4f)
	{
		// Mid sounds
		TArray<ERfsnViseme> Mids = {ERfsnViseme::EH, ERfsnViseme::IH, ERfsnViseme::UH, ERfsnViseme::AO};
		return Mids[Seed % Mids.Num()];
	}
	else
	{
		// Consonant sounds
		TArray<ERfsnViseme> Cons = {ERfsnViseme::CDG, ERfsnViseme::FV, ERfsnViseme::MBP, ERfsnViseme::L};
		return Cons[Seed % Cons.Num()];
	}
}

void URfsnLipSync::ResetVisemeWeights()
{
	VisemeWeights.Empty();

	VisemeWeights.Add(ERfsnViseme::Silence, 1.0f);
	VisemeWeights.Add(ERfsnViseme::AA, 0.0f);
	VisemeWeights.Add(ERfsnViseme::AO, 0.0f);
	VisemeWeights.Add(ERfsnViseme::EE, 0.0f);
	VisemeWeights.Add(ERfsnViseme::EH, 0.0f);
	VisemeWeights.Add(ERfsnViseme::IH, 0.0f);
	VisemeWeights.Add(ERfsnViseme::OH, 0.0f);
	VisemeWeights.Add(ERfsnViseme::OO, 0.0f);
	VisemeWeights.Add(ERfsnViseme::UH, 0.0f);
	VisemeWeights.Add(ERfsnViseme::CDG, 0.0f);
	VisemeWeights.Add(ERfsnViseme::FV, 0.0f);
	VisemeWeights.Add(ERfsnViseme::L, 0.0f);
	VisemeWeights.Add(ERfsnViseme::MBP, 0.0f);
	VisemeWeights.Add(ERfsnViseme::TH, 0.0f);
	VisemeWeights.Add(ERfsnViseme::WQ, 0.0f);
}
