// RFSN Replicated Dialogue
// Network replication for multiplayer dialogue

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnReplicatedDialogue.generated.h"

/**
 * Component that handles network replication of RFSN dialogue.
 * Add to NPCs that need their dialogue synced across clients.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnReplicatedDialogue : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnReplicatedDialogue();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Replicate dialogue to all clients */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication")
	bool bReplicateDialogue = true;

	/** Only server makes RFSN requests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication")
	bool bServerAuthoritative = true;

	/** Replicate affinity/relationship changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication")
	bool bReplicateRelationship = true;

	// ─────────────────────────────────────────────────────────────
	// Replicated State
	// ─────────────────────────────────────────────────────────────

	/** Currently speaking NPC name */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSentence, BlueprintReadOnly, Category = "State")
	FString CurrentSentence;

	/** Current NPC action */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	ERfsnNpcAction CurrentAction = ERfsnNpcAction::Idle;

	/** Is dialogue active */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bDialogueActive = false;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Start dialogue (server only) */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void ServerStartDialogue(const FString& PlayerUtterance);

	/** Called on clients when sentence is replicated */
	UFUNCTION()
	void OnRep_CurrentSentence();

	/** Multicast RPC to show sentence on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowSentence(const FString& Sentence);

	/** Multicast RPC for action */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastNpcAction(ERfsnNpcAction Action);

	/** Server RPC to request dialogue */
	UFUNCTION(Server, Reliable)
	void ServerRequestDialogue(const FString& PlayerUtterance);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TWeakObjectPtr<URfsnNpcClientComponent> CachedClient;

	UFUNCTION()
	void OnLocalSentenceReceived(const FRfsnSentence& Sentence);

	UFUNCTION()
	void OnLocalActionReceived(ERfsnNpcAction Action);
};
