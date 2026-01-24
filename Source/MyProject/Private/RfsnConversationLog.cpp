// RFSN Conversation Log Implementation

#include "RfsnConversationLog.h"
#include "RfsnNpcClientComponent.h"

URfsnConversationLog::URfsnConversationLog() {
  PrimaryComponentTick.bCanEverTick = false;
}

void URfsnConversationLog::LogPlayerMessage(const FString &Message) {
  FRfsnConversationEntry Entry;
  Entry.Speaker = TEXT("Player");
  Entry.Message = Message;
  Entry.Timestamp = FDateTime::Now();
  Entry.bIsPlayer = true;

  Entries.Add(Entry);

  // Trim if over max
  while (Entries.Num() > MaxEntries) {
    Entries.RemoveAt(0);
  }

  OnConversationUpdated.Broadcast(Entry);
}

void URfsnConversationLog::LogNpcMessage(const FString &NpcName,
                                         const FString &Message) {
  FRfsnConversationEntry Entry;
  Entry.Speaker = NpcName;
  Entry.Message = Message;
  Entry.Timestamp = FDateTime::Now();
  Entry.bIsPlayer = false;

  Entries.Add(Entry);

  // Trim if over max
  while (Entries.Num() > MaxEntries) {
    Entries.RemoveAt(0);
  }

  OnConversationUpdated.Broadcast(Entry);
}

TArray<FRfsnConversationEntry>
URfsnConversationLog::GetRecentEntries(int32 Count) const {
  TArray<FRfsnConversationEntry> Recent;
  int32 Start = FMath::Max(0, Entries.Num() - Count);
  for (int32 i = Start; i < Entries.Num(); ++i) {
    Recent.Add(Entries[i]);
  }
  return Recent;
}

void URfsnConversationLog::ClearLog() { Entries.Empty(); }

void URfsnConversationLog::BindToRfsnClient(URfsnNpcClientComponent *Client) {
  if (Client) {
    BoundNpcName = Client->NpcName;
    Client->OnSentenceReceived.AddDynamic(
        this, &URfsnConversationLog::OnRfsnSentence);
  }
}

void URfsnConversationLog::OnRfsnSentence(const FRfsnSentence &Sentence) {
  if (!Sentence.Sentence.IsEmpty()) {
    LogNpcMessage(BoundNpcName, Sentence.Sentence);
  }
}
