// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilDropperComponent.h"
#include "Items/SigilItemInfo.h"
#include "SigilItemDropperComponent.generated.h"


class USigilInventorySystemComponent;

UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilItemDropperComponent : public USigilDropperComponent
{
	GENERATED_BODY()

public:
	/**
 	 * Get the item infos of this dropper will drops.
 	 * 获取要掉落的道具信息。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|Dropper")
	TArray<FSigilItemInfo> GetItemsToDrop() const;

	virtual void Drop() override;

protected:
	virtual void BeginPlay() override;

	virtual TArray<FSigilItemInfo> GetItemsToDropInternal() const;
	virtual void DropItemsInternal(const TArray<FSigilItemInfo>& ItemInfos);

	virtual void DropInventoryPickup(const TArray<FSigilItemInfo>& ItemInfos);
	virtual void DropItemPickup(const FSigilItemInfo& ItemInfo);

	/**
	 * Target collection to drop.
	 * 指定要掉落库存中的哪个集合里的道具.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(Categories="Sigil.Inventory.Collection"))
	FGameplayTag CollectionTag;

	/**
	 * If the drops is inventory pickup or item pickups?
	 * 指定以InventoryPickup的方式掉落还是以ItemPickup
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper")
	bool bDropAsInventory{true};
};
