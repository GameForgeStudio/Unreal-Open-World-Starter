// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SigilItemStack.h"
#include "UObject/Object.h"
#include "SigilItemInfo.generated.h"

struct FSigilItemStack;
class USigilItemCollection;
class USigilItemInstance;

/**
 * Item information is a temporary structure used to pass item-related information across different operations, such as its belonging collection, amount, and corresponding item instance.
 * 道具信息是一个临时的结构体，用于在不同的操作中传递道具相关信息，例如其所属集合、数量及对应的道具实例等。
 */
USTRUCT(BlueprintType, DisplayName="GIS Item Information")
struct SIGILINVENTORY_API FSigilItemInfo
{
	GENERATED_BODY()

	/**
	 * Default constructor for item information.
	 * 道具信息的默认构造函数。
	 */
	FSigilItemInfo();

	/**
	 * Constructor for item information with item, amount, and collection.
	 * 使用道具、数量和集合构造道具信息。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 * @param InCollection The item collection where the item originates. 道具来源的集合。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection);

	/**
	 * Constructor for item information with item, amount, collection, and stack ID.
	 * 使用道具、数量、集合和堆栈ID构造道具信息。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 * @param InCollection The item collection where the item originates. 道具来源的集合。
	 * @param InStackId The stack ID associated with the item. 与道具关联的堆栈ID。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection, FGuid InStackId);

	/**
	 * Constructor for item information with item and amount.
	 * 使用道具和数量构造道具信息。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount);

	/**
	 * Constructor for item information with item, amount, and collection ID.
	 * 使用道具、数量和集合ID构造道具信息。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 * @param InCollectionId The collection ID associated with the item. 与道具关联的集合ID。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, FGuid InCollectionId);

	/**
	 * Constructor for item information with item, amount, and collection tag.
	 * 使用道具、数量和集合标签构造道具信息。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 * @param InCollectionTag The collection tag associated with the item. 与道具关联的集合标签。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, FGameplayTag InCollectionTag);

	/**
	 * Constructor for item information by copying another info and changing the amount.
	 * 通过复制其他道具信息并更改数量构造道具信息。
	 * @param InAmount The new item amount. 新的道具数量。
	 * @param OtherInfo The item info to copy. 要复制的道具信息。
	 */
	FSigilItemInfo(int32 InAmount, const FSigilItemInfo& OtherInfo);

	FSigilItemInfo(int32 InAmount, int32 InIndex, const FSigilItemInfo& OtherInfo);

	/**
	 * Constructor for item information by copying another info with a new item and amount.
	 * 通过复制其他道具信息并指定新道具和数量构造道具信息。
	 * @param InItem The new item instance. 新的道具实例。
	 * @param InAmount The new item amount. 新的道具数量。
	 * @param OtherInfo The item info to copy. 要复制的道具信息。
	 */
	FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, const FSigilItemInfo& OtherInfo);

	/**
	 * Constructor for item information from an item stack.
	 * 从道具堆栈构造道具信息。
	 * @param ItemStack The item stack to convert from. 要转换的道具堆栈。
	 */
	FSigilItemInfo(const FSigilItemStack& ItemStack);

	/**
	 * Gets a debug string representation of the item information.
	 * 获取道具信息的调试字符串表示。
	 * @return The debug string. 调试字符串。
	 */
	FString GetDebugString() const;

	/**
	 * Compares this item info with another for equality.
	 * 将此道具信息与另一个比较以判断是否相等。
	 * @param Other The other item info to compare with. 要比较的其他道具信息。
	 * @return True if the item infos are equal, false otherwise. 如果道具信息相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilItemInfo& Other) const;

	/**
	 * Checks if the item information is valid.
	 * 检查道具信息是否有效。
	 * @return True if the item info is valid, false otherwise. 如果道具信息有效则返回true，否则返回false。
	 */
	bool IsValid() const;

	/**
	 * Gets the inventory system component associated with the item collection.
	 * 获取与道具集合关联的库存系统组件。
	 * @return The inventory system component, or nullptr if not found. 库存系统组件，如果未找到则返回nullptr。
	 */
	USigilInventorySystemComponent* GetInventory() const;

	/**
	 * Static constant representing an invalid or empty item info.
	 * 表示无效或空道具信息的静态常量。
	 */
	static FSigilItemInfo None;

	/**
	 * The item instance associated with this information, with context-dependent usage.
	 * 与此信息关联的道具实例，用途因上下文而异。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS", meta=(DisplayName="Item Instance"))
	TObjectPtr<USigilItemInstance> Item;

	/**
	 * The amount of the item instance in this information.
	 * 此信息中道具实例的数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS", meta=(DisplayName="Item Amount"))
	int32 Amount;

	/**
	 * The collection instance where the information originates, with context-dependent usage.
	 * 此信息的源集合，用途因上下文而异。
	 * @attention For adding items, it specifies the source collection for overflow return; for retrieving items, it indicates the source collection.
	 * @注意 添加道具时，指定溢出返回的源集合；取回道具时，表示来源集合。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	TObjectPtr<USigilItemCollection> ItemCollection;

	/**
	 * The stack ID associated with this information, with context-dependent usage.
	 * 与此信息关联的堆栈ID，用途因上下文而异。
	 * @attention For adding/removing items, it determines the target stack; for retrieving items, it indicates the source stack.
	 * @注意 添加/移除道具时，用于确定目标堆栈；取回道具时，表示来源堆栈。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	FGuid StackId;

	/**
	 * The collection ID associated with this information, with context-dependent usage.
	 * 与此信息关联的集合ID，用途因上下文而异。
	 * @attention For adding/removing items, it determines the target collection; for retrieving items, it is usually null.
	 * @注意 添加/移除道具时，用于确定目标集合；取回道具时，通常为空。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	FGuid CollectionId;

	/**
	 * The collection tag associated with this information, with context-dependent usage.
	 * 与此信息关联的集合标签，用途因上下文而异。
	 * @attention For adding/removing items, it determines the target collection (lower priority than CollectionId); for retrieving items, it is usually null.
	 * @注意 添加/移除道具时，用于确定目标集合（优先级低于CollectionId）；取回道具时，通常为空。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS", meta=(Categories="Sigil.Inventory.Collection"))
	FGameplayTag CollectionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	int32 Index{INDEX_NONE};
};
