// RFSN Dynamic Pricing Implementation

#include "RfsnDynamicPricing.h"
#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"
#include "Engine/GameInstance.h"

URfsnDynamicPricing::URfsnDynamicPricing()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnDynamicPricing::BeginPlay()
{
	Super::BeginPlay();
	RFSN_LOG(TEXT("DynamicPricing initialized for %s with %d items"), *GetOwner()->GetName(), Inventory.Num());
}

float URfsnDynamicPricing::GetPrice(const FString& ItemId) const
{
	const FRfsnItemPrice* Item = FindItem(ItemId);
	if (!Item)
	{
		return 0.0f;
	}

	float FinalPrice = Item->BasePrice;

	// Apply reputation modifier
	FinalPrice *= GetReputationModifier();

	// Apply stock modifier
	FinalPrice *= GetStockModifier(ItemId);

	// Apply temporary modifiers
	FinalPrice *= CalculateModifierMultiplier(ItemId, Item->Category);

	return FMath::RoundToFloat(FinalPrice);
}

float URfsnDynamicPricing::GetBuybackPrice(const FString& ItemId) const
{
	const FRfsnItemPrice* Item = FindItem(ItemId);
	if (!Item)
	{
		return 0.0f;
	}

	float BaseValue = Item->BasePrice * BuybackPercentage;

	// Better prices with good reputation
	float RepMod = GetReputationModifier();
	if (RepMod < 1.0f)
	{
		// Good reputation - pay more for player's items
		BaseValue *= (1.0f + (1.0f - RepMod) * 0.5f);
	}
	else
	{
		// Bad reputation - pay less
		BaseValue *= (1.0f / RepMod);
	}

	// Low stock = merchant pays more
	float StockMod = GetStockModifier(ItemId);
	if (StockMod > 1.0f)
	{
		BaseValue *= FMath::Sqrt(StockMod); // Partial benefit
	}

	return FMath::RoundToFloat(BaseValue);
}

float URfsnDynamicPricing::GetReputationModifier() const
{
	URfsnFactionSystem* FactionSys = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			FactionSys = GI->GetSubsystem<URfsnFactionSystem>();
		}
	}

	if (!FactionSys)
	{
		return 1.0f;
	}

	float Reputation = FactionSys->GetReputation(MerchantFaction);

	if (Reputation > 0)
	{
		// Positive reputation = discount
		float Discount = (Reputation / 10.0f) * ReputationDiscountRate;
		Discount = FMath::Min(Discount, MaxReputationDiscount);
		return 1.0f - Discount;
	}
	else if (Reputation < 0)
	{
		// Negative reputation = price increase
		float Penalty = (FMath::Abs(Reputation) / 10.0f) * ReputationDiscountRate;
		Penalty = FMath::Min(Penalty, MaxReputationPenalty);
		return 1.0f + Penalty;
	}

	return 1.0f;
}

float URfsnDynamicPricing::GetStockModifier(const FString& ItemId) const
{
	const FRfsnItemPrice* Item = FindItem(ItemId);
	if (!Item)
	{
		return 1.0f;
	}

	if (Item->CurrentStock <= Item->LowStockThreshold && Item->CurrentStock > 0)
	{
		// Low stock - increase price
		float Scarcity = 1.0f - (float)Item->CurrentStock / (float)Item->LowStockThreshold;
		return 1.0f + Scarcity * (LowStockPriceIncrease - 1.0f);
	}
	else if (Item->CurrentStock == 0)
	{
		// Out of stock
		return LowStockPriceIncrease * 1.5f;
	}
	else if (Item->CurrentStock > Item->MaxStock * 0.8f)
	{
		// Overstocked - slight discount
		return 0.9f;
	}

	return 1.0f;
}

float URfsnDynamicPricing::BuyItem(const FString& ItemId, int32 Quantity)
{
	FRfsnItemPrice* Item = FindItem(ItemId);
	if (!Item || Item->CurrentStock < Quantity)
	{
		return 0.0f;
	}

	float TotalPrice = GetPrice(ItemId) * Quantity;
	Item->CurrentStock -= Quantity;

	OnStockChanged.Broadcast(ItemId, Item->CurrentStock);
	OnPriceChanged.Broadcast(ItemId, GetPrice(ItemId));

	RFSN_LOG(TEXT("Bought %d x %s for %.0f"), Quantity, *ItemId, TotalPrice);
	return TotalPrice;
}

float URfsnDynamicPricing::SellItem(const FString& ItemId, int32 Quantity)
{
	FRfsnItemPrice* Item = FindItem(ItemId);

	float Value = GetBuybackPrice(ItemId) * Quantity;

	if (Item)
	{
		Item->CurrentStock = FMath::Min(Item->CurrentStock + Quantity, Item->MaxStock * 2);
		OnStockChanged.Broadcast(ItemId, Item->CurrentStock);
	}

	RFSN_LOG(TEXT("Sold %d x %s for %.0f"), Quantity, *ItemId, Value);
	return Value;
}

void URfsnDynamicPricing::AddPriceModifier(const FString& Name, float Multiplier, const FString& Category,
                                           const FString& ItemId, float DurationHours)
{
	// Remove existing modifier with same name
	RemovePriceModifier(Name);

	FRfsnPriceModifier Modifier;
	Modifier.Name = Name;
	Modifier.Multiplier = Multiplier;
	Modifier.AffectedCategory = Category;
	Modifier.AffectedItemId = ItemId;
	Modifier.Duration = DurationHours;
	Modifier.TimeRemaining = DurationHours;

	ActiveModifiers.Add(Modifier);
	RFSN_LOG(TEXT("Added price modifier: %s (x%.2f)"), *Name, Multiplier);
}

void URfsnDynamicPricing::RemovePriceModifier(const FString& Name)
{
	ActiveModifiers.RemoveAll([&Name](const FRfsnPriceModifier& Mod) { return Mod.Name == Name; });
}

void URfsnDynamicPricing::RestockAll()
{
	for (FRfsnItemPrice& Item : Inventory)
	{
		Item.CurrentStock = Item.MaxStock;
		OnStockChanged.Broadcast(Item.ItemId, Item.CurrentStock);
	}
	RFSN_LOG(TEXT("Restocked all items"));
}

TArray<FRfsnItemPrice> URfsnDynamicPricing::GetItemsInCategory(const FString& Category) const
{
	TArray<FRfsnItemPrice> Result;
	for (const FRfsnItemPrice& Item : Inventory)
	{
		if (Item.Category.Equals(Category, ESearchCase::IgnoreCase))
		{
			Result.Add(Item);
		}
	}
	return Result;
}

FRfsnItemPrice URfsnDynamicPricing::GetItem(const FString& ItemId) const
{
	const FRfsnItemPrice* Item = FindItem(ItemId);
	return Item ? *Item : FRfsnItemPrice();
}

bool URfsnDynamicPricing::HasStock(const FString& ItemId, int32 Quantity) const
{
	const FRfsnItemPrice* Item = FindItem(ItemId);
	return Item && Item->CurrentStock >= Quantity;
}

FString URfsnDynamicPricing::GetPricingContext() const
{
	FString Context;

	float RepMod = GetReputationModifier();
	if (RepMod < 0.85f)
	{
		Context = TEXT("You're a valued customer, I'll give you my best prices. ");
	}
	else if (RepMod < 0.95f)
	{
		Context = TEXT("I can offer you a small discount. ");
	}
	else if (RepMod > 1.2f)
	{
		Context = TEXT("Hmm, I don't know you well. Prices are standard. ");
	}
	else if (RepMod > 1.4f)
	{
		Context = TEXT("Your reputation precedes you. Expect premium prices. ");
	}

	// Check for low stock items
	int32 LowStockCount = 0;
	for (const FRfsnItemPrice& Item : Inventory)
	{
		if (Item.CurrentStock <= Item.LowStockThreshold)
		{
			LowStockCount++;
		}
	}

	if (LowStockCount > Inventory.Num() / 3)
	{
		Context += TEXT("Supplies are running low, some items are scarce. ");
	}

	// Check for active events
	for (const FRfsnPriceModifier& Mod : ActiveModifiers)
	{
		if (Mod.Multiplier > 1.1f)
		{
			Context += FString::Printf(TEXT("Due to %s, some prices are higher. "), *Mod.Name);
			break;
		}
		if (Mod.Multiplier < 0.9f)
		{
			Context += FString::Printf(TEXT("Special deal: %s! "), *Mod.Name);
			break;
		}
	}

	return Context;
}

void URfsnDynamicPricing::TickModifiers(float GameHoursElapsed)
{
	for (int32 i = ActiveModifiers.Num() - 1; i >= 0; --i)
	{
		FRfsnPriceModifier& Mod = ActiveModifiers[i];
		if (Mod.Duration > 0)
		{
			Mod.TimeRemaining -= GameHoursElapsed;
			if (Mod.TimeRemaining <= 0)
			{
				RFSN_LOG(TEXT("Price modifier expired: %s"), *Mod.Name);
				ActiveModifiers.RemoveAt(i);
			}
		}
	}
}

FRfsnItemPrice* URfsnDynamicPricing::FindItem(const FString& ItemId)
{
	return Inventory.FindByPredicate([&ItemId](const FRfsnItemPrice& Item)
	                                 { return Item.ItemId.Equals(ItemId, ESearchCase::IgnoreCase); });
}

const FRfsnItemPrice* URfsnDynamicPricing::FindItem(const FString& ItemId) const
{
	return Inventory.FindByPredicate([&ItemId](const FRfsnItemPrice& Item)
	                                 { return Item.ItemId.Equals(ItemId, ESearchCase::IgnoreCase); });
}

float URfsnDynamicPricing::CalculateModifierMultiplier(const FString& ItemId, const FString& Category) const
{
	float Multiplier = 1.0f;

	for (const FRfsnPriceModifier& Mod : ActiveModifiers)
	{
		// Check if modifier applies
		bool bApplies = false;

		if (!Mod.AffectedItemId.IsEmpty() && Mod.AffectedItemId.Equals(ItemId, ESearchCase::IgnoreCase))
		{
			bApplies = true;
		}
		else if (!Mod.AffectedCategory.IsEmpty() && Mod.AffectedCategory.Equals(Category, ESearchCase::IgnoreCase))
		{
			bApplies = true;
		}
		else if (Mod.AffectedItemId.IsEmpty() && Mod.AffectedCategory.IsEmpty())
		{
			bApplies = true; // Applies to all
		}

		if (bApplies)
		{
			Multiplier *= Mod.Multiplier;
		}
	}

	return Multiplier;
}
