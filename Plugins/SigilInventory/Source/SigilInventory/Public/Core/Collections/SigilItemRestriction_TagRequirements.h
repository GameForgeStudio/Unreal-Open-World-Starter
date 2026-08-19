// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemRestriction.h"
#include "SigilItemRestriction_TagRequirements.generated.h"

/**
 * Restricts items to those that satisfy a specific tag query for addition to a collection.
 * 限制只有满足特定标签查询的道具可以添加到集合。
 */
UCLASS(NotBlueprintable, DisplayName=ItemRestriction_TagRequirements)
class SIGILINVENTORY_API USigilItemRestriction_TagRequirements final : public USigilItemRestriction
{
	GENERATED_BODY()

protected:
	/**
	 * Tag query that items must satisfy to be added to the collection.
	 * 道具必须满足的标签查询才能添加到集合。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ItemRestriction)
	FGameplayTagQuery TagQuery;

	/**
	 * Checks if an item satisfies the tag query for addition to the collection.
	 * 检查道具是否满足标签查询以添加到集合。
	 * @param ItemInfo Information about the item to add. 要添加的道具信息。
	 * @param ReceivingCollection The collection receiving the item. 接收道具的集合。
	 * @return True if the item can be added, false otherwise. 如果可以添加道具则返回true，否则返回false。
	 */
	virtual bool CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const override;
};