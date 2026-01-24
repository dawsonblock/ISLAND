// RFSN Dialogue Widget Implementation

#include "RfsnDialogueWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"

URfsnDialogueWidget::URfsnDialogueWidget() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URfsnDialogueWidget::BeginPlay() { Super::BeginPlay(); }

void URfsnDialogueWidget::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(ClearTimer);
  }
  Super::EndPlay(EndPlayReason);
}

void URfsnDialogueWidget::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (bIsShowingDialogue && bUseTypewriter) {
    UpdateTypewriter(DeltaTime);
  }
}

void URfsnDialogueWidget::BindToRfsnClient(
    URfsnNpcClientComponent *RfsnClient) {
  if (RfsnClient) {
    RfsnClient->OnSentenceReceived.AddDynamic(
        this, &URfsnDialogueWidget::OnRfsnSentenceReceived);
    CurrentNpcName = RfsnClient->NpcName;
  }
}

void URfsnDialogueWidget::OnRfsnSentenceReceived(
    const FRfsnSentence &Sentence) {
  if (!Sentence.Sentence.IsEmpty()) {
    // Queue the sentence
    SentenceQueue.Add(Sentence.Sentence);

    // Start processing if not already
    if (!bIsProcessingQueue) {
      ProcessNextSentence();
    }
  }
}

void URfsnDialogueWidget::DisplaySentence(const FString &NpcName,
                                          const FString &Sentence) {
  CurrentNpcName = NpcName;
  CurrentFullText = Sentence;

  if (bUseTypewriter) {
    CurrentDisplayText = TEXT("");
    TypewriterProgress = 0.0f;
    SetComponentTickEnabled(true);
  } else {
    CurrentDisplayText = Sentence;
  }

  bIsShowingDialogue = true;
  OnDialogueDisplayed.Broadcast(NpcName, Sentence);

  UE_LOG(LogTemp, Log, TEXT("[Dialogue] %s: %s"), *NpcName, *Sentence);

  // Set clear timer
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(ClearTimer);
    World->GetTimerManager().SetTimer(ClearTimer, this,
                                      &URfsnDialogueWidget::OnSentenceTimeout,
                                      SentenceDisplayDuration, false);
  }
}

void URfsnDialogueWidget::ClearDialogue() {
  CurrentDisplayText = TEXT("");
  CurrentFullText = TEXT("");
  bIsShowingDialogue = false;
  SetComponentTickEnabled(false);

  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(ClearTimer);
  }
}

void URfsnDialogueWidget::ProcessNextSentence() {
  if (SentenceQueue.Num() > 0) {
    bIsProcessingQueue = true;
    FString NextSentence = SentenceQueue[0];
    SentenceQueue.RemoveAt(0);
    DisplaySentence(CurrentNpcName, NextSentence);
  } else {
    bIsProcessingQueue = false;
  }
}

void URfsnDialogueWidget::UpdateTypewriter(float DeltaTime) {
  if (CurrentDisplayText.Len() >= CurrentFullText.Len()) {
    // Typewriter complete
    SetComponentTickEnabled(false);
    return;
  }

  TypewriterProgress += DeltaTime * TypewriterSpeed;
  int32 CharsToShow = FMath::FloorToInt(TypewriterProgress);
  CharsToShow = FMath::Min(CharsToShow, CurrentFullText.Len());

  CurrentDisplayText = CurrentFullText.Left(CharsToShow);
}

void URfsnDialogueWidget::OnSentenceTimeout() {
  // Process next sentence or clear
  if (SentenceQueue.Num() > 0) {
    ProcessNextSentence();
  } else {
    ClearDialogue();
  }
}
