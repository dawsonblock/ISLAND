// RFSN Player Dialogue Input Widget Implementation

#include "RfsnPlayerInputWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "RfsnNpcClientComponent.h"

void URfsnPlayerInputWidget::NativeConstruct() {
  Super::NativeConstruct();

  // Bind button click
  if (SubmitButton) {
    SubmitButton->OnClicked.AddDynamic(
        this, &URfsnPlayerInputWidget::OnSubmitButtonClicked);
  }

  // Bind text commit (Enter key)
  if (InputTextBox) {
    InputTextBox->OnTextCommitted.AddDynamic(
        this, &URfsnPlayerInputWidget::OnTextCommitted);
    InputTextBox->SetHintText(PlaceholderText);
  }
}

void URfsnPlayerInputWidget::ShowAndFocus() {
  SetVisibility(ESlateVisibility::Visible);

  if (InputTextBox) {
    InputTextBox->SetText(FText::GetEmpty());
    InputTextBox->SetKeyboardFocus();
  }
}

void URfsnPlayerInputWidget::HideInput() {
  SetVisibility(ESlateVisibility::Hidden);

  if (InputTextBox) {
    InputTextBox->SetText(FText::GetEmpty());
  }
}

void URfsnPlayerInputWidget::SubmitInput() {
  if (!InputTextBox) {
    return;
  }

  FString Text = InputTextBox->GetText().ToString();

  // Validate
  if (Text.IsEmpty()) {
    return;
  }

  // Truncate if too long
  if (Text.Len() > MaxInputLength) {
    Text = Text.Left(MaxInputLength);
  }

  // Send to NPC
  if (TargetNpc) {
    TargetNpc->SendPlayerUtterance(Text);
  }

  // Broadcast event
  OnInputSubmitted.Broadcast(Text);

  // Clear input
  InputTextBox->SetText(FText::GetEmpty());

  UE_LOG(LogTemp, Log, TEXT("[PlayerInput] Submitted: %s"), *Text);
}

void URfsnPlayerInputWidget::SetNpcName(const FString &Name) {
  if (NpcNameText) {
    NpcNameText->SetText(
        FText::FromString(FString::Printf(TEXT("Speaking to: %s"), *Name)));
  }
}

void URfsnPlayerInputWidget::OnSubmitButtonClicked() { SubmitInput(); }

void URfsnPlayerInputWidget::OnTextCommitted(const FText &Text,
                                             ETextCommit::Type CommitMethod) {
  // Submit on Enter key
  if (CommitMethod == ETextCommit::OnEnter) {
    SubmitInput();
  }
}
