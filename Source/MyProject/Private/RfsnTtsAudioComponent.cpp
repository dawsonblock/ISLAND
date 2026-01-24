// RFSN TTS Audio Component Implementation

#include "RfsnTtsAudioComponent.h"
#include "RfsnVoiceRouter.h"
#include "RfsnEmotionBlend.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

URfsnTtsAudioComponent::URfsnTtsAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnTtsAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create audio component dynamically
	AActor* Owner = GetOwner();
	if (Owner)
	{
		AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("TtsAudioComponent"));
		if (AudioComponent)
		{
			AudioComponent->RegisterComponent();
			AudioComponent->AttachToComponent(Owner->GetRootComponent(),
			                                  FAttachmentTransformRules::KeepRelativeTransform);
			AudioComponent->bAutoActivate = false;
			AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
			AudioComponent->SetPitchMultiplier(PitchMultiplier);

			if (AttenuationSettings)
			{
				AudioComponent->AttenuationSettings = AttenuationSettings;
			}
		}
	}
}

void URfsnTtsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAudio();
	Super::EndPlay(EndPlayReason);
}

void URfsnTtsAudioComponent::BindToRfsnClient(URfsnNpcClientComponent* RfsnClient)
{
	if (RfsnClient)
	{
		RfsnClient->OnSentenceReceived.AddDynamic(this, &URfsnTtsAudioComponent::OnRfsnSentence);
	}
}

void URfsnTtsAudioComponent::OnRfsnSentence(const FRfsnSentence& Sentence)
{
	// Route through VoiceRouter if available for intelligent TTS model selection
	URfsnVoiceRouter* VoiceRouter = GetOwner()->FindComponentByClass<URfsnVoiceRouter>();

	if (VoiceRouter)
	{
		// Determine intensity from emotion blend
		ERfsnVoiceIntensity Intensity = VoiceRouter->GetIntensityFromEmotion();

		// For final sentences or longer text, potentially use higher quality
		if (Sentence.bIsFinal && Sentence.Sentence.Len() > 50)
		{
			Intensity = ERfsnVoiceIntensity::High;
		}

		VoiceRouter->SynthesizeAuto(Sentence.Sentence, Intensity);

		UE_LOG(LogTemp, Log, TEXT("[TTS] Routed to Chatterbox: %s"), *Sentence.Sentence.Left(50));
	}
	else
	{
		// Fallback: direct HTTP call to default TTS
		RequestTtsFromServer(Sentence.Sentence);
	}
}

void URfsnTtsAudioComponent::PlayAudioFromPCM(const TArray<uint8>& PCMData, int32 SampleRate)
{
	if (PCMData.Num() == 0 || !AudioComponent)
	{
		return;
	}

	// Create procedural sound wave
	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>();
	if (!SoundWave)
	{
		return;
	}

	SoundWave->SetSampleRate(SampleRate);
	SoundWave->NumChannels = 1;
	SoundWave->Duration = static_cast<float>(PCMData.Num() / 2) / static_cast<float>(SampleRate);
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

void URfsnTtsAudioComponent::StopAudio()
{
	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}
	bIsPlaying = false;
	AudioQueue.Empty();
}

bool URfsnTtsAudioComponent::IsPlaying() const
{
	return AudioComponent ? AudioComponent->IsPlaying() : false;
}

void URfsnTtsAudioComponent::ProcessNextInQueue()
{
	if (AudioQueue.Num() == 0)
	{
		bIsPlaying = false;
		OnAudioFinished.Broadcast();
		return;
	}

	auto& Next = AudioQueue[0];
	PlayAudioFromPCM(Next.Value, 22050);
	OnAudioStarted.Broadcast(Next.Key);
	AudioQueue.RemoveAt(0);
}

void URfsnTtsAudioComponent::OnAudioPlaybackFinished()
{
	ProcessNextInQueue();
}

void URfsnTtsAudioComponent::RequestTtsFromServer(const FString& Text)
{
	// Fallback TTS endpoint (Chatterbox Turbo default)
	FString Endpoint = TEXT("http://localhost:8001/synthesize/turbo");

	// Build JSON request
	FString JsonContent = FString::Printf(TEXT("{\"text\":\"%s\",\"emotion\":\"neutral\",\"intensity\":0.5}"), *Text);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonContent);

	HttpRequest->OnProcessRequestComplete().BindLambda(
	    [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	    {
		    if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
		    {
			    UE_LOG(LogTemp, Log, TEXT("[TTS] Fallback synthesis complete"));
			    // Audio path returned - would load and play
		    }
		    else
		    {
			    UE_LOG(LogTemp, Warning, TEXT("[TTS] Fallback synthesis failed"));
		    }
	    });

	HttpRequest->ProcessRequest();
	UE_LOG(LogTemp, Log, TEXT("[TTS] Fallback request sent: %s"), *Text.Left(50));
}
