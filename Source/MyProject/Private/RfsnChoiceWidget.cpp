// RFSN Player Choice Widget Implementation

#include "RfsnChoiceWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "RfsnNpcClientComponent.h"

void URfsnChoiceWidget::NativeConstruct() {
  Super::NativeConstruct();
  SetDefaultChoices();
}

void URfsnChoiceWidget::ShowChoices(const TArray<FString> &InChoices) {
  Choices = InChoices;
  CreateChoiceButtons();
  SetVisibility(ESlateVisibility::Visible);
}

void URfsnChoiceWidget::SetDefaultChoices() {
  Choices.Empty();
  Choices.Add(TEXT("Hello."));
  Choices.Add(TEXT("What's going on?"));
  Choices.Add(TEXT("I need help."));
  Choices.Add(TEXT("Goodbye."));
}

void URfsnChoiceWidget::SetAggressiveChoices() {
  Choices.Empty();
  Choices.Add(TEXT("Get out of my way."));
  Choices.Add(TEXT("I'm warning you..."));
  Choices.Add(TEXT("You'll regret this."));
  Choices.Add(TEXT("[Attack]"));
}

void URfsnChoiceWidget::SetFriendlyChoices() {
  Choices.Empty();
  Choices.Add(TEXT("Nice to meet you!"));
  Choices.Add(TEXT("How can I help?"));
  Choices.Add(TEXT("Tell me more."));
  Choices.Add(TEXT("Thank you."));
}

void URfsnChoiceWidget::HideChoices() {
  SetVisibility(ESlateVisibility::Hidden);
  ClearChoiceButtons();
}

void URfsnChoiceWidget::SelectChoice(int32 Index) {
  if (!Choices.IsValidIndex(Index)) {
    return;
  }

  FString ChoiceText = Choices[Index];

  // Send to NPC
  if (TargetNpc) {
    TargetNpc->SendPlayerUtterance(ChoiceText);
  }

  // Broadcast event
  OnChoiceSelected.Broadcast(Index, ChoiceText);

  // Hide after selection
  HideChoices();

  UE_LOG(LogTemp, Log, TEXT("[ChoiceWidget] Selected: %s"), *ChoiceText);
}

void URfsnChoiceWidget::CreateChoiceButtons() {
  ClearChoiceButtons();

  if (!ChoiceContainer) {
    UE_LOG(LogTemp, Warning, TEXT("[ChoiceWidget] No ChoiceContainer bound"));
    return;
  }

  for (int32 i = 0; i < Choices.Num(); ++i) {
    // Create button dynamically
    UButton *Button = NewObject<UButton>(this);
    if (!Button) {
      continue;
    }

    // Create text block for button
    UTextBlock *ButtonText = NewObject<UTextBlock>(Button);
    if (ButtonText) {
      ButtonText->SetText(FText::FromString(Choices[i]));
      Button->AddChild(ButtonText);
    }

    // Bind click - using index capture
    int32 CapturedIndex = i;
    Button->OnClicked.AddDynamic(this, &URfsnChoiceWidget::SelectChoice);
    // Note: In actual UMG, we'd need a different approach for index binding

    ChoiceContainer->AddChild(Button);
    ChoiceButtons.Add(Button);
  }
}

void URfsnChoiceWidget::ClearChoiceButtons() {
  if (ChoiceContainer) {
    ChoiceContainer->ClearChildren();
  }
  ChoiceButtons.Empty();
}
