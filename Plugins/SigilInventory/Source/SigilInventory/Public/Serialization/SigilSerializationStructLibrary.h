// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameplayTagContainer.h"
#include "SigilCurrencyEntry.h"
#include "SigilMixinContainer.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif
#include "SigilSerializationStructLibrary.generated.h"

class USigilItemFragment;
class USigilItemCollectionDefinition;
class USigilItemDefinition;
class USigilItemCollection;
class USigilItemInstance;

/**
 * Record of an item instance for serialization.
 * 用于序列化的道具实例记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilItemRecord
{
	GENERATED_BODY()

	/**
	 * Unique identifier for the item instance.
	 * 道具实例的唯一标识符。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FGuid ItemId;

	/**
	 * Asset path to the item definition.
	 * 道具定义的资产路径。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FString DefinitionAssetPath;

	/**
	 * The serialized runtime state of each item fragment.
	 * 已经序列化的各item fragment 对应的运行时数据。
	 */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	// TArray<FSigilItemFragmentStateRecord> FragmentStateRecords;

	/**
	 * The serialized runtime state of each item fragment.
	 * 已经序列化的各item fragment 对应的运行时数据。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	TArray<FSigilMixinRecord> FragmentStateRecords;

	/**
	 * Binary data for the item instance.
	 * 道具实例的二进制数据。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	TArray<uint8> ByteData;

	/**
	 * Equality operator to compare item records.
	 * 比较道具记录的相等性运算符。
	 * @param Other The other item record to compare with. 要比较的其他道具记录。
	 * @return True if the item IDs are equal, false otherwise. 如果道具ID相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilItemRecord& Other) const;

	/**
	 * Checks if the item record is valid.
	 * 检查道具记录是否有效。
	 * @return True if the item ID and definition asset path are valid, false otherwise. 如果道具ID和定义资产路径有效则返回true，否则返回false。
	 */
	bool IsValid() const;
};


/**
 * Record for an item stack for serialization.
 * 用于序列化的道具堆栈记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilStackRecord
{
	GENERATED_BODY()

	/**
	 * Unique identifier for the stack.
	 * 堆栈的唯一标识符。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FGuid Id;

	/**
	 * Unique identifier for the associated item instance.
	 * 关联道具实例的唯一标识符。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FGuid ItemId;

	/**
	 * Unique identifier for the collection containing the stack.
	 * 包含堆栈的集合的唯一标识符。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FGuid CollectionId;

	/**
	 * Amount of items in the stack.
	 * 堆栈中的道具数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	int32 Amount{0};

	/**
	 * Equality operator to compare stack records.
	 * 比较堆栈记录的相等性运算符。
	 * @param Other The other stack record to compare with. 要比较的其他堆栈记录。
	 * @return True if the stack ID, item ID, and collection ID are equal, false otherwise. 如果堆栈ID、道具ID和集合ID相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilStackRecord& Other) const
	{
		return ItemId == Other.ItemId && Id == Other.Id && CollectionId == Other.CollectionId;
	}

	/**
	 * Checks if the stack record is valid.
	 * 检查堆栈记录是否有效。
	 * @return True if the stack ID, item ID, and collection ID are valid, false otherwise. 如果堆栈ID、道具ID和集合ID有效则返回true，否则返回false。
	 */
	bool IsValid() const;
};

/**
 * Record of a collection instance for serialization.
 * 用于序列化的道具集合记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilCollectionRecord
{
	GENERATED_BODY()

	/**
	 * Gameplay tag identifying the collection.
	 * 标识集合的游戏标签。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	FGameplayTag Tag;

	/**
	 * Unique identifier for the collection.
	 * 集合的唯一标识符。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	FGuid Id;

	/**
	 * Asset path to the collection definition.
	 * 集合定义的资产路径。
	 */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category="GIS")
	FString DefinitionAssetPath;

	/**
	 * Array of stack records within the collection.
	 * 集合中的堆栈记录数组。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	TArray<FSigilStackRecord> StackRecords;

	/**
	 * Checks if the collection record is valid.
	 * 检查集合记录是否有效。
	 * @return True if the collection ID and definition asset path are valid, false otherwise. 如果集合ID和定义资产路径有效则返回true，否则返回false。
	 */
	bool IsValid() const;
};

/**
 * Record of an inventory for serialization.
 * 用于序列化的库存记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilInventoryRecord
{
	GENERATED_BODY()

	/**
	 * Array of collection records within the inventory.
	 * 库存中的集合记录数组。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	TArray<FSigilCollectionRecord> CollectionRecords;

	/**
	 * Array of item records within the inventory.
	 * 库存中的道具记录数组。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	TArray<FSigilItemRecord> ItemRecords;

	/**
	 * Checks if the inventory record is valid.
	 * 检查库存记录是否有效。
	 * @return True if the inventory contains at least one collection record, false otherwise. 如果库存包含至少一个集合记录则返回true，否则返回false。
	 */
	bool IsValid() const
	{
		return CollectionRecords.Num() > 0;
	}
};

/**
 * Record of a currency system for serialization.
 * 用于序列化的货币系统记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilCurrencyRecord
{
	GENERATED_BODY()

	/**
	 * Default constructor for the currency record.
	 * 货币记录的默认构造函数。
	 */
	FSigilCurrencyRecord();

	/**
	 * Unique key for the currency record.
	 * 货币记录的唯一键。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	FName Key;

	/**
	 * Array of currency entries in the record.
	 * 记录中的货币条目数组。
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="GIS")
	TArray<FSigilCurrencyEntry> Currencies;
};
