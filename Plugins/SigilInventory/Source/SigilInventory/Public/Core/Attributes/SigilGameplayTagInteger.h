// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SigilGameplayTagInteger.generated.h"

struct FSigilGameplayTagIntegerContainer;

/**
 * Interface for objects that own a gameplay tag integer container.
 * 拥有游戏标签整型容器对象的接口。
 */
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class SIGILINVENTORY_API USigilGameplayTagIntegerContainerOwner : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface class for handling updates to integer attributes in a gameplay tag container.
 * 处理游戏标签容器中整型属性更新的接口类。
 */
class SIGILINVENTORY_API ISigilGameplayTagIntegerContainerOwner
{
	GENERATED_BODY()

public:
	/**
	 * Called when an integer attribute associated with a gameplay tag is updated.
	 * 当与游戏标签关联的整型属性更新时调用。
	 * @param Tag The gameplay tag identifying the attribute. 标识属性的游戏标签。
	 * @param OldValue The previous value of the attribute. 属性之前的值。
	 * @param NewValue The new value of the attribute. 属性的新值。
	 */
	virtual void OnTagIntegerUpdate(const FGameplayTag& Tag, int32 OldValue, int32 NewValue) = 0;
};

/**
 * Represents a gameplay tag and integer value pair.
 * 表示一个游戏标签和整型值的键值对。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilGameplayTagInteger : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/**
	 * Default constructor for the gameplay tag integer pair.
	 * 游戏标签整型对的默认构造函数。
	 */
	FSigilGameplayTagInteger()
	{
	}

	/**
	 * Constructor for the gameplay tag integer pair with initial values.
	 * 使用初始值构造游戏标签整型对。
	 * @param InTag The gameplay tag for the pair. 键值对的游戏标签。
	 * @param InValue The integer value for the pair. 键值对的整型值。
	 */
	FSigilGameplayTagInteger(FGameplayTag InTag, int32 InValue)
		: Tag(InTag)
		  , Value(InValue)
	{
	}

	/**
	 * Gets a debug string representation of the tag-value pair.
	 * 获取标签-值对的调试字符串表示。
	 * @return The debug string. 调试字符串。
	 */
	FString GetDebugString() const;

	friend FSigilGameplayTagIntegerContainer;

	/**
	 * The gameplay tag identifying the attribute.
	 * 标识属性的游戏标签。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FGameplayTag Tag;

	/**
	 * The integer value associated with the tag.
	 * 与标签关联的整型值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	int32 Value = 0;

	/**
	 * The previous integer value of the attribute (not replicated).
	 * 属性的前一个整型值（不复制）。
	 * @attention Likely a typo in the original code; should be int32 instead of float.
	 * @注意 原始代码中可能有误，应为int32而非float。
	 */
	UPROPERTY(NotReplicated)
	float PrevValue = 0;
};

/**
 * Container for storing gameplay tag integer pairs.
 * 存储游戏标签整型对的容器。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilGameplayTagIntegerContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	/**
	 * Default constructor for the integer container.
	 * 整型容器的默认构造函数。
	 */
	FSigilGameplayTagIntegerContainer()
	//	: Owner(nullptr)
	{
	}

	/**
	 * Constructor for the integer container with an owning object.
	 * 使用拥有对象构造整型容器。
	 * @param InObject The object that owns this container. 拥有此容器的对象。
	 */
	FSigilGameplayTagIntegerContainer(UObject* InObject)
		: ContainerOwner(InObject)
	{
	}

	/**
	 * Adds a new tag-value pair to the container.
	 * 向容器添加新的标签-值对。
	 * @param Tag The gameplay tag to add. 要添加的游戏标签。
	 * @param Value The integer value to associate with the tag. 与标签关联的整型值。
	 */
	void AddItem(FGameplayTag Tag, int32 Value);

	/**
	 * Sets the value for an existing tag or adds a new tag-value pair.
	 * 为现有标签设置值或添加新的标签-值对。
	 * @param Tag The gameplay tag to set. 要设置的游戏标签。
	 * @param Value The integer value to set. 要设置的整型值。
	 */
	void SetItem(FGameplayTag Tag, int32 Value);

	/**
	 * Removes a specified amount from a tag-value pair.
	 * 从标签-值对中移除指定数量。
	 * @param Tag The gameplay tag to remove value from. 要移除值的游戏标签。
	 * @param Value The amount to remove. 要移除的数量。
	 */
	void RemoveItem(FGameplayTag Tag, int32 Value);

	/**
	 * Gets the value associated with a specific tag.
	 * 获取与指定标签关联的值。
	 * @param Tag The gameplay tag to query. 要查询的游戏标签。
	 * @return The integer value associated with the tag. 与标签关联的整型值。
	 */
	int32 GetValue(FGameplayTag Tag) const
	{
		return TagToValueMap.FindRef(Tag);
	}

	/**
	 * Checks if the container contains a specific tag.
	 * 检查容器是否包含指定标签。
	 * @param Tag The gameplay tag to check. 要检查的游戏标签。
	 * @return True if the tag exists in the container, false otherwise. 如果标签存在于容器中则返回true，否则返回false。
	 */
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToValueMap.Contains(Tag);
	}

	//~FFastArraySerializer contract
	/**
	 * Called before items are removed during replication.
	 * 复制期间移除条目前调用。
	 * @param RemovedIndices The indices of items to remove. 要移除的条目索引。
	 * @param FinalSize The final size of the items array after removal. 移除后条目数组的最终大小。
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * Called after items are added during replication.
	 * 复制期间添加条目后调用。
	 * @param AddedIndices The indices of added items. 添加的条目索引。
	 * @param FinalSize The final size of the items array after addition. 添加后条目数组的最终大小。
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * Called after items are changed during replication.
	 * 复制期间条目更改后调用。
	 * @param ChangedIndices The indices of changed items. 更改的条目索引。
	 * @param FinalSize The final size of the items array after change. 更改后条目数组的最终大小。
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	/**
	 * Handles delta serialization for network replication.
	 * 处理网络复制的增量序列化。
	 * @param DeltaParms The serialization parameters. 序列化参数。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FSigilGameplayTagInteger, FSigilGameplayTagIntegerContainer>(Items, DeltaParms, *this);
	}

	/**
	 * The object that owns this container.
	 * 拥有此容器的对象。
	 */
	UPROPERTY(Transient)
	TObjectPtr<UObject> ContainerOwner{nullptr};

	/**
	 * Replicated list of gameplay tag integer pairs.
	 * 游戏标签整型对的复制列表。
	 */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category="GIS", meta=(TitleProperty="{Tag}->{Value}"))
	TArray<FSigilGameplayTagInteger> Items;

	/**
	 * Accelerated map of tags to values for efficient queries.
	 * 标签到值的加速映射，用于高效查询。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, NotReplicated, SaveGame, Category="GIS", meta=(ForceInlineRow))
	TMap<FGameplayTag, int32> TagToValueMap;
};

/**
 * Traits for the integer container to enable network delta serialization.
 * 整型容器的特性，用于启用网络增量序列化。
 */
template <>
struct TStructOpsTypeTraits<FSigilGameplayTagIntegerContainer> : TStructOpsTypeTraitsBase2<FSigilGameplayTagIntegerContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
