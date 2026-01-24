// RFSN Player Choice Widget
// Displays multiple-choice dialogue responses for the player

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RfsnChoiceWidget.generated.h"

class UVerticalBox;
class UButton;
class UTextBlock;
class URfsnNpcClientComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChoiceSelected, int32,
                                             ChoiceIndex, const FString &,
                                             ChoiceText);

/**
 * Widget displaying multiple dialogue choices for player selection.
 * Shows 2-4 response options that send different messages to the NPC.
 */
UCLASS()
class MYPROJECT_API URfsnChoiceWidget : public UUserWidget {
  GENERATED_BODY()

public:
  // ─────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────

  /** Target NPC for sending responses */
  UPROPERTY(BlueprintReadWrite, Category = "RFSN")
  TObjectPtr<URfsnNpcClientComponent> TargetNpc;

  /** Available choices */
  UPROPERTY(BlueprintReadWrite, Category = "RFSN")
  TArray<FString> Choices;

  /** Choice button style class */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
  TSubclassOf<UUserWidget> ChoiceButtonClass;

  // ─────────────────────────────────────────────────────────────
  // Events
  // ─────────────────────────────────────────────────────────────

  UPROPERTY(BlueprintAssignable, Category = "RFSN|Events")
  FOnChoiceSelected OnChoiceSelected;

  // ─────────────────────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────────────────────

  /** Show the widget with the given choices */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void ShowChoices(const TArray<FString> &InChoices);

  /** Set default dialogue choices based on context */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SetDefaultChoices();

  /** Set aggressive dialogue choices */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SetAggressiveChoices();

  /** Set friendly dialogue choices */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SetFriendlyChoices();

  /** Hide the widget */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void HideChoices();

  /** Select a choice by index */
  UFUNCTION(BlueprintCallable, Category = "RFSN")
  void SelectChoice(int32 Index);

protected:
  virtual void NativeConstruct() override;

  /** Container for choice buttons - bind in Blueprint */
  UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
  TObjectPtr<UVerticalBox> ChoiceContainer;

  /** NPC name label - bind in Blueprint */
  UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
  TObjectPtr<UTextBlock> NpcNameLabel;

private:
  void CreateChoiceButtons();
  void ClearChoiceButtons();

  UPROPERTY()
  TArray<TObjectPtr<UButton>> ChoiceButtons;
};
