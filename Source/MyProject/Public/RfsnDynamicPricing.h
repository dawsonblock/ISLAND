// RFSN Dynamic Pricing System
// Manages merchant prices based on relationships, supply/demand, and events
// Integrates with faction system for reputation-based pricing

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnDynamicPricing.generated.h"

class URfsnFactionSystem;
class URfsnNpcClientComponent;

/**
 * Single item pricing data
 */
USTRUCT(BlueprintType)
struct FRfsnItemPrice
{
	GENERATED_BODY()

	/** Item identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	FString ItemId;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	FString DisplayName;

	/** Base price before modifiers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	float BasePrice = 100.0f;

	/** Current stock (affects price) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	int32 CurrentStock = 10;

	/** Maximum stock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	int32 MaxStock = 20;

	/** Minimum stock before price increases */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	int32 LowStockThreshold = 3;

	/** Item category for group modifiers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	FString Category = TEXT("General");

	/** Is this a buy or sell price? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Price")
	bool bIsSellPrice = false;
};

/**
 * Temporary price modifier
 */
USTRUCT(BlueprintType)
struct FRfsnPriceModifier
{
	GENERATED_BODY()

	/** Modifier name */
	UPROPERTY(BlueprintReadWrite, Category = "Modifier")
	FString Name;

	/** Category to affect (empty = all) */
	UPROPERTY(BlueprintReadWrite, Category = "Modifier")
	FString AffectedCategory;

	/** Item to affect (empty = all in category) */
	UPROPERTY(BlueprintReadWrite, Category = "Modifier")
	FString AffectedItemId;

	/** Multiplicative modifier (1.0 = no change) */
	UPROPERTY(BlueprintReadWrite, Category = "Modifier")
	float Multiplier = 1.0f;

	/** Duration in game hours (-1 = permanent) */
	UPROPERTY(BlueprintReadWrite, Category = "Modifier")
	float Duration = -1.0f;

	/** Time remaining */
	float TimeRemaining = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPriceChanged, const FString&, ItemId, float, NewPrice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStockChanged, const FString&, ItemId, int32, NewStock);

/**
 * Dynamic Pricing Component
 * Manages merchant prices with reputation and supply/demand
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnDynamicPricing : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnDynamicPricing();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Merchant's faction (affects reputation pricing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	FString MerchantFaction = TEXT("merchants");

	/** Base reputation discount per 10 rep points (0.05 = 5% per 10 points) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	float ReputationDiscountRate = 0.05f;

	/** Maximum reputation discount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	float MaxReputationDiscount = 0.3f;

	/** Penalty for low reputation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	float MaxReputationPenalty = 0.5f;

	/** Low stock price increase factor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	float LowStockPriceIncrease = 1.5f;

	/** Buyback percentage (what merchant pays for items) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Config")
	float BuybackPercentage = 0.4f;

	/** Items in inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing|Inventory")
	TArray<FRfsnItemPrice> Inventory;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Active temporary modifiers */
	UPROPERTY(BlueprintReadOnly, Category = "Pricing|State")
	TArray<FRfsnPriceModifier> ActiveModifiers;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Pricing|Events")
	FOnPriceChanged OnPriceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Pricing|Events")
	FOnStockChanged OnStockChanged;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get final price for an item (includes all modifiers) */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	float GetPrice(const FString& ItemId) const;

	/** Get buyback price for an item */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	float GetBuybackPrice(const FString& ItemId) const;

	/** Get reputation modifier */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	float GetReputationModifier() const;

	/** Get stock modifier for item */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	float GetStockModifier(const FString& ItemId) const;

	/** Buy an item (reduce stock, return final price) */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	float BuyItem(const FString& ItemId, int32 Quantity = 1);

	/** Sell an item to merchant (increase stock, return buyback value) */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	float SellItem(const FString& ItemId, int32 Quantity = 1);

	/** Add a temporary price modifier */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	void AddPriceModifier(const FString& Name, float Multiplier, const FString& Category = TEXT(""),
	                      const FString& ItemId = TEXT(""), float DurationHours = -1.0f);

	/** Remove a price modifier by name */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	void RemovePriceModifier(const FString& Name);

	/** Restock all items */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	void RestockAll();

	/** Get all items in category */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	TArray<FRfsnItemPrice> GetItemsInCategory(const FString& Category) const;

	/** Get item by ID */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	FRfsnItemPrice GetItem(const FString& ItemId) const;

	/** Check if item is in stock */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	bool HasStock(const FString& ItemId, int32 Quantity = 1) const;

	/** Get price context for LLM (e.g., "Prices are high today") */
	UFUNCTION(BlueprintPure, Category = "Pricing")
	FString GetPricingContext() const;

	/** Tick modifiers (call with game hours elapsed) */
	UFUNCTION(BlueprintCallable, Category = "Pricing")
	void TickModifiers(float GameHoursElapsed);

protected:
	virtual void BeginPlay() override;

private:
	/** Find item in inventory */
	FRfsnItemPrice* FindItem(const FString& ItemId);
	const FRfsnItemPrice* FindItem(const FString& ItemId) const;

	/** Get all applicable modifiers for an item */
	float CalculateModifierMultiplier(const FString& ItemId, const FString& Category) const;
};
