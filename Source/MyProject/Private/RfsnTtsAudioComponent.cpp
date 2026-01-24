// RFSN TTS Audio Component Implementation

#include "RfsnTtsAudioComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

URfsnTtsAudioComponent::URfsnTtsAudioComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void URfsnTtsAudioComponent::BeginPlay() {
  Super::BeginPlay();

  // Create audio component dynamically
  AActor *Owner = GetOwner();
  if (Owner) {
    AudioComponent =
        NewObject<UAudioComponent>(Owner, TEXT("TtsAudioComponent"));
    if (AudioComponent) {
      AudioComponent->RegisterComponent();
      AudioComponent->AttachToComponent(
          Owner->GetRootComponent(),
          FAttachmentTransformRules::KeepRelativeTransform);
      AudioComponent->bAutoActivate = false;
      AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
      AudioComponent->SetPitchMultiplier(PitchMultiplier);

      if (AttenuationSettings) {
        AudioComponent->AttenuationSettings = AttenuationSettings;
      }
    }
  }
}

void URfsnTtsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  StopAudio();
  Super::EndPlay(EndPlayReason);
}

void URfsnTtsAudioComponent::BindToRfsnClient(
    URfsnNpcClientComponent *RfsnClient) {
  if (RfsnClient) {
    RfsnClient->OnSentenceReceived.AddDynamic(
        this, &URfsnTtsAudioComponent::OnRfsnSentence);
  }
}

void URfsnTtsAudioComponent::OnRfsnSentence(const FRfsnSentence &Sentence) {
  // In a full implementation, the RFSN server would stream audio bytes
  // For now, we log and could integrate with a local TTS solution
  UE_LOG(LogTemp, Verbose, TEXT("[TTS] Received sentence: %s"),
         *Sentence.Sentence);

  // If audio data was included in the response, we would queue it here
  // AudioQueue.Add(MakeTuple(Sentence.Sentence, AudioBytes));
  // ProcessNextInQueue();
}

void URfsnTtsAudioComponent::PlayAudioFromPCM(const TArray<uint8> &PCMData,
                                              int32 SampleRate) {
  if (PCMData.Num() == 0 || !AudioComponent) {
    return;
  }

  // Create procedural sound wave
  USoundWaveProcedural *SoundWave = NewObject<USoundWaveProcedural>();
  if (!SoundWave) {
    return;
  }

  SoundWave->SetSampleRate(SampleRate);
  SoundWave->NumChannels = 1;
  SoundWave->Duration =
      static_cast<float>(PCMData.Num() / 2) / static_cast<float>(SampleRate);
  SoundWave->bLooping = false;

  // Queue the audio data
  SoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

  // Play the sound
  AudioComponent->SetSound(SoundWave);
  AudioComponent->Play();
  bIsPlaying = true;

  OnAudioStarted.Broadcast(TEXT(""));

  UE_LOG(LogTemp, Log, TEXT("[TTS] Playing audio: %.2fs"), SoundWave->Duration);
}

void URfsnTtsAudioComponent::StopAudio() {
  if (AudioComponent && AudioComponent->IsPlaying()) {
    AudioComponent->Stop();
  }
  bIsPlaying = false;
  AudioQueue.Empty();
}

bool URfsnTtsAudioComponent::IsPlaying() const {
  return AudioComponent ? AudioComponent->IsPlaying() : false;
}

void URfsnTtsAudioComponent::ProcessNextInQueue() {
  if (AudioQueue.Num() == 0) {
    bIsPlaying = false;
    OnAudioFinished.Broadcast();
    return;
  }

  auto &Next = AudioQueue[0];
  PlayAudioFromPCM(Next.Value, 22050);
  OnAudioStarted.Broadcast(Next.Key);
  AudioQueue.RemoveAt(0);
}

void URfsnTtsAudioComponent::OnAudioPlaybackFinished() { ProcessNextInQueue(); }
