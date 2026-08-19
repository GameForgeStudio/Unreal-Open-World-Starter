// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemRestriction.h"
#include "SigilItemRestriction_StacksNumLimit.generated.h"

/**
 * Restricts the total number of stacks for an item collection.
 * 限制道具集合的总堆叠数量。
 */
UCLASS(NotBlueprintable, DisplayName=ItemRestriction_StacksNumLimit)
class SIGILINVENTORY_API USigilItemRestriction_StacksNumLimit final : public USigilItemRestriction
{
	GENERATED_BODY()

protected:
	/**
	 * Checks if an item can be added to the collection based on stack limits.
	 * 检查是否可以根据堆叠限制将道具添加到集合。
	 * @param ItemInfo Information about the item to add. 要添加的道具信息。
	 * @param ReceivingCollection The collection receiving the item. 接收道具的集合。
	 * @return True if the item can be added, false otherwise. 如果可以添加道具则返回true，否则返回false。
	 */
	virtual bool CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const override;

	/**
	 * Maximum number of stacks allowed in the collection.
	 * 集合中允许的最大堆叠数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ItemRestriction)
	int32 MaxStacksNum = 30;
};