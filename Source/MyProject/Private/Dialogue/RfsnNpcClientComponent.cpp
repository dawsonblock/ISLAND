// RFSN NPC Client Component Implementation
// HTTP SSE streaming client for RFSN Orchestrator

#include "Dialogue/RfsnNpcClientComponent.h"
#include "Dialogue/RfsnEmotionBlend.h"
#include "Social/RfsnBackstoryGenerator.h"
#include "Social/RfsnRelationshipManager.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

URfsnNpcClientComponent::URfsnNpcClientComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnNpcClientComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-register with RelationshipManager to sync saved state
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (URfsnRelationshipManager* RelMgr = GI->GetSubsystem<URfsnRelationshipManager>())
		{
			RelMgr->RegisterNpcClient(this);
		}
	}
}

void URfsnNpcClientComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelDialogue();

	// Unregister to save relationship state
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (URfsnRelationshipManager* RelMgr = GI->GetSubsystem<URfsnRelationshipManager>())
			{
				RelMgr->UnregisterNpcClient(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}


void URfsnNpcClientComponent::SendPlayerUtterance(const FString& PlayerText)
{
	CancelDialogue();

	if (URfsnBackstoryGenerator* BackstoryGen = GetOwner()->FindComponentByClass<URfsnBackstoryGenerator>())
	{
		BackstoryGen->OnFirstInteraction();
	}

	FString CurrentMood = Mood;
	FString DialogueTone = TEXT("");
	if (URfsnEmotionBlend* EmotionBlend = GetOwner()->FindComponentByClass<URfsnEmotionBlend>())
	{
		CurrentMood = EmotionBlend->ToMoodString();
		DialogueTone = EmotionBlend->ToDialogueTone();
	}

	FString BackstoryContext = TEXT("");
	if (URfsnBackstoryGenerator* BackstoryGen = GetOwner()->FindComponentByClass<URfsnBackstoryGenerator>())
	{
		BackstoryContext = BackstoryGen->GetShortContext();
	}

	TSharedPtr<FJsonObject> NpcState = MakeShareable(new FJsonObject());
	NpcState->SetStringField(TEXT("npc_name"), NpcName);
	NpcState->SetStringField(TEXT("npc_id"), NpcId);
	NpcState->SetNumberField(TEXT("affinity"), Affinity);
	NpcState->SetStringField(TEXT("mood"), CurrentMood);
	NpcState->SetStringField(TEXT("relationship"), Relationship);
	NpcState->SetStringField(TEXT("dialogue_tone"), DialogueTone);
	NpcState->SetStringField(TEXT("backstory_context"), BackstoryContext);

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetStringField(TEXT("user_input"), PlayerText);
	Payload->SetObjectField(TEXT("npc_state"), NpcState);
	Payload->SetStringField(TEXT("tts_engine"), TtsEngine);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

	ResetStreamState();
	ActiveRequestId = FGuid::NewGuid();

	CurrentRequest = FHttpModule::Get().CreateRequest();
	if (!CurrentRequest.IsValid())
	{
		EnterFallbackMode(TEXT("failed to create dialogue request"));
		return;
	}

	CurrentRequest->SetURL(OrchestratorUrl);
	CurrentRequest->SetVerb(TEXT("POST"));
	CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	CurrentRequest->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
	CurrentRequest->SetContentAsString(JsonString);

	CurrentRequest->OnRequestProgress64().BindUObject(this, &URfsnNpcClientComponent::OnStreamProgress);
	CurrentRequest->OnProcessRequestComplete().BindUObject(this, &URfsnNpcClientComponent::OnStreamComplete);

	bIsStreaming = true;

	UE_LOG(LogTemp, Log, TEXT("[RFSN] Sending utterance to %s (request=%s): %s"),
	       *NpcName, *ActiveRequestId.ToString(), *PlayerText);

	if (!CurrentRequest->ProcessRequest())
	{
		EnterFallbackMode(TEXT("failed to start dialogue request"));
	}
}

void URfsnNpcClientComponent::CancelDialogue()
{
	if (CurrentRequest.IsValid())
	{
		CurrentRequest->CancelRequest();
		CurrentRequest.Reset();
	}

	bIsStreaming = false;
	ActiveRequestId.Invalidate();
	ResetStreamState();
}

void URfsnNpcClientComponent::ResetStreamState()
{
	bReceivedMeta = false;
	bReceivedAnyEvent = false;
	bFallbackMode = false;
	RawStreamBuffer.Empty();
	PendingLineFragment.Empty();
	LastProcessedOffset = 0;
	ProcessedEventLines.Empty();
}

void URfsnNpcClientComponent::ProcessPendingStreamData(const FString& NewChunk)
{
	if (NewChunk.IsEmpty())
	{
		return;
	}

	PendingLineFragment += NewChunk;
	ProcessPendingLineFragment(false);
}

void URfsnNpcClientComponent::ProcessPendingLineFragment(bool bFlushTrailingFragment)
{
	int32 NewlineIndex = INDEX_NONE;
	while (PendingLineFragment.FindChar(TEXT('\n'), NewlineIndex))
	{
		FString Line = PendingLineFragment.Left(NewlineIndex).Replace(TEXT("\r"), TEXT("")).TrimStartAndEnd();
		PendingLineFragment = PendingLineFragment.Mid(NewlineIndex + 1);

		if (!Line.IsEmpty() && !ProcessedEventLines.Contains(Line))
		{
			ProcessedEventLines.Add(Line);
			ProcessSSELine(Line);
		}
	}

	if (bFlushTrailingFragment)
	{
		const FString TrailingLine = PendingLineFragment.Replace(TEXT("\r"), TEXT("")).TrimStartAndEnd();
		PendingLineFragment.Empty();

		if (!TrailingLine.IsEmpty() && !ProcessedEventLines.Contains(TrailingLine))
		{
			ProcessedEventLines.Add(TrailingLine);
			ProcessSSELine(TrailingLine);
		}
	}
}

void URfsnNpcClientComponent::OnStreamProgress(FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived)
{
	if (!Request.IsValid() || !CurrentRequest.IsValid() || CurrentRequest.Get() != Request.Get())
	{
		return;
	}

	FHttpResponsePtr Response = Request->GetResponse();
	if (!Response.IsValid())
	{
		return;
	}

	const FString Content = Response->GetContentAsString();
	if (Content.Len() <= LastProcessedOffset)
	{
		return;
	}

	const FString NewChunk = Content.Mid(LastProcessedOffset);
	LastProcessedOffset = Content.Len();
	RawStreamBuffer = Content;
	ProcessPendingStreamData(NewChunk);
}

void URfsnNpcClientComponent::OnStreamComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bIsStreaming = false;

	if (!Request.IsValid() || !CurrentRequest.IsValid() || CurrentRequest.Get() != Request.Get())
	{
		return;
	}

	if (Response.IsValid())
	{
		const FString FinalContent = Response->GetContentAsString();
		if (FinalContent.Len() > LastProcessedOffset)
		{
			const FString NewChunk = FinalContent.Mid(LastProcessedOffset);
			LastProcessedOffset = FinalContent.Len();
			RawStreamBuffer = FinalContent;
			ProcessPendingStreamData(NewChunk);
		}
	}

	ProcessPendingLineFragment(true);
	CurrentRequest.Reset();

	if (!bReceivedAnyEvent)
	{
		const FString FailureReason = Response.IsValid()
			? FString::Printf(TEXT("backend unavailable (HTTP %d)"), Response->GetResponseCode())
			: TEXT("backend unavailable");
		EnterFallbackMode(FailureReason);
		return;
	}

	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RFSN] Stream ended in degraded mode for %s (request=%s)"),
		       *NpcName, *ActiveRequestId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[RFSN] Dialogue stream complete for %s (request=%s)"),
		       *NpcName, *ActiveRequestId.ToString());
	}

	ActiveRequestId.Invalidate();
	OnDialogueComplete.Broadcast();
}

void URfsnNpcClientComponent::ProcessSSELine(const FString& Line)
{
	if (!Line.StartsWith(TEXT("data:")))
	{
		return;
	}

	FString JsonData = Line.Mid(5).TrimStartAndEnd();
	if (JsonData.IsEmpty())
	{
		return;
	}

	if (JsonData == TEXT("[DONE]"))
	{
		return;
	}

	if (JsonData.Contains(TEXT("\"npc_action\"")))
	{
		if (!bReceivedMeta)
		{
			ParseMetaEvent(JsonData);
		}
		return;
	}

	if (JsonData.Contains(TEXT("\"sentence\"")))
	{
		ParseSentenceEvent(JsonData);
	}
}

void URfsnNpcClientComponent::ParseMetaEvent(const FString& JsonData)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RFSN] Failed to parse meta event: %s"), *JsonData);
		return;
	}

	FRfsnDialogueMeta Meta;
	JsonObject->TryGetStringField(TEXT("player_signal"), Meta.PlayerSignal);
	JsonObject->TryGetStringField(TEXT("bandit_key"), Meta.BanditKey);
	JsonObject->TryGetStringField(TEXT("action_mode"), Meta.ActionMode);

	FString ActionString;
	if (JsonObject->TryGetStringField(TEXT("npc_action"), ActionString))
	{
		Meta.NpcAction = ParseNpcAction(ActionString);
		LastNpcAction = Meta.NpcAction;
	}

	JsonObject->TryGetStringField(TEXT("instant_bark"), Meta.InstantBark);
	JsonObject->TryGetNumberField(TEXT("bark_duration_ms"), Meta.BarkDurationMs);

	bReceivedMeta = true;
	bReceivedAnyEvent = true;

	UE_LOG(LogTemp, Log, TEXT("[RFSN] Meta: action=%s, mode=%s, signal=%s, bark='%s'"), *ActionString, *Meta.ActionMode,
	       *Meta.PlayerSignal, *Meta.InstantBark.Left(30));

	OnMetaReceived.Broadcast(Meta);
	OnNpcActionReceived.Broadcast(Meta.NpcAction);
}

void URfsnNpcClientComponent::ParseSentenceEvent(const FString& JsonData)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RFSN] Failed to parse sentence event: %s"), *JsonData);
		return;
	}

	FRfsnSentence Sentence;
	JsonObject->TryGetStringField(TEXT("sentence"), Sentence.Sentence);
	JsonObject->TryGetBoolField(TEXT("is_final"), Sentence.bIsFinal);

	double LatencyDouble = 0.0;
	if (JsonObject->TryGetNumberField(TEXT("latency_ms"), LatencyDouble))
	{
		Sentence.LatencyMs = static_cast<float>(LatencyDouble);
	}

	if (!Sentence.Sentence.IsEmpty())
	{
		if (URfsnEmotionBlend* EmotionBlend = GetOwner()->FindComponentByClass<URfsnEmotionBlend>())
		{
			if (LastNpcAction == ERfsnNpcAction::Attack || LastNpcAction == ERfsnNpcAction::Threaten)
			{
				EmotionBlend->ApplyStimulus(TEXT("Anger"), 0.5f);
			}
			else if (LastNpcAction == ERfsnNpcAction::Flee)
			{
				EmotionBlend->ApplyStimulus(TEXT("Fear"), 0.5f);
			}
			else if (LastNpcAction == ERfsnNpcAction::Greet || LastNpcAction == ERfsnNpcAction::Help)
			{
				EmotionBlend->ApplyStimulus(TEXT("Joy"), 0.3f);
			}
		}

		bReceivedAnyEvent = true;
		UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *NpcName, *Sentence.Sentence);
		OnSentenceReceived.Broadcast(Sentence);
	}
}

void URfsnNpcClientComponent::EnterFallbackMode(const FString& Reason)
{
	if (bFallbackMode)
	{
		return;
	}

	bFallbackMode = true;
	bIsStreaming = false;
	CurrentRequest.Reset();

	UE_LOG(LogTemp, Warning, TEXT("[RFSN] Falling back to local bark for %s (request=%s): %s"),
	       *NpcName, *ActiveRequestId.ToString(), *Reason);

	EmitLocalBark();
	ActiveRequestId.Invalidate();
}

void URfsnNpcClientComponent::EmitLocalBark()
{
	FRfsnDialogueMeta Meta;
	Meta.PlayerSignal = TEXT("offline");
	Meta.ActionMode = TEXT("OfflineFallback");
	Meta.NpcAction = ERfsnNpcAction::Talk;
	Meta.InstantBark = TEXT("Stay focused.");
	Meta.BarkDurationMs = 800;
	LastNpcAction = Meta.NpcAction;
	bReceivedMeta = true;
	bReceivedAnyEvent = true;

	OnMetaReceived.Broadcast(Meta);
	OnNpcActionReceived.Broadcast(Meta.NpcAction);

	FRfsnSentence Sentence;
	Sentence.Sentence = FString::Printf(TEXT("%s keeps watch and says, \"Stay focused. We can talk after this.\""), *NpcName);
	Sentence.bIsFinal = true;
	Sentence.LatencyMs = 0.0f;
	OnSentenceReceived.Broadcast(Sentence);
	OnDialogueComplete.Broadcast();
}

ERfsnNpcAction URfsnNpcClientComponent::ParseNpcAction(const FString& ActionString)
{
	FString Upper = ActionString.ToUpper();

	if (Upper == TEXT("GREET"))
		return ERfsnNpcAction::Greet;
	if (Upper == TEXT("WARN"))
		return ERfsnNpcAction::Warn;
	if (Upper == TEXT("IDLE"))
		return ERfsnNpcAction::Idle;
	if (Upper == TEXT("FLEE"))
		return ERfsnNpcAction::Flee;
	if (Upper == TEXT("ATTACK"))
		return ERfsnNpcAction::Attack;
	if (Upper == TEXT("TRADE"))
		return ERfsnNpcAction::Trade;
	if (Upper == TEXT("OFFER"))
		return ERfsnNpcAction::Offer;
	if (Upper == TEXT("APOLOGIZE"))
		return ERfsnNpcAction::Apologize;
	if (Upper == TEXT("THREATEN"))
		return ERfsnNpcAction::Threaten;
	if (Upper == TEXT("HELP"))
		return ERfsnNpcAction::Help;
	if (Upper == TEXT("REQUEST"))
		return ERfsnNpcAction::Request;
	if (Upper == TEXT("AGREE"))
		return ERfsnNpcAction::Agree;
	if (Upper == TEXT("DISAGREE"))
		return ERfsnNpcAction::Disagree;
	if (Upper == TEXT("ACCEPT"))
		return ERfsnNpcAction::Accept;
	if (Upper == TEXT("REFUSE"))
		return ERfsnNpcAction::Refuse;
	if (Upper == TEXT("IGNORE"))
		return ERfsnNpcAction::Ignore;
	if (Upper == TEXT("INQUIRE"))
		return ERfsnNpcAction::Inquire;
	if (Upper == TEXT("EXPLAIN"))
		return ERfsnNpcAction::Explain;
	if (Upper == TEXT("ANSWER"))
		return ERfsnNpcAction::Answer;

	return ERfsnNpcAction::Talk;
}
