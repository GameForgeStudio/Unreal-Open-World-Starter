// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilSerializationStructLibrary.h"
#include "UObject/Object.h"
#include "SigilInventoryFactory.generated.h"

class USigilItemDefinition;
class USigilInventorySystemComponent;
class USigilItemInstance;

/**
 * Inventory Factory class for managing low level operation in GIS.
 * 库存工厂类用于管理GIS中的底层操作。
 * @details Extend this class to have full control over how serialization works. 
 * @细节 拓展此类以完全控制序列化的运作。
 */
UCLASS(BlueprintType, Blueprintable)
class SIGILINVENTORY_API USigilInventoryFactory : public UObject
{
	GENERATED_BODY()

public:
	USigilInventoryFactory();

	/**
	 * Creates a new item instance for an actor.
	 * 为演员创建新的道具实例。
	 * @param Owner The actor that will own this item (used for network replication). 将拥有此道具的演员（用于网络复制）。
	 * @param ItemDefinition The definition of the item to create. 要创建的道具定义。
	 * @return The newly created item instance, or nullptr if creation failed. 新创建的道具实例，如果创建失败则返回nullptr。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	USigilItemInstance* CreateItem(AActor* Owner, const USigilItemDefinition* ItemDefinition);
	virtual USigilItemInstance* CreateItem_Implementation(AActor* Owner, const USigilItemDefinition* ItemDefinition);

	/**
	 * Duplicates an existing item to create a new one.
	 * 复制现有道具以创建新道具。
	 * @param Owner The actor that will own the new item. 将拥有新道具的演员。
	 * @param SrcItem The item to duplicate from. 要复制的原始道具。
	 * @param bGenerateNewId If true, will generate new id while copying. 如果为真，会在复制时，生成新的id
	 * @return The newly created item instance, or nullptr if duplication failed. 新创建的道具实例，如果复制失败则返回nullptr。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	USigilItemInstance* DuplicateItem(AActor* Owner, USigilItemInstance* SrcItem, bool bGenerateNewId = true);
	virtual USigilItemInstance* DuplicateItem_Implementation(AActor* Owner, USigilItemInstance* SrcItem, bool bGenerateNewId = true);

	// /**
	//  * Create and restore an item instance from item record.
	//  * 从item记录录中创建并恢复道具实例。
	//  * @param Owner The actor that will own the new item. 将拥有新道具的演员。
	//  * @param InRecord The record of item. 道具记录
	//  * @return The newly created item instance, or nullptr if restoring failed. 新创建的道具实例，如果恢复失败则返回nullptr。
	//  */


	/**
	 * Serializes an item instance into a record.
	 * 将道具实例序列化为记录。
	 * @param Item The item instance to serialize. 要序列化的道具实例。
	 * @param Record The resulting item record (output). 输出的道具记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	bool SerializeItem(USigilItemInstance* Item, FSigilItemRecord& Record);

	/**
	 * Deserializes an item from an item record.
	 * 从道具记录反序列化道具。
	 * @param Owner The actor owning the item instance (generally inventory system component's owner). 拥有此道具实例的演员，通常是库存系统组件的Owner。
	 * @param Record The item record to deserialize. 要反序列化的道具记录。
	 * @return The deserialized item instance, or nullptr if deserialization failed. 反序列化的道具实例，如果失败则返回nullptr。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	USigilItemInstance* DeserializeItem(AActor* Owner, const FSigilItemRecord& Record);

	/**
	 * Creates a new collection instance for an actor.
	 * 为演员创建新的集合实例。
	 * @param Owner The actor that will own this collection (used for network replication). 将拥有此集合的演员（用于网络复制）。
	 * @param Definition The definition of the collection to create. 要创建的集合定义。
	 * @return The newly created collection instance, or nullptr if creation failed. 新创建的集合实例，如果创建失败则返回nullptr。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	USigilItemCollection* CreateCollection(AActor* Owner, const USigilItemCollectionDefinition* Definition);

	/**
	 * Serializes an item collection into a record.
	 * 将道具集合序列化为记录。
	 * @param Collection The item collection to serialize. 要序列化的道具集合。
	 * @param Record The resulting collection record (output). 输出的集合记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	bool SerializeCollection(USigilItemCollection* Collection, FSigilCollectionRecord& Record);

	/**
	 * Deserializes a collection from a collection record.
	 * 从集合记录反序列化集合。
	 * @param InventorySystem The inventory system component to associate with the collection. 与集合关联的库存系统组件。
	 * @param Record The collection record to deserialize. 要反序列化的集合记录。
	 * @param ItemsMap A map of item IDs to item instances (output). 道具ID到道具实例的映射（输出）。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	void DeserializeCollection(USigilInventorySystemComponent* InventorySystem, const FSigilCollectionRecord& Record, TMap<FGuid, USigilItemInstance*>& ItemsMap);

	/**
	 * Serializes an entire inventory into a record.
	 * 将整个库存序列化为记录。
	 * @param InventorySystem The inventory system component to serialize. 要序列化的库存系统组件。
	 * @param Record The resulting inventory record. 输出的库存记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	bool SerializeInventory(USigilInventorySystemComponent* InventorySystem, FSigilInventoryRecord& Record);

	/**
	 * Deserializes an inventory from an inventory record.
	 * 从库存记录反序列化库存。
	 * @param InventorySystem The inventory system component to populate. 要填充的库存系统组件。
	 * @param InRecord The inventory record to deserialize. 要反序列化的库存记录。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|Factory")
	void DeserializeInventory(USigilInventorySystemComponent* InventorySystem, const FSigilInventoryRecord& InRecord);

protected:
	/**
	 * The default class used to construct item instances.
	 * 用于构造道具实例的默认类。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Factory", NoClear)
	TSoftClassPtr<USigilItemInstance> DefaultItemInstanceClass;

#if WITH_EDITOR
	/**
	 * Validates the data for this factory in the editor.
	 * 在编辑器中验证工厂的数据。
	 * @param Context The data validation context. 数据验证上下文。
	 * @return The result of the data validation. 数据验证的结果。
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
