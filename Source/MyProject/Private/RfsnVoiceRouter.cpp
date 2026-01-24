// RFSN Voice Router Implementation

#include "RfsnVoiceRouter.h"
#include "RfsnEmotionBlend.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

URfsnVoiceRouter::URfsnVoiceRouter()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnVoiceRouter::BeginPlay()
{
	Super::BeginPlay();

	// Cache emotion blend reference
	EmotionBlend = GetOwner()->FindComponentByClass<URfsnEmotionBlend>();

	RFSN_LOG(TEXT("VoiceRouter initialized for %s (emotion: %s)"), *GetOwner()->GetName(),
	         EmotionBlend ? TEXT("available") : TEXT("not found"));
}

void URfsnVoiceRouter::Synthesize(const FRfsnTtsRequest& Request)
{
	// Determine backend
	ERfsnTtsBackend Backend = Request.bUseForced ? Request.ForcedBackend : DetermineBackend(Request);

	// Update stats
	if (Backend == ERfsnTtsBackend::ChatterboxFull)
	{
		FullRequestCount++;
	}
	else if (Backend == ERfsnTtsBackend::ChatterboxTurbo)
	{
		TurboRequestCount++;
	}

	LastUsedBackend = Backend;

	RFSN_LOG(TEXT("[VoiceRouter] %s → %s: \"%s\""), *Request.NpcId, *BackendToString(Backend), *Request.Text.Left(50));

	OnTtsRouted.Broadcast(Backend, Request.Text);
	SendToBackend(Backend, Request.Text, Request.Style);
}

void URfsnVoiceRouter::SynthesizeAuto(const FString& Text, ERfsnVoiceIntensity Intensity)
{
	FRfsnTtsRequest Request;
	Request.Text = Text;
	Request.Intensity = Intensity;

	// Build style from emotion if available
	if (bAutoRouteFromEmotion && EmotionBlend)
	{
		Request.Style = BuildStyleFromEmotion();
		Request.Intensity = GetIntensityFromEmotion();
	}
	else
	{
		Request.Style.Emotion = DefaultEmotion;
		Request.Style.Intensity = 0.5f;
	}

	// Get NPC ID
	if (URfsnNpcClientComponent* NpcClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		Request.NpcId = NpcClient->NpcId;
	}

	Synthesize(Request);
}

void URfsnVoiceRouter::SynthesizeBark(const FString& Text)
{
	FRfsnTtsRequest Request;
	Request.Text = Text;
	Request.bIsBark = true;
	Request.Intensity = ERfsnVoiceIntensity::Low;
	Request.Style.Emotion = DefaultEmotion;
	Request.Style.Intensity = 0.3f;

	if (URfsnNpcClientComponent* NpcClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		Request.NpcId = NpcClient->NpcId;
	}

	Synthesize(Request);
}

void URfsnVoiceRouter::SynthesizeStoryCritical(const FString& Text)
{
	FRfsnTtsRequest Request;
	Request.Text = Text;
	Request.bIsStoryCritical = true;
	Request.Intensity = ERfsnVoiceIntensity::High;

	if (bAutoRouteFromEmotion && EmotionBlend)
	{
		Request.Style = BuildStyleFromEmotion();
	}
	else
	{
		Request.Style.Emotion = DefaultEmotion;
		Request.Style.Intensity = 0.8f;
	}

	if (URfsnNpcClientComponent* NpcClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		Request.NpcId = NpcClient->NpcId;
	}

	Synthesize(Request);
}

ERfsnTtsBackend URfsnVoiceRouter::DetermineBackend(const FRfsnTtsRequest& Request) const
{
	// Rule 1: Story-critical always uses Full
	if (Request.bIsStoryCritical && bAlwaysFullForStoryCritical)
	{
		return ERfsnTtsBackend::ChatterboxFull;
	}

	// Rule 2: Barks always use Turbo
	if (Request.bIsBark && bAlwaysTurboForBarks)
	{
		return ERfsnTtsBackend::ChatterboxTurbo;
	}

	// Rule 3: Route by intensity
	switch (Request.Intensity)
	{
	case ERfsnVoiceIntensity::High:
		return ERfsnTtsBackend::ChatterboxFull;

	case ERfsnVoiceIntensity::Medium:
		// Turbo with boosted style (handled in synthesis)
		return ERfsnTtsBackend::ChatterboxTurbo;

	case ERfsnVoiceIntensity::Low:
	default:
		return ERfsnTtsBackend::ChatterboxTurbo;
	}
}

ERfsnVoiceIntensity URfsnVoiceRouter::GetIntensityFromEmotion() const
{
	if (!EmotionBlend)
	{
		return ERfsnVoiceIntensity::Low;
	}

	// Get VAD values
	float Arousal = EmotionBlend->CurrentEmotion.Arousal;
	float Valence = FMath::Abs(EmotionBlend->CurrentEmotion.Valence);

	// High arousal or extreme valence = high intensity
	if (Arousal >= HighArousalThreshold || Valence >= HighIntensityThreshold)
	{
		return ERfsnVoiceIntensity::High;
	}

	// Medium arousal = medium intensity
	if (Arousal >= 0.4f || Valence >= 0.4f)
	{
		return ERfsnVoiceIntensity::Medium;
	}

	return ERfsnVoiceIntensity::Low;
}

FRfsnVoiceStyle URfsnVoiceRouter::BuildStyleFromEmotion() const
{
	FRfsnVoiceStyle Style;
	Style.VoiceReferencePath = DefaultVoiceReference;

	if (!EmotionBlend)
	{
		Style.Emotion = DefaultEmotion;
		Style.Intensity = 0.5f;
		return Style;
	}

	// Map dominant emotion to style
	Style.Emotion = EmotionBlend->EmotionToString(EmotionBlend->DominantEmotion).ToLower();
	Style.Intensity = EmotionBlend->CurrentEmotion.Arousal;

	// Adjust pace based on arousal
	float Arousal = EmotionBlend->CurrentEmotion.Arousal;
	Style.PaceModifier = FMath::Lerp(0.9f, 1.3f, Arousal);

	// Adjust pitch based on valence
	float Valence = EmotionBlend->CurrentEmotion.Valence;
	Style.PitchModifier = FMath::Lerp(0.9f, 1.1f, (Valence + 1.0f) * 0.5f);

	return Style;
}

FString URfsnVoiceRouter::BackendToString(ERfsnTtsBackend Backend)
{
	switch (Backend)
	{
	case ERfsnTtsBackend::ChatterboxFull:
		return TEXT("Chatterbox-Full");
	case ERfsnTtsBackend::ChatterboxTurbo:
		return TEXT("Chatterbox-Turbo");
	case ERfsnTtsBackend::Qwen:
		return TEXT("Qwen3-TTS");
	case ERfsnTtsBackend::Kokoro:
		return TEXT("Kokoro");
	default:
		return TEXT("Unknown");
	}
}

FString URfsnVoiceRouter::GetUsageStats() const
{
	int32 Total = FullRequestCount + TurboRequestCount;
	if (Total == 0)
	{
		return TEXT("No TTS requests yet");
	}

	float FullPercent = (float)FullRequestCount / (float)Total * 100.0f;
	return FString::Printf(TEXT("Full: %d (%.1f%%), Turbo: %d (%.1f%%)"), FullRequestCount, FullPercent,
	                       TurboRequestCount, 100.0f - FullPercent);
}

void URfsnVoiceRouter::SendToBackend(ERfsnTtsBackend Backend, const FString& Text, const FRfsnVoiceStyle& Style)
{
	FString Endpoint = GetBackendEndpoint(Backend);
	if (Endpoint.IsEmpty())
	{
		RFSN_LOG(TEXT("VoiceRouter: No endpoint for %s"), *BackendToString(Backend));
		return;
	}

	// Build JSON request
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("text"), Text);
	JsonObject->SetStringField(TEXT("emotion"), Style.Emotion);
	JsonObject->SetNumberField(TEXT("intensity"), Style.Intensity);
	JsonObject->SetNumberField(TEXT("pace"), Style.PaceModifier);
	JsonObject->SetNumberField(TEXT("pitch"), Style.PitchModifier);

	if (!Style.VoiceReferencePath.IsEmpty())
	{
		JsonObject->SetStringField(TEXT("voice_reference"), Style.VoiceReferencePath);
	}

	// Serialize
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonString);

	// Handle response
	HttpRequest->OnProcessRequestComplete().BindLambda(
	    [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	    {
		    if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
		    {
			    FString AudioPath = Response->GetContentAsString();
			    OnTtsComplete.Broadcast(AudioPath);
		    }
		    else
		    {
			    RFSN_LOG(TEXT("VoiceRouter: TTS request failed"));
		    }
	    });

	HttpRequest->ProcessRequest();
}

FString URfsnVoiceRouter::GetBackendEndpoint(ERfsnTtsBackend Backend) const
{
	switch (Backend)
	{
	case ERfsnTtsBackend::ChatterboxFull:
		return ChatterboxFullEndpoint;
	case ERfsnTtsBackend::ChatterboxTurbo:
		return ChatterboxTurboEndpoint;
	case ERfsnTtsBackend::Qwen:
		return QwenEndpoint;
	default:
		return ChatterboxTurboEndpoint;
	}
}
