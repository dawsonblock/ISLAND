// RFSN Conversation Log Component
// Tracks dialogue history for display and persistence

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RfsnConversationLog.generated.h"
#include "RfsnNpcClientComponent.h"

USTRUCT(BlueprintType)
struct FRfsnConversationEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Conversation")
  FString Speaker;

  UPROPERTY(BlueprintReadOnly, Category = "Conversation")
  FString Message;

  UPROPERTY(BlueprintReadOnly, Category = "Conversation")
  FDateTime Timestamp;

  UPROPERTY(BlueprintReadOnly, Category = "Conversation")
  bool bIsPlayer = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationUpdated,
                                            const FRfsnConversationEntry &,
                                            Entry);

/**
 * Component that logs conversation history with NPCs.
 * Can be attached to player or used globally.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnConversationLog : public UActorComponent {
  GENERATED_BODY()

public:
  URfsnConversationLog();

  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** Maximum entries to keep in log */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
  int32 MaxEntries = 50;

  // ─────────────────────────────────────────────────────────────
  // Events
  // ─────────────────────────────────────────────────────────────

  UPROPERTY(BlueprintAssignable, Category = "Conversation|Events")
  FOnConversationUpdated OnConversationUpdated;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Add player message to log */
  UFUNCTION(BlueprintCallable, Category = "Conversation")
  void LogPlayerMessage(const FString &Message);

  /** Add NPC message to log */
  UFUNCTION(BlueprintCallable, Category = "Conversation")
  void LogNpcMessage(const FString &NpcName, const FString &Message);

  /** Get all conversation entries */
  UFUNCTION(BlueprintPure, Category = "Conversation")
  const TArray<FRfsnConversationEntry> &GetEntries() const { return Entries; }

  /** Get last N entries */
  UFUNCTION(BlueprintCallable, Category = "Conversation")
  TArray<FRfsnConversationEntry> GetRecentEntries(int32 Count) const;

  /** Clear conversation log */
  UFUNCTION(BlueprintCallable, Category = "Conversation")
  void ClearLog();

  /** Bind to RFSN client for auto-logging */
  UFUNCTION(BlueprintCallable, Category = "Conversation")
  void BindToRfsnClient(URfsnNpcClientComponent *Client);

protected:
  UPROPERTY()
  TArray<FRfsnConversationEntry> Entries;

private:
  UFUNCTION()
  void OnRfsnSentence(const FRfsnSentence &Sentence);

  FString BoundNpcName;
};
