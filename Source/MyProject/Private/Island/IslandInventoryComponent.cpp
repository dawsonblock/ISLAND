#include "Island/IslandInventoryComponent.h"

UIslandInventoryComponent::UIslandInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIslandInventoryComponent::AddItem(EIslandItemType ItemType, int32 Count)
{
	if (ItemType == EIslandItemType::None || Count <= 0)
	{
		return;
	}

	Items.FindOrAdd(ItemType) += Count;
}

bool UIslandInventoryComponent::HasItem(EIslandItemType ItemType, int32 Count) const
{
	return GetItemCount(ItemType) >= Count;
}

bool UIslandInventoryComponent::ConsumeItem(EIslandItemType ItemType, int32 Count)
{
	if (!HasItem(ItemType, Count) || Count <= 0)
	{
		return false;
	}

	int32& ItemCount = Items.FindOrAdd(ItemType);
	ItemCount -= Count;
	if (ItemCount <= 0)
	{
		Items.Remove(ItemType);
	}

	return true;
}

int32 UIslandInventoryComponent::GetItemCount(EIslandItemType ItemType) const
{
	if (const int32* FoundCount = Items.Find(ItemType))
	{
		return *FoundCount;
	}

	return 0;
}

bool UIslandInventoryComponent::HasRequiredTowerParts() const
{
	return HasItem(EIslandItemType::TowerFuse) && HasItem(EIslandItemType::TowerFuel) &&
	       HasItem(EIslandItemType::AntennaCrank);
}
