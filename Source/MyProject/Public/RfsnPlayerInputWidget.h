// RFSN Player Dialogue Input Widget
// Slate-based text input for player-NPC conversations

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RfsnPlayerInputWidget.generated.h"

class URfsnNpcClientComponent;
class UEditableTextBox;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerInputSubmitted,
                                            const FString &, PlayerText);

/**
 * Widget for player to type dialogue input to NPCs.
 * Provides a text box and submit button for conversation.
 */
UCLASS()
class MYPROJECT_API URfsnPlayerInputWidget : public UUserWidget {
  GENERATED_BODY()

public:
  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** Reference to the NPC's RFSN client */
  UPROPERTY(BlueprintReadWrite, Category = "RFSN")
  TObjectPtr<URfsnNpcClientComponent> TargetNpc;

  /** Placeholder text for input box */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
  FText PlaceholderText = FText::FromString(TEXT("Type your message..."));

  /** Maximum input length */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
  int32 MaxInputLength = 500;

  // ─────────────────────────────────────────────────────────────
  // Events
  // ─────────────────────────────────────────────────────────────

  /** Called when player submits text */
  UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
  FOnPlayerInputSubmitted OnInputSubmitted;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Show the input widget and focus input */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void ShowAndFocus();

  /** Hide the input widget */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void HideInput();

  /** Submit current text to NPC */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SubmitInput();

  /** Set the NPC name display */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SetNpcName(const FString &Name);

protected:
  virtual void NativeConstruct() override;

  /** Input text box - bind in Blueprint */
  UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
  TObjectPtr<UEditableTextBox> InputTextBox;

  /** Submit button - bind in Blueprint */
  UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
  TObjectPtr<UButton> SubmitButton;

  /** NPC name display - bind in Blueprint */
  UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
  TObjectPtr<UTextBlock> NpcNameText;

  UFUNCTION()
  void OnSubmitButtonClicked();

  UFUNCTION()
  void OnTextCommitted(const FText &Text, ETextCommit::Type CommitMethod);
};
