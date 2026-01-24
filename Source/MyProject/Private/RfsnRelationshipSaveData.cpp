// RFSN Relationship Save Data Implementation

#include "RfsnRelationshipSaveData.h"

FRfsnNpcRelationship &
URfsnRelationshipSaveData::GetOrCreateRelationship(const FString &NpcId) {
  if (!NpcRelationships.Contains(NpcId)) {
    FRfsnNpcRelationship NewRelationship;
    NewRelationship.NpcId = NpcId;
    NewRelationship.Affinity = 0.0f;
    NewRelationship.Relationship = TEXT("Stranger");
    NewRelationship.InteractionCount = 0;
    NewRelationship.LastInteraction = FDateTime::Now();
    NpcRelationships.Add(NpcId, NewRelationship);
  }

  return NpcRelationships[NpcId];
}

void URfsnRelationshipSaveData::UpdateFromClient(const FString &NpcId,
                                                 float Affinity,
                                                 const FString &Relationship) {
  FRfsnNpcRelationship &Rel = GetOrCreateRelationship(NpcId);
  Rel.Affinity = Affinity;
  Rel.Relationship = Relationship;
  Rel.InteractionCount++;
  Rel.LastInteraction = FDateTime::Now();
  SaveTimestamp = FDateTime::Now();
}
