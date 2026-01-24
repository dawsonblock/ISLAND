// Horror NPC Implementation

#include "Variant_Horror/HorrorNPC.h"
#include "Components/AudioComponent.h"
#include "RfsnDialogueWidget.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnNpcDialogueTrigger.h"

AHorrorNPC::AHorrorNPC() {
  // Create RFSN client
  RfsnClient =
      CreateDefaultSubobject<URfsnNpcClientComponent>(TEXT("RfsnClient"));

  // Create dialogue trigger
  DialogueTrigger =
      CreateDefaultSubobject<URfsnNpcDialogueTrigger>(TEXT("DialogueTrigger"));

  // Create voice audio component
  VoiceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("VoiceAudio"));
  VoiceAudio->SetupAttachment(RootComponent);
  VoiceAudio->bAutoActivate = false;
}

void AHorrorNPC::BeginPlay() {
  Super::BeginPlay();

  CurrentHP = MaxHP;

  // Configure RFSN client
  if (RfsnClient) {
    RfsnClient->NpcName = NpcDisplayName;
    RfsnClient->NpcId = FString::Printf(TEXT("horror_npc_%s"), *GetName());
    RfsnClient->Mood = bIsHostile ? TEXT("Hostile") : TEXT("Wary");
    RfsnClient->OnNpcActionReceived.AddDynamic(this,
                                               &AHorrorNPC::OnRfsnNpcAction);
    RfsnClient->OnSentenceReceived.AddDynamic(this,
                                              &AHorrorNPC::OnRfsnSentence);
  }

  // Configure trigger
  if (DialogueTrigger) {
    DialogueTrigger->RfsnClient = RfsnClient;
    DialogueTrigger->ProximityRadius = AggroRadius;
  }
}

void AHorrorNPC::SpeakToPlayer(const FString &PlayerUtterance) {
  if (RfsnClient) {
    RfsnClient->SendPlayerUtterance(PlayerUtterance);
  }
}

void AHorrorNPC::TakeDamageAmount(float Damage) {
  CurrentHP -= Damage;

  // Update mood based on damage
  if (RfsnClient && CurrentHP < MaxHP * 0.5f) {
    RfsnClient->Mood = TEXT("Desperate");
  }

  if (CurrentHP <= 0.0f) {
    CurrentHP = 0.0f;
    OnDeath.Broadcast();
    // Could trigger death behavior here
  }
}

void AHorrorNPC::OnRfsnNpcAction(ERfsnNpcAction Action) {
  switch (Action) {
  case ERfsnNpcAction::Attack:
  case ERfsnNpcAction::Threaten:
    bIsHostile = true;
    UE_LOG(LogTemp, Log, TEXT("[%s] RFSN: Becoming hostile"), *NpcDisplayName);
    break;

  case ERfsnNpcAction::Flee:
    bIsHostile = false;
    UE_LOG(LogTemp, Log, TEXT("[%s] RFSN: Fleeing"), *NpcDisplayName);
    // Trigger flee behavior in AI controller
    break;

  case ERfsnNpcAction::Warn:
    UE_LOG(LogTemp, Log, TEXT("[%s] RFSN: Warning player"), *NpcDisplayName);
    break;

  case ERfsnNpcAction::Greet:
  case ERfsnNpcAction::Help:
    bIsHostile = false;
    UE_LOG(LogTemp, Log, TEXT("[%s] RFSN: Friendly interaction"),
           *NpcDisplayName);
    break;

  default:
    UE_LOG(LogTemp, Log, TEXT("[%s] RFSN action: %d"), *NpcDisplayName,
           static_cast<int32>(Action));
    break;
  }
}

void AHorrorNPC::OnRfsnSentence(const FRfsnSentence &Sentence) {
  // Log for now - TTS audio would be triggered here
  UE_LOG(LogTemp, Log, TEXT("[%s] Says: %s"), *NpcDisplayName,
         *Sentence.Sentence);

  // If we had TTS audio data, we would play it here:
  // VoiceAudio->Play();
}
