// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemRestriction.h"
#include "SigilItemRestriction_UniqueOnly.generated.h"

/**
 * Restricts a collection to only accept unique items.
 * 限制集合仅接受唯一道具。
 * @details Prevents duplicate items from being added to the collection.
 * @细节 防止重复道具被添加到集合。
 */
UCLASS(NotBlueprintable, DisplayName=ItemRestriction_UniqueOnly)
class SIGILINVENTORY_API USigilItemRestriction_UniqueOnly final : public USigilItemRestriction
{
	GENERATED_BODY()

protected:
	/**
	 * Checks if an item is unique before adding it to the collection.
	 * 在将道具添加到集合之前检查其是否唯一。
	 * @param ItemInfo Information about the item to add. 要添加的道具信息。
	 * @param ReceivingCollection The collection receiving the item. 接收道具的集合。
	 * @return True if the item is unique and can be added, false otherwise. 如果道具唯一且可添加则返回true，否则返回false。
	 */
	virtual bool CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const override;
};