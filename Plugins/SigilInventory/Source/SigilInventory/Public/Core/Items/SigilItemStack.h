// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SigilItemStack.generated.h"

class UActorComponent;
class USigilItemCollection;
class USigilItemSlotCollection;
class USigilItemMultiStackCollection;
class USigilItemDefinition;
class USigilItemInstance;
class USigilInventorySystemComponent;

/**
 * Enum defining the types of changes to an item stack.
 * 定义道具堆栈变更类型的枚举。
 */
UENUM(BlueprintType)
enum class ESigilItemStackChangeType : uint8
{
	WasAdded, // Item stack was added. 道具堆栈被添加。
	WasRemoved, // Item stack was removed. 道具堆栈被移除。
	Changed // Item stack was modified. 道具堆栈被修改。
};

/**
 * Structure used to store the corresponding item instance, their amount, and their position within the collection.
 * 道具堆栈是一个简单的结构体，用于存储对应的道具实例、数量以及在集合中的位置。
 */
USTRUCT()
struct SIGILINVENTORY_API FSigilItemStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	friend struct FSigilItemStackContainer;

	/**
	 * Static constant representing an invalid stack ID.
	 * 表示无效堆栈ID的静态常量。
	 */
	static FGuid InvalidId;

	/**
	 * Default constructor for item stack.
	 * 道具堆栈的默认构造函数。
	 */
	FSigilItemStack();

	/**
	 * Gets a debug string representation of the item stack.
	 * 获取道具堆栈的调试字符串表示。
	 * @return The debug string. 调试字符串。
	 */
	FString GetDebugString();

	/**
	 * Initializes the item stack with specified parameters.
	 * 使用指定参数初始化道具堆栈。
	 * @param InStackId The stack ID. 堆栈ID。
	 * @param InItem The item instance. 道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 * @param InCollection The collection the stack belongs to. 堆栈所属的集合。
	 */
	void Initialize(FGuid InStackId, USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection, int32 InIndex = INDEX_NONE);

	/**
	 * Checks if the item stack is valid.
	 * 检查道具堆栈是否有效。
	 * @return True if the stack is valid, false otherwise. 如果堆栈有效则返回true，否则返回false。
	 */
	bool IsValidStack() const;

	/**
	 * Resets the item stack to its default state.
	 * 将道具堆栈重置为默认状态。
	 */
	void Reset();

	/**
	 * The item instance of this stack.
	 * 此堆栈中的道具实例。
	 */
	UPROPERTY(VisibleInstanceOnly, Category="GIS", meta=(DisplayName="ItemInstance", ShowInnerProperties, DisplayPriority=0))
	TObjectPtr<USigilItemInstance> Item{nullptr};

	/**
	 * The quantity of the item in this stack.
	 * 此堆栈中的道具数量。
	 */
	UPROPERTY(VisibleAnywhere, Category="GIS")
	int32 Amount = 0;

	/**
	 * The stack ID.
	 * 堆栈ID。
	 */
	UPROPERTY(VisibleAnywhere, Category="GIS")
	FGuid Id;

	/**
	 * The collection this stack belongs to.
	 * 此堆栈所属的集合。
	 * @attention TODO: Should this really need to be replicated?
	 * @注意 TODO：此属性是否真的需要复制？
	 */
	UPROPERTY(VisibleAnywhere, Category="GIS")
	TObjectPtr<USigilItemCollection> Collection = nullptr;

	/**
	 * The collection ID (commented out).
	 * 集合ID（已注释）。
	 */
	// UPROPERTY(VisibleAnywhere, Category="GIS")
	// FGuid CollectionId;

	/**
	 * The last observed count of the stack (not replicated).
	 * 堆栈的最后观察计数（不复制）。
	 */
	UPROPERTY(NotReplicated)
	int32 LastObservedAmount = INDEX_NONE;

	/**
	 * The position of the stack within the collection.
	 * 堆栈在集合中的位置。
	 */
	UPROPERTY(VisibleAnywhere, Category="GIS")
	int32 Index = INDEX_NONE;

	/**
	 * Compares this item stack with another for equality.
	 * 将此道具堆栈与另一个比较以判断是否相等。
	 * @param Other The other item stack to compare with. 要比较的其他道具堆栈。
	 * @return True if the stacks are equal, false otherwise. 如果堆栈相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilItemStack& Other) const;

	/**
	 * Compares this item stack with another for inequality.
	 * 将此道具堆栈与另一个比较以判断是否不相等。
	 * @param Other The other item stack to compare with. 要比较的其他道具堆栈。
	 * @return True if the stacks are not equal, false otherwise. 如果堆栈不相等则返回true，否则返回false。
	 */
	bool operator!=(const FSigilItemStack& Other) const;

	/**
	 * Compares this item stack's ID with another ID for equality.
	 * 将此道具堆栈的ID与另一个ID比较以判断是否相等。
	 * @param OtherId The other ID to compare with. 要比较的其他ID。
	 * @return True if the IDs are equal, false otherwise. 如果ID相等则返回true，否则返回false。
	 */
	bool operator==(const FGuid& OtherId) const;

	/**
	 * Compares this item stack's ID with another ID for inequality.
	 * 将此道具堆栈的ID与另一个ID比较以判断是否不相等。
	 * @param OtherId The other ID to compare with. 要比较的其他ID。
	 * @return True if the IDs are not equal, false otherwise. 如果ID不相等则返回true，否则返回false。
	 */
	bool operator!=(const FGuid& OtherId) const;
};

/**
 * The container for item stacks, supporting fast array serialization.
 * 道具堆栈的容器，支持快速数组序列化。
 */
USTRUCT()
struct SIGILINVENTORY_API FSigilItemStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	friend USigilItemCollection;
	friend USigilItemMultiStackCollection;
	friend USigilItemSlotCollection;

	/**
	 * Default constructor for item stack container.
	 * 道具堆栈容器的默认构造函数。
	 */
	FSigilItemStackContainer() : OwningCollection(nullptr)
	{
	}

	/**
	 * Constructor for item stack container with a specified collection.
	 * 使用指定集合构造道具堆栈容器。
	 * @param InCollection The owning collection. 所属集合。
	 */
	FSigilItemStackContainer(USigilItemCollection* InCollection) : OwningCollection(InCollection)
	{
	}

	//~FFastArraySerializer contract
	/**
	 * Called before items are removed during replication.
	 * 复制期间在移除道具前调用。
	 * @param RemovedIndices The indices of removed items. 移除道具的索引。
	 * @param FinalSize The final size of the array after removal. 移除后数组的最终大小。
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * Called after items are added during replication.
	 * 复制期间在添加道具后调用。
	 * @param AddedIndices The indices of added items. 添加道具的索引。
	 * @param FinalSize The final size of the array after addition. 添加后数组的最终大小。
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * Called after items are changed during replication.
	 * 复制期间在道具更改后调用。
	 * @param ChangedIndices The indices of changed items. 更改道具的索引。
	 * @param FinalSize The final size of the array after changes. 更改后数组的最终大小。
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	/**
	 * Handles delta serialization for the item stack container.
	 * 处理道具堆栈容器的增量序列化。
	 * @param DeltaParms The serialization parameters. 序列化参数。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FSigilItemStack, FSigilItemStackContainer>(Stacks, DeltaParms, *this);
	}

	const FSigilItemStack* FindById(const FGuid& StackId) const;

	const FSigilItemStack* FindByItemId(const FGuid& ItemId) const;

	int32 IndexOfById(const FGuid& StackId) const;

	int32 IndexOfByItemId(const FGuid& ItemId) const;

	int32 IndexOfByIds(const FGuid& StackId, const FGuid& ItemId) const;

protected:
	/**
	 * Replicated list of item stacks.
	 * 复制的道具堆栈列表。
	 */
	UPROPERTY(VisibleAnywhere, Category="GIS", meta=(ShowOnlyInnerProperties, DisplayName="Item Stacks"))
	TArray<FSigilItemStack> Stacks;

	/**
	 * The collection that owns this container (not replicated).
	 * 拥有此容器的集合（不复制）。
	 */
	UPROPERTY(NotReplicated)
	TObjectPtr<USigilItemCollection> OwningCollection;
};

/**
 * Template specialization to enable network delta serialization for the item stack container.
 * 为道具堆栈容器启用网络增量序列化的模板特化。
 */
template <>
struct TStructOpsTypeTraits<FSigilItemStackContainer> : TStructOpsTypeTraitsBase2<FSigilItemStackContainer>
{
	enum { WithNetDeltaSerializer = true };
};
