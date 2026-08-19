// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SigilItemInfo.h"
#include "NativeGameplayTags.h"
#include "SigilInventoryMessages.generated.h"

class USigilInventorySystemComponent;
class USigilItemCollection;
class USigilItemInstance;

/**
 * Message struct for general inventory updates.
 * 通用库存更新的消息结构体。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryUpdateMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component associated with the update.
	 * 与更新关联的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;
};

/**
 * Message struct for item stack additions or removals in the inventory.
 * 库存中道具堆栈添加或移除的消息结构体。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryStackUpdateMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component associated with the stack update.
	 * 与堆栈更新关联的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The type of change to the item stack (e.g., added, removed, changed).
	 * 道具堆栈的变更类型（例如，添加、移除、变更）。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	ESigilItemStackChangeType ChangeType{ESigilItemStackChangeType::Changed};

	/**
	 * The changed item instance.
	 * 变更的道具实例。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilItemInstance> Instance = nullptr;

	/**
	 * The unique ID of the changed stack.
	 * 变更堆栈的唯一ID。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	FGuid StackId;

	/**
	 * The unique ID of the collection containing the changed stack.
	 * 包含变更堆栈的集合的唯一ID。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	FGuid CollectionId;

	/**
	 * The new amount of the item instance in the stack.
	 * 道具实例在堆栈中的当前数量。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	int32 NewCount = 0;

	/**
	 * The change in amount of the item instance in the stack (difference from previous amount).
	 * 道具实例在堆栈中的数量变化（与之前数量的差值）。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	int32 Delta = 0;
};

/**
 * Message struct for adding item info to the inventory.
 * 将道具信息添加到库存的消息结构体。
 * @details The ItemInfo is the source of the amount being added, and the ItemStack is the result.
 * @细节 ItemInfo是添加数量的来源，ItemStack是添加的结果。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryAddItemInfoMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component associated with the item addition.
	 * 与道具添加关联的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The item info being added.
	 * 正在添加的道具信息。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo ItemInfo;

	/**
	 * The unique ID of the stack where the item is added.
	 * 添加道具的堆栈的唯一ID。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FGuid StackId;

	/**
	 * The unique ID of the collection where the item is added.
	 * 添加道具的集合的唯一ID。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FGuid CollectionId;

	/**
	 * The gameplay tag of the collection where the item is added.
	 * 添加道具的集合的游戏标签。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FGameplayTag CollectionTag;
};

/**
 * Message struct for when item info addition is rejected by the inventory.
 * 道具信息添加被库存拒绝时的消息结构体。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryAddItemInfoRejectedMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component that rejected the item info.
	 * 拒绝道具信息的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The collection that rejected the item info.
	 * 拒绝道具信息的集合。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilItemCollection> Collection;

	/**
	 * The original item info that was attempted to be added.
	 * 尝试添加的原始道具信息。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo OriginalItemInfo;

	/**
	 * The portion of the item info that was successfully added.
	 * 成功添加的道具信息部分。
	 * @attention May be empty if no items were added. 如果没有添加任何道具，可能为空。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo ItemInfoAdded;

	/**
	 * The portion of the item info that was rejected due to reasons like overflow or collection restrictions.
	 * 由于溢出或集合限制等原因被拒绝的道具信息部分。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo RejectedItemInfo;

	/**
	 * The item info returned to the incoming collection if the "return overflow" option is enabled.
	 * 如果启用了“返回溢出”选项，则返回到输入集合的道具信息。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo ReturnedItemInfo;
};

/**
 * Message struct for removing item info from the inventory.
 * 从库存中移除道具信息的消息结构体。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryRemoveItemInfoMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component associated with the item removal.
	 * 与道具移除关联的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The item info being removed.
	 * 正在移除的道具信息。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	FSigilItemInfo ItemInfo;
};

/**
 * Message struct for collection updates in the inventory.
 * 库存中集合更新的消息结构体。
 */
USTRUCT(BlueprintType)
struct FSigilInventoryCollectionUpdateMessage
{
	GENERATED_BODY()

	/**
	 * The inventory system component associated with the collection update.
	 * 与集合更新关联的库存系统组件。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The collection that was updated.
	 * 已更新的集合。
	 */
	UPROPERTY(BlueprintReadOnly, Category="GIS")
	TObjectPtr<USigilItemCollection> Collection;
};