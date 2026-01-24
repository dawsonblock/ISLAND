// RFSN NPC Client Component
// Connects NPCs to the RFSN Orchestrator for LLM-driven dialogue

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "RfsnNpcClientComponent.generated.h"

UENUM(BlueprintType)
enum class ERfsnNpcAction : uint8
{
	Greet,
	Warn,
	Idle,
	Flee,
	Attack,
	Trade,
	Talk,
	Apologize,
	Threaten,
	Help,
	Request,
	Agree,
	Disagree,
	Ignore,
	Inquire,
	Explain
};

USTRUCT(BlueprintType)
struct FRfsnDialogueMeta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	FString PlayerSignal;

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	FString BanditKey;

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	ERfsnNpcAction NpcAction = ERfsnNpcAction::Talk;

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	FString ActionMode;
};

USTRUCT(BlueprintType)
struct FRfsnSentence
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	FString Sentence;

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	bool bIsFinal = false;

	UPROPERTY(BlueprintReadOnly, Category = "RFSN")
	float LatencyMs = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRfsnMetaReceived, const FRfsnDialogueMeta&, Meta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRfsnSentenceReceived, const FRfsnSentence&, Sentence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRfsnNpcActionReceived, ERfsnNpcAction, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRfsnDialogueComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRfsnError, const FString&, ErrorMessage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcClientComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcClientComponent();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** URL of the RFSN Orchestrator streaming endpoint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Config")
	FString OrchestratorUrl = TEXT("http://127.0.0.1:8000/api/dialogue/stream");

	/** Unique identifier for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Config")
	FString NpcId = TEXT("npc_001");

	/** Display name for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Config")
	FString NpcName = TEXT("NPC");

	/** Current mood state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|State")
	FString Mood = TEXT("Neutral");

	/** Relationship to player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|State")
	FString Relationship = TEXT("Stranger");

	/** Affinity score (-1 to 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|State", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Affinity = 0.0f;

	/** TTS engine to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RFSN|Config")
	FString TtsEngine = TEXT("piper");

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when meta event is received (contains NPC action) */
	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRfsnMetaReceived OnMetaReceived;

	/** Called for each sentence in the dialogue stream */
	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRfsnSentenceReceived OnSentenceReceived;

	/** Called when NPC action is determined */
	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRfsnNpcActionReceived OnNpcActionReceived;

	/** Called when dialogue stream completes */
	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRfsnDialogueComplete OnDialogueComplete;

	/** Called on connection or parsing error */
	UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
	FOnRfsnError OnError;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Send player utterance to RFSN and stream response */
	UFUNCTION(BlueprintCallable, Category = "RFSN")
	void SendPlayerUtterance(const FString& PlayerText);

	/** Cancel any in-progress dialogue stream */
	UFUNCTION(BlueprintCallable, Category = "RFSN")
	void CancelDialogue();

	/** Check if dialogue is currently streaming */
	UFUNCTION(BlueprintPure, Category = "RFSN")
	bool IsDialogueActive() const { return bIsStreaming; }

	/** Get the last received NPC action */
	UFUNCTION(BlueprintPure, Category = "RFSN")
	ERfsnNpcAction GetLastNpcAction() const { return LastNpcAction; }

	/** Convert action string to enum */
	UFUNCTION(BlueprintPure, Category = "RFSN")
	static ERfsnNpcAction ParseNpcAction(const FString& ActionString);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;
	bool bIsStreaming = false;
	bool bGotMeta = false;
	ERfsnNpcAction LastNpcAction = ERfsnNpcAction::Talk;
	FString StreamBuffer;

	void OnStreamProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived);
	void OnStreamComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void ProcessSSELine(const FString& Line);
	void ParseMetaEvent(const FString& JsonData);
	void ParseSentenceEvent(const FString& JsonData);
};
