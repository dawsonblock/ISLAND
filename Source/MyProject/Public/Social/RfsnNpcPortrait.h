// RFSN NPC Portrait Widget
// Displays NPC character card during dialogue with portrait, name, faction, and mood

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcPortrait.generated.h"

class URfsnNpcClientComponent;
class URfsnEmotionBlend;
class URfsnBackstoryGenerator;
class URfsnFactionSystem;
class UTexture2D;

/**
 * Portrait display data
 */
USTRUCT(BlueprintType)
struct FRfsnPortraitData
{
	GENERATED_BODY()

	/** NPC display name */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString NpcName;

	/** Faction name */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString FactionName;

	/** Current occupation */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString Occupation;

	/** Current mood string */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString Mood;

	/** Relationship tier (Hostile, Neutral, Friendly, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString RelationshipTier;

	/** Affinity value (-100 to 100) */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	float Affinity = 0.0f;

	/** Dominant emotion name */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString DominantEmotion;

	/** Portrait texture (if set) */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	UTexture2D* Portrait = nullptr;

	/** Portrait border color based on faction/mood */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FLinearColor BorderColor = FLinearColor::White;

	/** Short backstory context */
	UPROPERTY(BlueprintReadWrite, Category = "Portrait")
	FString ShortContext;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortraitUpdated, const FRfsnPortraitData&, PortraitData);

/**
 * NPC Portrait Component
 * Gathers and exposes NPC display data for UI widgets
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcPortrait : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcPortrait();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Portrait texture asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait|Config")
	UTexture2D* PortraitTexture;

	/** Faction ID for this NPC (used for color lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait|Config")
	FString FactionId = TEXT("survivors");

	/** Custom border color override (if not using faction color) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait|Config")
	FLinearColor CustomBorderColor = FLinearColor::White;

	/** Use faction color for border instead of custom */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait|Config")
	bool bUseFactionColor = true;

	/** Update portrait data every frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait|Config")
	bool bUpdateRealtime = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when portrait data changes */
	UPROPERTY(BlueprintAssignable, Category = "Portrait|Events")
	FOnPortraitUpdated OnPortraitUpdated;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get current portrait data */
	UFUNCTION(BlueprintPure, Category = "Portrait")
	FRfsnPortraitData GetPortraitData() const { return CachedData; }

	/** Force refresh portrait data */
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void RefreshPortraitData();

	/** Get faction color */
	UFUNCTION(BlueprintPure, Category = "Portrait")
	FLinearColor GetFactionColor(const FString& InFactionId) const;

	/** Get emotion color */
	UFUNCTION(BlueprintPure, Category = "Portrait")
	FLinearColor GetEmotionColor() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Cached portrait data */
	UPROPERTY()
	FRfsnPortraitData CachedData;

	/** Sibling components */
	UPROPERTY()
	URfsnNpcClientComponent* NpcClient;

	UPROPERTY()
	URfsnEmotionBlend* EmotionBlend;

	UPROPERTY()
	URfsnBackstoryGenerator* BackstoryGen;
};
