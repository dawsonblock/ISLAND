// RFSN NPC Client Component Implementation
// HTTP SSE streaming client for RFSN Orchestrator

#include "RfsnNpcClientComponent.h"
#include "RfsnRelationshipManager.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameInstance.h"

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
	// Cancel any existing stream
	CancelDialogue();

	// Build JSON payload matching RFSN DialogueRequest schema
	TSharedPtr<FJsonObject> NpcState = MakeShareable(new FJsonObject());
	NpcState->SetStringField(TEXT("npc_name"), NpcName);
	NpcState->SetStringField(TEXT("npc_id"), NpcId);
	NpcState->SetNumberField(TEXT("affinity"), Affinity);
	NpcState->SetStringField(TEXT("mood"), Mood);
	NpcState->SetStringField(TEXT("relationship"), Relationship);

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetStringField(TEXT("user_input"), PlayerText);
	Payload->SetObjectField(TEXT("npc_state"), NpcState);
	Payload->SetStringField(TEXT("tts_engine"), TtsEngine);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

	// Create HTTP request
	CurrentRequest = FHttpModule::Get().CreateRequest();
	CurrentRequest->SetURL(OrchestratorUrl);
	CurrentRequest->SetVerb(TEXT("POST"));
	CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	CurrentRequest->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
	CurrentRequest->SetContentAsString(JsonString);

	// Bind streaming callbacks
	CurrentRequest->OnRequestProgress().BindUObject(this, &URfsnNpcClientComponent::OnStreamProgress);
	CurrentRequest->OnProcessRequestComplete().BindUObject(this, &URfsnNpcClientComponent::OnStreamComplete);

	// Reset state
	bIsStreaming = true;
	bGotMeta = false;
	StreamBuffer.Empty();

	UE_LOG(LogTemp, Log, TEXT("[RFSN] Sending utterance to %s: %s"), *NpcName, *PlayerText);

	CurrentRequest->ProcessRequest();
}

void URfsnNpcClientComponent::CancelDialogue()
{
	if (CurrentRequest.IsValid() && bIsStreaming)
	{
		CurrentRequest->CancelRequest();
		CurrentRequest.Reset();
	}
	bIsStreaming = false;
	bGotMeta = false;
	StreamBuffer.Empty();
}

void URfsnNpcClientComponent::OnStreamProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	if (!Request.IsValid())
	{
		return;
	}

	FHttpResponsePtr Response = Request->GetResponse();
	if (!Response.IsValid())
	{
		return;
	}

	// Accumulate response content
	FString Content = Response->GetContentAsString();

	// Process any complete lines we haven't seen yet
	int32 SearchStart = StreamBuffer.Len();
	StreamBuffer = Content;

	// Find and process complete lines
	TArray<FString> Lines;
	StreamBuffer.ParseIntoArrayLines(Lines);

	for (const FString& Line : Lines)
	{
		if (!Line.IsEmpty())
		{
			ProcessSSELine(Line);
		}
	}
}

void URfsnNpcClientComponent::OnStreamComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bIsStreaming = false;

	if (!bSuccess || !Response.IsValid())
	{
		FString ErrorMsg = TEXT("Connection failed");
		if (Response.IsValid())
		{
			ErrorMsg =
			    FString::Printf(TEXT("HTTP %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
		}
		UE_LOG(LogTemp, Error, TEXT("[RFSN] Error: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return;
	}

	// Process any remaining content
	FString FinalContent = Response->GetContentAsString();
	if (FinalContent.Len() > StreamBuffer.Len())
	{
		TArray<FString> Lines;
		FinalContent.ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			if (!Line.IsEmpty())
			{
				ProcessSSELine(Line);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[RFSN] Dialogue stream complete for %s"), *NpcName);
	OnDialogueComplete.Broadcast();
}

void URfsnNpcClientComponent::ProcessSSELine(const FString& Line)
{
	// SSE format: "data: {...json...}"
	if (!Line.StartsWith(TEXT("data:")))
	{
		return;
	}

	FString JsonData = Line.Mid(5).TrimStartAndEnd();
	if (JsonData.IsEmpty())
	{
		return;
	}

	// Try meta event first (has npc_action field)
	if (!bGotMeta && JsonData.Contains(TEXT("\"npc_action\"")))
	{
		bGotMeta = true;
		ParseMetaEvent(JsonData);
		return;
	}

	// Sentence event (has sentence field)
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

	UE_LOG(LogTemp, Log, TEXT("[RFSN] Meta: action=%s, mode=%s, signal=%s"), *ActionString, *Meta.ActionMode,
	       *Meta.PlayerSignal);

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
		UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *NpcName, *Sentence.Sentence);
		OnSentenceReceived.Broadcast(Sentence);
	}
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
