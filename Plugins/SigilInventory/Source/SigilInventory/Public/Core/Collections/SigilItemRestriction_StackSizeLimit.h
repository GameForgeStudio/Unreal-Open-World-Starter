// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemRestriction.h"
#include "SigilItemRestriction_StackSizeLimit.generated.h"

/**
 * Restricts the maximum amount of an item within a specified collection.
 * 限制指定集合中道具的最大数量。
 * @attention Not suitable for MultiStackItemCollection.
 * @注意 不适用于MultiStackItemCollection。
 */
UCLASS(NotBlueprintable, DisplayName=ItemRestriction_StackSizeLimit)
class SIGILINVENTORY_API USigilItemRestriction_StackSizeLimit final : public USigilItemRestriction
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the stack size limit restriction.
	 * 栈大小限制的构造函数。
	 */
	USigilItemRestriction_StackSizeLimit();

protected:
	/**
	 * Internal check for adding an item to the collection.
	 * 检查道具是否可以添加到集合的内部函数。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @param ReceivingCollection The collection to add the item to. 要添加到的集合。
	 * @return True if the item can be added within the stack size limit, false otherwise. 如果道具可以在栈大小限制内添加则返回true，否则返回false。
	 */
	virtual bool CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const override;

	/**
	 * Gets the stack size limit for a specific item.
	 * 获取指定道具的栈大小限制。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @return The stack size limit for the item. 道具的栈大小限制。
	 */
	int32 GetStackSizeLimit(const USigilItemInstance* Item) const;

	/**
	 * The default stack size limit for any items.
	 * 针对所有道具的默认栈大小限制。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ItemRestriction)
	int32 DefaultStackSizeLimit;

	/**
	 * The integer attribute in the item definition used to limit the stack size for non-unique items (unique items cannot stack).
	 * 道具定义中用于限制非唯一道具栈大小的整型属性（唯一道具无法堆叠）。
	 * @attention This is optional.
	 * @注意 这是可选的。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ItemRestriction)
	FGameplayTag StackSizeLimitAttributeTag;
};