// RFSN Dialogue Widget Component
// Displays NPC dialogue sentences as subtitles with typewriter effect

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnDialogueWidget.generated.h"

class UTextBlock;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueDisplayed, const FString&, NpcName, const FString&, Sentence);

/**
 * Component that manages dialogue display for RFSN NPCs.
 * Can render dialogue as 3D world widget or screen-space UI.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnDialogueWidget : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnDialogueWidget();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Duration to display each sentence (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Display")
	float SentenceDisplayDuration = 5.0f;

	/** Typewriter effect speed (characters per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Display")
	float TypewriterSpeed = 40.0f;

	/** Enable typewriter effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Display")
	bool bUseTypewriter = true;

	/** Offset from NPC location for world-space widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Display")
	FVector WorldWidgetOffset = FVector(0.0f, 0.0f, 150.0f);

	/** Maximum width of dialogue text widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Display")
	float MaxTextWidth = 400.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when dialogue sentence is displayed */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueDisplayed OnDialogueDisplayed;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Display a dialogue sentence */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void DisplaySentence(const FString& NpcName, const FString& Sentence);

	/** Clear current dialogue */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ClearDialogue();

	/** Bind to an RFSN NPC client component */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BindToRfsnClient(URfsnNpcClientComponent* RfsnClient);

	/** Get currently displayed text */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FString GetCurrentText() const { return CurrentDisplayText; }

	/** Check if dialogue is currently showing */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsShowingDialogue() const { return bIsShowingDialogue; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	FString CurrentNpcName;
	FString CurrentFullText;
	FString CurrentDisplayText;
	bool bIsShowingDialogue = false;
	float TypewriterProgress = 0.0f;
	FTimerHandle ClearTimer;

	TArray<FString> SentenceQueue;
	bool bIsProcessingQueue = false;

	UFUNCTION()
	void OnRfsnSentenceReceived(const FRfsnSentence& Sentence);

	void ProcessNextSentence();
	void UpdateTypewriter(float DeltaTime);
	void OnSentenceTimeout();
};
