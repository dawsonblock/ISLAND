#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IslandInventoryComponent.generated.h"

UENUM(BlueprintType)
enum class EIslandItemType : uint8
{
	None,
	TowerFuse,
	TowerFuel,
	AntennaCrank,
	Medkit,
	Food
};

USTRUCT(BlueprintType)
struct FIslandInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EIslandItemType ItemType = EIslandItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Count = 0;
};

UCLASS(ClassGroup = (Island), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UIslandInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIslandInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(EIslandItemType ItemType, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(EIslandItemType ItemType, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeItem(EIslandItemType ItemType, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(EIslandItemType ItemType) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasRequiredTowerParts() const;

private:
	UPROPERTY()
	TMap<EIslandItemType, int32> Items;
};
