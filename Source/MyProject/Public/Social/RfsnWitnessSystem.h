// RFSN Witness System
// NPCs observe and share information about player actions
// Creates emergent reputation and rumor spreading

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RfsnWitnessSystem.generated.h"

class URfsnNpcClientComponent;
class URfsnFactionSystem;

/**
 * Type of witnessed event
 */
UENUM(BlueprintType)
enum class ERfsnWitnessEventType : uint8
{
	Combat UMETA(DisplayName = "Combat"),       // Player fought someone
	Theft UMETA(DisplayName = "Theft"),         // Player stole something
	Help UMETA(DisplayName = "Help"),           // Player helped someone
	Trade UMETA(DisplayName = "Trade"),         // Player made a deal
	Murder UMETA(DisplayName = "Murder"),       // Player killed someone
	Dialogue UMETA(DisplayName = "Dialogue"),   // Player said something notable
	QuestComplete UMETA(DisplayName = "Quest"), // Player completed objective
	Trespass UMETA(DisplayName = "Trespass"),   // Player entered restricted area
	Custom UMETA(DisplayName = "Custom")        // Custom event type
};

/**
 * A witnessed event record
 */
USTRUCT(BlueprintType)
struct FRfsnWitnessEvent
{
	GENERATED_BODY()

	/** Unique event ID */
	UPROPERTY(BlueprintReadOnly, Category = "Witness")
	FGuid EventId;

	/** Type of event */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	ERfsnWitnessEventType EventType = ERfsnWitnessEventType::Custom;

	/** Description of what happened */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FString Description;

	/** Location where it happened */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FVector Location = FVector::ZeroVector;

	/** Location name (if known) */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FString LocationName;

	/** The victim/target NPC ID (if any) */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FString TargetNpcId;

	/** Target's faction */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FString TargetFaction;

	/** When it happened (game time) */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	float GameTimeWhenOccurred = 0.0f;

	/** When it happened (real time) */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	FDateTime RealTimeWhenOccurred;

	/** How important/impactful (affects spread speed) */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	float Importance = 0.5f;

	/** Is this positive for player reputation? */
	UPROPERTY(BlueprintReadWrite, Category = "Witness")
	bool bIsPositive = false;

	/** NPCs who have heard about this */
	UPROPERTY(BlueprintReadOnly, Category = "Witness")
	TArray<FString> InformedNpcs;

	/** Original witnesses */
	UPROPERTY(BlueprintReadOnly, Category = "Witness")
	TArray<FString> OriginalWitnesses;

	/** Has this expired from memory? */
	UPROPERTY(BlueprintReadOnly, Category = "Witness")
	bool bExpired = false;

	/** Constructor */
	FRfsnWitnessEvent()
	{
		EventId = FGuid::NewGuid();
		RealTimeWhenOccurred = FDateTime::Now();
	}
};

/**
 * NPC's knowledge about an event (may be distorted)
 */
USTRUCT(BlueprintType)
struct FRfsnEventKnowledge
{
	GENERATED_BODY()

	/** Reference to the actual event */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	FGuid EventId;

	/** How this NPC heard about it (witness, rumor, etc) */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	FString Source;

	/** Accuracy of their knowledge (1.0 = saw it, decreases with rumor chain) */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	float Accuracy = 1.0f;

	/** Their opinion of the event (-1 to 1) */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	float Opinion = 0.0f;

	/** Can they talk about this? */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	bool bWillGossip = true;

	/** How many times have they shared this? */
	UPROPERTY(BlueprintReadOnly, Category = "Knowledge")
	int32 ShareCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEventWitnessed, const FRfsnWitnessEvent&, Event, const FString&,
                                             WitnessNpcId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRumorSpread, const FGuid&, EventId, const FString&, FromNpc,
                                               const FString&, ToNpc);

/**
 * Witness System Subsystem
 * Tracks player actions witnessed by NPCs and manages rumor spreading
 */
UCLASS()
class MYPROJECT_API URfsnWitnessSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Max distance NPCs can witness events */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Witness|Config")
	float WitnessRadius = 1500.0f;

	/** Chance for rumor to spread per tick (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Witness|Config")
	float RumorSpreadChance = 0.1f;

	/** How much accuracy degrades per rumor hop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Witness|Config")
	float AccuracyDecayPerHop = 0.15f;

	/** Events older than this (game hours) are forgotten */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Witness|Config")
	float MemoryDurationHours = 72.0f;

	/** Max events to track */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Witness|Config")
	int32 MaxTrackedEvents = 100;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when an NPC witnesses an event */
	UPROPERTY(BlueprintAssignable, Category = "Witness|Events")
	FOnEventWitnessed OnEventWitnessed;

	/** Called when a rumor spreads to a new NPC */
	UPROPERTY(BlueprintAssignable, Category = "Witness|Events")
	FOnRumorSpread OnRumorSpread;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Record a player action that can be witnessed */
	UFUNCTION(BlueprintCallable, Category = "Witness")
	FGuid RecordPlayerAction(ERfsnWitnessEventType EventType, const FString& Description, FVector Location,
	                         const FString& TargetNpcId = TEXT(""), float Importance = 0.5f, bool bPositive = false);

	/** Find NPCs who witnessed an event at a location */
	UFUNCTION(BlueprintCallable, Category = "Witness")
	TArray<FString> FindWitnesses(FVector Location, float Radius = -1.0f);

	/** Check if NPC knows about an event */
	UFUNCTION(BlueprintPure, Category = "Witness")
	bool DoesNpcKnow(const FString& NpcId, const FGuid& EventId) const;

	/** Get NPC's knowledge about an event */
	UFUNCTION(BlueprintPure, Category = "Witness")
	FRfsnEventKnowledge GetNpcKnowledge(const FString& NpcId, const FGuid& EventId) const;

	/** Get all events an NPC knows about */
	UFUNCTION(BlueprintPure, Category = "Witness")
	TArray<FRfsnWitnessEvent> GetNpcKnownEvents(const FString& NpcId) const;

	/** Get all recent events (for LLM context) */
	UFUNCTION(BlueprintPure, Category = "Witness")
	TArray<FRfsnWitnessEvent> GetRecentEvents(int32 MaxCount = 10) const;

	/** Get gossip string for NPC (for dialogue) */
	UFUNCTION(BlueprintPure, Category = "Witness")
	FString GetGossipForNpc(const FString& NpcId) const;

	/** Get context string for LLM prompt */
	UFUNCTION(BlueprintPure, Category = "Witness")
	FString GetWitnessContext(const FString& NpcId) const;

	/** Manually spread a rumor from one NPC to another */
	UFUNCTION(BlueprintCallable, Category = "Witness")
	void SpreadRumor(const FGuid& EventId, const FString& FromNpc, const FString& ToNpc);

	/** Tick rumor spreading (call periodically) */
	UFUNCTION(BlueprintCallable, Category = "Witness")
	void TickRumorSpreading();

	/** Clean up expired events */
	UFUNCTION(BlueprintCallable, Category = "Witness")
	void CleanupExpiredEvents();

private:
	/** All tracked events */
	UPROPERTY()
	TArray<FRfsnWitnessEvent> AllEvents;

	/** NPC knowledge map: NpcId -> (EventId -> Knowledge) */
	TMap<FString, TMap<FGuid, FRfsnEventKnowledge>> NpcKnowledge;

	/** Register NPC as witness to event */
	void RegisterWitness(const FGuid& EventId, const FString& NpcId, float Accuracy = 1.0f,
	                     const FString& Source = TEXT("witnessed"));

	/** Get event by ID */
	FRfsnWitnessEvent* FindEvent(const FGuid& EventId);
	const FRfsnWitnessEvent* FindEvent(const FGuid& EventId) const;

	/** Calculate opinion based on faction and event type */
	float CalculateOpinion(const FString& NpcFaction, const FRfsnWitnessEvent& Event) const;

	/** Get event type as string */
	static FString EventTypeToString(ERfsnWitnessEventType Type);
};
