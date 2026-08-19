// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilSerializationStructLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SigilInventorySubsystem.generated.h"

class USigilInventorySystemComponent;
class UDataTable;
class USigilInventoryFactory;


/**
 * Subsystem for managing inventory-related operations, such as item creation.
 * 用于管理库存相关操作（如道具创建）的子系统。
 */
UCLASS(Config=Game, DefaultConfig)
class SIGILINVENTORY_API USigilInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Gets the inventory subsystem instance from a world context object.
	 * 从世界上下文对象获取库存子系统实例。
	 * @param WorldContextObject The object providing the world context. 提供世界上下文的对象。
	 * @return The inventory subsystem instance, or nullptr if not found. 库存子系统实例，如果未找到则返回nullptr。
	 */
	static USigilInventorySubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Initializes the subsystem.
	 * 初始化子系统。
	 * @param Collection The subsystem collection. 子系统集合。
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Deinitializes the subsystem.
	 * 反初始化子系统。
	 */
	virtual void Deinitialize() override;

	/**
	 * Creates an item instance from a definition.
	 * 从道具定义创建道具实例。
	 * @param Owner The actor that will own this item (required for network replication). 将拥有此道具的演员（网络复制所需）。
	 * @param ItemDefinition The item definition to create the instance from. 用于创建实例的道具定义。
	 * @return The newly created item instance, or nullptr if creation failed. 新创建的道具实例，如果创建失败则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	USigilItemInstance* CreateItem(AActor* Owner, TSoftObjectPtr<USigilItemDefinition> ItemDefinition);

	/**
	 * Creates an item instance from a definition (C++ version).
	 * 从道具定义创建道具实例（C++版本）。
	 * @param Owner The actor that will own this item. 将拥有此道具的演员。
	 * @param ItemDefinition The item definition to create the instance from. 用于创建实例的道具定义。
	 * @return The newly created item instance, or nullptr if creation failed. 新创建的道具实例，如果创建失败则返回nullptr。
	 */
	USigilItemInstance* CreateItem(AActor* Owner, const USigilItemDefinition* ItemDefinition);

	/**
	 * Creates a new item by duplicating an existing item.
	 * 通过复制现有道具创建新道具。
	 * @param Owner The actor that will own the duplicated item. 将拥有复制道具的演员。
	 * @param FromItem The original item to duplicate. 要复制的原始道具。
	 * @attention The duplicated item will have a new unique ID. 复制的道具将具有新的唯一ID。
	 * @return The duplicated item instance, or nullptr if duplication failed. 复制的道具实例，如果复制失败则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	USigilItemInstance* DuplicateItem(AActor* Owner, USigilItemInstance* FromItem, bool bGenerateNewId = true);

	/**
	 * Serializes an item instance into a record.
	 * 将道具实例序列化为记录。
	 * @param Item The item instance to serialize. 要序列化的道具实例。
	 * @param Record The resulting item record (output). 输出的道具记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	bool SerializeItem(USigilItemInstance* Item, FSigilItemRecord& Record);

	/**
	 * Deserializes an item from an item record.
	 * 从道具记录反序列化道具。
	 * @param Owner The actor owning the item instance (generally inventory system component's owner). 拥有此道具实例的演员，通常是库存系统组件的Owner。
	 * @param Record The item record to deserialize. 要反序列化的道具记录。
	 * @return The deserialized item instance, or nullptr if deserialization failed. 反序列化的道具实例，如果失败则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	USigilItemInstance* DeserializeItem(AActor* Owner, const FSigilItemRecord& Record);

	/**
	 * Serializes an item collection into a record.
	 * 将道具集合序列化为记录。
	 * @param ItemCollection The item collection to serialize. 要序列化的道具集合。
	 * @param Record The resulting collection record (output). 输出的集合记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	bool SerializeCollection(USigilItemCollection* ItemCollection, FSigilCollectionRecord& Record);

	/**
	 * Deserializes a collection from a collection record.
	 * 从集合记录反序列化集合。
	 * @param InventorySystem The inventory system component to associate with the collection. 与集合关联的库存系统组件。
	 * @param Record The collection record to deserialize. 要反序列化的集合记录。
	 * @param ItemsMap A map of item IDs to item instances (output). 道具ID到道具实例的映射（输出）。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	void DeserializeCollection(USigilInventorySystemComponent* InventorySystem, const FSigilCollectionRecord& Record, TMap<FGuid, USigilItemInstance*>& ItemsMap);

	/**
	 * Serializes an entire inventory into a record.
	 * 将整个库存序列化为记录。
	 * @param InventorySystem The inventory system component to serialize. 要序列化的库存系统组件。
	 * @param Record The resulting inventory record. 输出的库存记录。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	bool SerializeInventory(USigilInventorySystemComponent* InventorySystem, FSigilInventoryRecord& Record);

	/**
	 * Deserializes an inventory from an inventory record.
	 * 从库存记录反序列化库存。
	 * @param InventorySystem The inventory system component to populate. 要填充的库存系统组件。
	 * @param Record The inventory record to deserialize. 要反序列化的库存记录。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	void DeserializeInventory(USigilInventorySystemComponent* InventorySystem, const FSigilInventoryRecord& Record);

protected:
	/**
	 * Initializes the item factory for the subsystem.
	 * 初始化子系统的道具工厂。
	 */
	virtual void InitializeFactory();

	/**
	 * The item factory used to create item instances.
	 * 用于创建道具实例的道具工厂。
	 */
	UPROPERTY()
	TObjectPtr<USigilInventoryFactory> Factory;
};
