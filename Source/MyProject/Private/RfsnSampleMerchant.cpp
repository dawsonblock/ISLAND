// RFSN Sample Merchant Implementation

#include "RfsnSampleMerchant.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnNpcDialogueTrigger.h"

ARfsnSampleMerchant::ARfsnSampleMerchant() {
  // Create RFSN client with merchant personality
  RfsnClient =
      CreateDefaultSubobject<URfsnNpcClientComponent>(TEXT("RfsnClient"));

  // Create proximity-based dialogue trigger
  DialogueTrigger =
      CreateDefaultSubobject<URfsnNpcDialogueTrigger>(TEXT("DialogueTrigger"));

  // Default inventory
  InventoryItems.Add(TEXT("Health Potion"));
  InventoryItems.Add(TEXT("Ammo Pack"));
  InventoryItems.Add(TEXT("Flashlight"));
}

void ARfsnSampleMerchant::BeginPlay() {
  Super::BeginPlay();

  // Configure RFSN client for merchant personality
  if (RfsnClient) {
    RfsnClient->NpcName = TEXT("Merchant");
    RfsnClient->NpcId = FString::Printf(TEXT("merchant_%s"), *GetName());
    RfsnClient->Mood = TEXT("Friendly");
    RfsnClient->Relationship = TEXT("Shopkeeper");
    RfsnClient->Affinity = 0.3f; // Slightly positive

    // Bind events
    RfsnClient->OnNpcActionReceived.AddDynamic(
        this, &ARfsnSampleMerchant::OnRfsnAction);
    RfsnClient->OnSentenceReceived.AddDynamic(
        this, &ARfsnSampleMerchant::OnDialogueSentence);
  }

  // Configure trigger
  if (DialogueTrigger) {
    DialogueTrigger->RfsnClient = RfsnClient;
    DialogueTrigger->TriggerMode = ERfsnDialogueTriggerMode::Proximity;
    DialogueTrigger->ProximityRadius = 250.0f;
    DialogueTrigger->DefaultPrompt = TEXT("Looking to trade?");
    DialogueTrigger->Cooldown = 8.0f;
  }

  UE_LOG(LogTemp, Log, TEXT("[Merchant] %s ready for trading with %d items"),
         *RfsnClient->NpcName, InventoryItems.Num());
}

void ARfsnSampleMerchant::OnRfsnAction(ERfsnNpcAction Action) {
  switch (Action) {
  case ERfsnNpcAction::Trade:
    UE_LOG(LogTemp, Log, TEXT("[Merchant] Opening trade interface"));
    // Could open trade UI here
    break;

  case ERfsnNpcAction::Offer:
    UE_LOG(LogTemp, Log, TEXT("[Merchant] Making special offer"));
    break;

  case ERfsnNpcAction::Refuse:
    UE_LOG(LogTemp, Log, TEXT("[Merchant] Refusing transaction"));
    break;

  case ERfsnNpcAction::Greet:
    UE_LOG(LogTemp, Log, TEXT("[Merchant] Greeting customer"));
    break;

  default:
    break;
  }
}

void ARfsnSampleMerchant::OnDialogueSentence(const FRfsnSentence &Sentence) {
  // Could trigger lip sync or gesture animations here
  UE_LOG(LogTemp, Verbose, TEXT("[Merchant] Says: %s"), *Sentence.Sentence);
}
