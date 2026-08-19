// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCollectionContainer.h"
#include "SigilCoreStructLibrary.h"
#include "SigilInventoryMessages.h"
#include "Items/SigilItemInfo.h"
#include "SigilSerializationStructLibrary.h"
#include "Components/ActorComponent.h"
#include "SigilInventorySystemComponent.generated.h"

class USigilCurrencySystemComponent;
class USigilItemCollectionDefinition;
class USigilItemCollection;


/**
 * Delegate triggered when the inventory system is initialized.
 * 库存系统初始化时触发的委托。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSigilInventory_InitializedSignature);

/**
 * Delegate triggered when an item stack in the inventory is updated.
 * 库存中的道具堆栈更新时触发的委托。
 * @param Message The update message containing stack details. 包含堆栈详细信息的更新消息。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilInventory_StackUpdateSignature, const FSigilInventoryStackUpdateMessage&, Message);

/**
 * Delegate triggered when an item is added to the inventory (server-side only).
 * 道具添加到库存时触发的委托（仅限服务器端）。
 * @param Message The message containing details of the added item. 包含添加道具详细信息的消息。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilInventory_AddItemInfoSignature, const FSigilInventoryAddItemInfoMessage&, Message);

/**
 * Delegate triggered when an item addition is rejected (server-side only).
 * 道具添加被拒绝时触发的委托（仅限服务器端）。
 * @param Message The message containing details of the rejected item. 包含拒绝道具详细信息的消息。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilInventory_AddItemInfoRejectedSignature, const FSigilInventoryAddItemInfoRejectedMessage&, Message);

/**
 * Delegate triggered when an item is removed from the inventory (server-side only).
 * 道具从库存移除时触发的委托（仅限服务器端）。
 * @param Message The message containing details of the removed item. 包含移除道具详细信息的消息。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilInventory_RemoveItemInfoSignature, const FSigilInventoryRemoveItemInfoMessage&, Message);

/**
 * Delegate triggered when a collection is added or removed from the inventory.
 * 集合添加或移除时触发的委托。
 * @param Collection The item collection involved. 涉及的道具集合。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilInventory_CollectionSignature, USigilItemCollection*, Collection);

/**
 * Dynamic delegate for inventory system initialization events.
 * 库存系统初始化事件的动态委托。
 */
UDELEGATE()
DECLARE_DYNAMIC_DELEGATE(FSigilInventorySystem_Initialized_DynamicEvent);

/**
 * Inventory system component for managing items and collections.
 * 管理道具和集合的库存系统组件。
 */
UCLASS(ClassGroup=(GIS), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilInventorySystemComponent : public UActorComponent
{
	GENERATED_BODY()

	friend FSigilItemStackContainer;
	friend FSigilCollectionContainer;

public:
	/**
	 * Sets default values for this component's properties.
	 * 为组件的属性设置默认值。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilInventorySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	/**
	 * Gets the properties that should be replicated for this object.
	 * 获取需要为此对象复制的属性。
	 * @param OutLifetimeProps Array to store the replicated properties. 存储复制属性的数组。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Replicates subobjects for this component.
	 * 为此组件复制子对象。
	 * @param Channel The actor channel. 演员通道。
	 * @param Bunch The replication data bunch. 复制数据束。
	 * @param RepFlags The replication flags. 复制标志。
	 * @return True if subobjects were replicated, false otherwise. 如果子对象被复制则返回true，否则返回false。
	 */
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	//~End of UObject interface

	//~UActorComponent interface
	/**
	 * Called when the component is registered.
	 * 组件注册时调用。
	 */
	virtual void OnRegister() override;

	/**
	 * Initializes the component.
	 * 初始化组件。
	 */
	virtual void InitializeComponent() override;

	/**
	 * Prepares the component for replication.
	 * 为组件的复制做准备。
	 */
	virtual void ReadyForReplication() override;

	/**
	 * Called when the game starts.
	 * 游戏开始时调用。
	 */
	virtual void BeginPlay() override;

	/**
	 * Updates the component each frame.
	 * 每帧更新组件。
	 * @param DeltaTime Time since the last tick. 上次tick以来的时间。
	 * @param TickType The type of tick. tick类型。
	 * @param ThisTickFunction The tick function. tick函数。
	 */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~End of UActorComponent interface

#pragma region Inventory
	/**
	 * Finds the inventory system component on the specified actor.
	 * 在指定演员上查找库存系统组件。
	 * @param Actor The actor to search for the component. 要查找组件的演员。
	 * @param Inventory The found inventory component (output). 找到的库存组件（输出）。
	 * @return True if the component was found, false otherwise. 如果找到组件则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem", Meta = (DefaultToSelf="Actor", ExpandBoolAsExecs = "ReturnValue"))
	static bool FindInventorySystemComponent(const AActor* Actor, USigilInventorySystemComponent*& Inventory);

	/**
	 * Gets the inventory system component from the specified actor.
	 * 从指定演员获取库存系统组件。
	 * @param Actor The actor to query. 要查询的演员。
	 * @return The inventory system component, or nullptr if not found. 库存系统组件，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem", Meta = (DefaultToSelf="Actor"))
	static USigilInventorySystemComponent* GetInventorySystemComponent(const AActor* Actor);

	/**
	 * Static helper to find the inventory system component on an actor.
	 * 在演员上查找库存系统组件的静态辅助函数。
	 * @param Actor The actor to query. 要查询的演员。
	 * @return The inventory system component, or nullptr if not found. 库存系统组件，如果未找到则返回nullptr。
	 */
	static USigilInventorySystemComponent* FindInventorySystemComponent(const AActor* Actor);

	/**
	 * Initializes the inventory system.
	 * 初始化库存系统。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual void InitializeInventorySystem();

	/**
	 * Initializes the inventory system with a specific record.
	 * 使用特定记录初始化库存系统。
	 * @param InventoryRecord The inventory record to initialize with. 初始化使用的库存记录。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual void InitializeInventorySystemWithRecord(const FSigilInventoryRecord& InventoryRecord);

	/**
	 * Resets the inventory system.
	 * 重置库存系统。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual void ResetInventorySystem();

	/**
	 * Checks if the inventory system is initialized.
	 * 检查库存系统是否已初始化。
	 * @return True if initialized, false otherwise. 如果已初始化则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	bool IsInventoryInitialized() const;

	/**
	 * Binds a delegate to be called when the inventory system is initialized.
	 * 绑定一个委托，在库存系统初始化时调用。
	 * @param Delegate The delegate to bind. 要绑定的委托。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	void BindToInventorySystemInitialized(FSigilInventorySystem_Initialized_DynamicEvent Delegate);

	/**
	 * Gets the associated currency system of this inventory.
	 * 获取库存关联的货币系统。
	 * @return The currency system component. 货币系统组件。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	USigilCurrencySystemComponent* GetCurrencySystem() const;

	/**
	 * Loads the default loadouts for the inventory.
	 * 为库存加载默认装备。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual void LoadDefaultLoadouts();

	/**
	 * Server-side function to load default loadouts.
	 * 服务器端函数，用于加载默认装备。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|InventorySystem")
	virtual void ServerLoadDefaultLoadouts();

protected:
	/**
	 * Called when the inventory system is initialized.
	 * 库存系统初始化时调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|InventorySystem")
	void OnInventorySystemInitialized();

	/**
	 * List of delegates for inventory system initialization.
	 * 库存系统初始化的委托列表。
	 */
	UPROPERTY()
	TArray<FSigilInventorySystem_Initialized_DynamicEvent> InitializedDelegates;

#pragma endregion

#pragma region Events

public:
	/**
	 * Event triggered when the inventory system is initialized.
	 * 库存系统初始化时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_InitializedSignature OnInventorySystemInitializedEvent;

	/**
	 * Event triggered when any item stack in collections changes (both server and client).
	 * 集合中的道具堆栈更改时触发的事件（服务器和客户端均触发）。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_StackUpdateSignature OnInventoryStackUpdate;

	/**
	 * Event triggered when a collection is added to the inventory.
	 * 集合添加到库存时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_CollectionSignature OnCollectionAddedEvent;

	/**
	 * Event triggered when a collection is removed from the inventory.
	 * 集合从库存移除时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_CollectionSignature OnCollectionRemovedEvent;

	/**
	 * Event triggered when an item is added to the inventory (server-side only).
	 * 道具添加到库存时触发的事件（仅限服务器端）。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_AddItemInfoSignature OnInventoryAddItemInfo;

	/**
	 * Event triggered when an item addition is rejected (server-side only).
	 * 道具添加被拒绝时触发的事件（仅限服务器端）。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_AddItemInfoRejectedSignature OnInventoryAddItemInfo_Rejected;

	/**
	 * Event triggered when an item is removed from the inventory (server-side only).
	 * 道具从库存移除时触发的事件（仅限服务器端）。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilInventory_RemoveItemInfoSignature OnInventoryRemoveItemInfo;

#pragma endregion

#pragma region Items
	/**
	 * Checks if an item can be added to the inventory.
	 * 检查道具是否可以添加到库存。
	 * @param InItemInfo The item info to check. 要检查的道具信息。
	 * @param OutItemInfo The resulting item info (output). 结果道具信息（输出）。
	 * @return True if the item can be added, false otherwise. 如果道具可以添加则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual bool CanAddItem(const FSigilItemInfo& InItemInfo, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Adds an item to the inventory.
	 * 将道具添加到库存。
	 * @param ItemInfo The item info specifying the item, collection, and stack details. 指定道具、集合和堆栈详细信息的道具信息。
	 * @return The number of items added, or 0 if none were added. 添加的道具数量，如果未添加则返回0。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem", meta=(AutoCreateRefTerm="ItemInfo"))
	virtual FSigilItemInfo AddItem(const FSigilItemInfo& ItemInfo);

	/**
	 * Adds a group of items to the inventory.
	 * 将一组道具添加到库存。
	 * @param ItemInfos The array of item infos to add. 要添加的道具信息数组。
	 * @return The array of actually added item infos. 实际添加的道具信息数组。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual TArray<FSigilItemInfo> AddItems(TArray<FSigilItemInfo> ItemInfos);

	/**
	 * Adds an item to a specific collection by its definition.
	 * 通过道具定义将道具添加到指定集合。
	 * @param CollectionTag The tag of the target collection. 目标集合的标签。
	 * @param ItemDefinition The item definition to add. 要添加的道具定义。
	 * @param NewAmount The amount of the item to add. 要添加的道具数量。
	 * @return The actually added item info. 实际添加的道具信息。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual FSigilItemInfo AddItemByDefinition(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                          const FGameplayTag CollectionTag, TSoftObjectPtr<USigilItemDefinition> ItemDefinition, const int32 NewAmount);

	/**
	 * Server-side function to add an item to a specific collection by its definition.
	 * 服务器端函数，通过道具定义将道具添加到指定集合。
	 * @param CollectionTag The tag of the target collection. 目标集合的标签。
	 * @param ItemDefinition The item definition to add. 要添加的道具定义。
	 * @param NewAmount The amount of the item to add. 要添加的道具数量。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|InventorySystem")
	void ServerAddItemByDefinition(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                               const FGameplayTag CollectionTag, const TSoftObjectPtr<USigilItemDefinition>& ItemDefinition, const int32 NewAmount);

	/**
	 * Checks if an item can be moved within the inventory.
	 * 检查道具是否可以在库存内移动。
	 * @param ItemInfo The item info specifying the source and target collection. 指定源集合和目标集合的道具信息。
	 * @return True if the item can be moved, false otherwise. 如果道具可以移动则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem", meta=(AutoCreateRefTerm="ItemInfo"))
	virtual bool CanMoveItem(const FSigilItemInfo& ItemInfo) const;

	/**
	 * Moves an item within the inventory.
	 * 在库存内移动道具。
	 * @param ItemInfo The item info specifying the source and target collection. 指定源集合和目标集合的道具信息。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem", meta=(AutoCreateRefTerm="ItemInfo"))
	virtual void MoveItem(const FSigilItemInfo& ItemInfo);

	/**
	 * Server-side function to move an item within the inventory.
	 * 服务器端函数，在库存内移动道具。
	 * @param ItemInfo The item info specifying the source and target collection. 指定源集合和目标集合的道具信息。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|InventorySystem")
	virtual void ServerMoveItem(const FSigilItemInfo& ItemInfo);

	/**
	 * Checks if an item can be removed from the inventory.
	 * 检查道具是否可以从库存移除。
	 * @param ItemInfo The item info to check. 要检查的道具信息。
	 * @return True if the item can be removed, false otherwise. 如果道具可以移除则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	bool CanRemoveItem(const FSigilItemInfo& ItemInfo) const;

	/**
	 * Removes an item from the inventory.
	 * 从库存移除道具。
	 * @param ItemInfo The item info specifying the item and amount to remove. 指定要移除的道具和数量的道具信息。
	 * @return The item info of the actually removed items. 实际移除的道具信息。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	FSigilItemInfo RemoveItem(const FSigilItemInfo& ItemInfo);

	/**
	 * Server-side function to remove an item from the inventory.
	 * 服务器端函数，从库存移除道具。
	 * @param ItemInfo The item info specifying the item and amount to remove. 指定要移除的道具和数量的道具信息。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|InventorySystem")
	void ServerRemoveItem(FSigilItemInfo ItemInfo);

	/**
	 * Implementation of the server-side item removal.
	 * 服务器端道具移除的实现。
	 * @param ItemInfo The item info specifying the item and amount to remove. 指定要移除的道具和数量的道具信息。
	 */
	virtual void ServerRemoveItem_Implementation(FSigilItemInfo ItemInfo);

	/**
	 * Removes an item from the inventory by its definition.
	 * 通过道具定义从库存移除道具。
	 * @param ItemDefinition The item definition to remove. 要移除的道具定义。
	 * @param Amount The amount to remove. 要移除的数量。
	 * @return The item info of the actually removed items. 实际移除的道具信息。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual FSigilItemInfo RemoveItemByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, const int32 Amount);

	/**
	 * Removes all items from the inventory.
	 * 从库存移除所有道具。
	 * @param RemoveItemsFromIgnoredCollections Whether to remove items from ignored collections. 是否移除忽略集合中的道具。
	 * @param DisableEventsWhileRemoving Whether to disable events during removal. 是否在移除期间禁用事件。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual void RemoveAllItems(bool RemoveItemsFromIgnoredCollections = false, bool DisableEventsWhileRemoving = true);

#pragma endregion

#pragma region Item Queries
	/**
	 * Gets the amount of a specific item in the inventory.
	 * 获取库存中特定道具的数量。
	 * @param Item The item instance to check. 要检查的道具实例。
	 * @param SimilarItem Whether to count similar items or exact matches. 是否统计相似道具或精确匹配。
	 * @return The amount of the item in the inventory. 库存中的道具数量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual int32 GetItemAmount(USigilItemInstance* Item, bool SimilarItem = true) const;

	/**
	 * Gets the total amount of items with the specified definition in all collections.
	 * 获取所有集合中具有指定定义的道具总数。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param Unique Whether to count unique items or total amounts. 是否统计唯一道具或总数量。
	 * @return The number of items with the specified definition. 具有指定定义的道具数量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual int32 GetItemAmountByDefinition(TSoftObjectPtr<USigilItemDefinition> ItemDefinition, bool Unique) const;

	/**
	 * Gets item info for a specific item in a collection.
	 * 获取集合中特定道具的道具信息。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param OutItemInfo The item info (output). 道具信息（输出）。
	 * @return True if the item was found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	virtual bool GetItemInfoInCollection(USigilItemInstance* Item, UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                     const FGameplayTag CollectionTag, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Finds item info for a specific item in a collection.
	 * 在集合中查找特定道具的道具信息。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param OutItemInfo The item info (output). 道具信息（输出）。
	 * @return True if the item was found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|InventorySystem", meta=(ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindItemInfoInCollection(USigilItemInstance* Item, UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                      const FGameplayTag CollectionTag, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Gets all item infos in a specified collection.
	 * 获取指定集合中的所有道具信息。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param OutItemInfos The array of item infos (output). 道具信息数组（输出）。
	 * @return True if any items were found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	virtual bool GetAllItemInfosInCollection(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                         const FGameplayTag CollectionTag, TArray<FSigilItemInfo>& OutItemInfos) const;

	/**
	 * Finds all item infos in a specified collection.
	 * 查找指定集合中的所有道具信息。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param OutItemInfos The array of item infos (output). 道具信息数组（输出）。
	 * @return True if any items were found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|InventorySystem", meta=(ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindAllItemInfosInCollection(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                          const FGameplayTag CollectionTag, TArray<FSigilItemInfo>& OutItemInfos) const;

	/**
	 * Retrieves information about an item instance in the inventory.
	 * 检索库存中指定道具实例的信息。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @param ItemInfo The item info (output). 道具信息（输出）。
	 * @return True if the item was found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	virtual bool GetItemInfo(USigilItemInstance* Item, FSigilItemInfo& ItemInfo) const;

	/**
	 * Finds information about an item instance in the inventory.
	 * 查找库存中指定道具实例的信息。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @param ItemInfo The item info (output). 道具信息（输出）。
	 * @return True if the item was found, false otherwise. 如果找到道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|InventorySystem", meta=(ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindItemInfo(USigilItemInstance* Item, FSigilItemInfo& ItemInfo) const;

	/**
	 * Retrieves the first item info matching the specified definition.
	 * 检索匹配指定定义的第一个道具信息。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param OutItemInfo The first matching item info (output). 匹配的第一个道具信息（输出）。
	 * @return True if a matching item was found, false otherwise. 如果找到匹配的道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	virtual bool GetItemInfoByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Finds the first item info matching the specified definition.
	 * 查找匹配指定定义的第一个道具信息。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param OutItemInfo The first matching item info (output). 匹配的第一个道具信息（输出）。
	 * @return True if a matching item was found, false otherwise. 如果找到匹配的道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|InventorySystem", meta=(ExpandBoolAsExecs="ReturnValue"))
	bool FindItemInfoByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Retrieves all item infos matching the specified definition across all collections.
	 * 检索所有集合中匹配指定定义的道具信息。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param OutItemInfos The array of matching item infos (output). 匹配的道具信息数组（输出）。
	 * @return True if any matching items were found, false otherwise. 如果找到匹配的道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	virtual bool GetItemInfosByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TArray<FSigilItemInfo>& OutItemInfos) const;

	/**
	 * Finds all item infos matching the specified definition across all collections.
	 * 查找所有集合中匹配指定定义的道具信息。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param OutItemInfos The array of matching item infos (output). 匹配的道具信息数组（输出）。
	 * @return True if any matching items were found, false otherwise. 如果找到匹配的道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|InventorySystem", meta=(ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindItemInfosByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TArray<FSigilItemInfo>& OutItemInfos) const;

	/**
	 * Gets all item infos from the inventory.
	 * 获取库存中的所有道具信息。
	 * @return The array of all item infos in the inventory. 库存中的所有道具信息数组。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual TArray<FSigilItemInfo> GetItemInfos() const;

	/**
	 * Checks if the inventory has enough items of a specific definition.
	 * 检查库存中是否有足够多的指定道具。
	 * @param ItemDefinition The item definition to check. 要检查的道具定义。
	 * @param Amount The required amount. 所需数量。
	 * @return True if there are enough items, false otherwise. 如果道具数量足够则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual bool HasEnoughItem(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, int32 Amount) const;

#pragma endregion

#pragma region Collections
	/**
	 * Gets all collections in the inventory.
	 * 获取库存中的所有集合。
	 * @return The array of item collections. 道具集合数组。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual TArray<USigilItemCollection*> GetItemCollections() const;

	/**
	 * Checks if all default collections have been created.
	 * 检查是否已创建所有默认集合。
	 * @return True if all default collections are created, false otherwise. 如果所有默认集合都已创建则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual bool IsDefaultCollectionCreated() const;

	/**
	 * Determines the target collection for an item.
	 * 确定道具的目标集合。
	 * @param ItemInfo The item info to determine the collection for. 要确定集合的道具信息。
	 * @return The target item collection. 目标道具集合。
	 */
	USigilItemCollection* DetermineTargetCollection(const FSigilItemInfo& ItemInfo) const;

	/**
	 * Gets the default item collection.
	 * 获取默认道具集合。
	 * @return The default item collection. 默认道具集合。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual USigilItemCollection* GetDefaultCollection() const;

	/**
	 * Gets the number of collections in the inventory.
	 * 获取库存中的集合数量。
	 * @return The number of collections. 集合数量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	int32 GetCollectionCount() const;

	/**
	 * Gets a collection by its tag.
	 * 通过标签获取集合。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @return The collection with the specified tag. 具有指定标签的集合。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	USigilItemCollection* GetCollectionByTag(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
		const FGameplayTag CollectionTag) const;

	/**
	 * Gets a collection by matching tags.
	 * 通过匹配标签获取集合。
	 * @param Tags The tags to match. 要匹配的标签。
	 * @return The collection matching the tags. 匹配标签的集合。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	virtual USigilItemCollection* GetCollectionByTags(FGameplayTagContainer Tags);

	/**
	 * Gets a collection by its ID.
	 * 通过ID获取集合。
	 * @param CollectionId The ID of the collection. 集合的ID。
	 * @return The collection with the specified ID. 具有指定ID的集合。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|InventorySystem")
	USigilItemCollection* GetCollectionById(FGuid CollectionId) const;

	/**
	 * Gets a typed collection by its tag.
	 * 通过标签获取类型化的集合。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param DesiredClass The desired class of the collection. 集合的期望类。
	 * @return The typed collection with the specified tag. 具有指定标签的类型化集合。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem", meta=(DeterminesOutputType="DesiredClass", DynamicOutputParam="ReturnValue"))
	USigilItemCollection* GetTypedCollectionByTag(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                                             const FGameplayTag CollectionTag, TSubclassOf<USigilItemCollection> DesiredClass) const;

	/**
	 * Finds a typed collection by its tag.
	 * 通过标签查找类型化的集合。
	 * @param CollectionTag The tag of the collection. 集合的标签。
	 * @param DesiredClass The desired class of the collection. 集合的期望类。
	 * @param OutCollection The found collection (output). 找到的集合（输出）。
	 * @return True if the collection was found, false otherwise. 如果找到集合则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem", meta=(DeterminesOutputType="DesiredClass", DynamicOutputParam="OutCollection", ExpandBoolAsExecs="ReturnValue"))
	bool FindTypedCollectionByTag(UPARAM(meta=(Categories="Sigil.Inventory.Collection"))
	                              const FGameplayTag CollectionTag, TSubclassOf<USigilItemCollection> DesiredClass, USigilItemCollection*& OutCollection);

	/**
	 * Adds a collection to the inventory by its definition.
	 * 通过定义将集合添加到库存。
	 * @param CollectionDefinition The collection definition to add. 要添加的集合定义。
	 * @return The added collection instance. 添加的集合实例。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|InventorySystem")
	virtual USigilItemCollection* AddCollectionByDefinition(TSoftObjectPtr<const USigilItemCollectionDefinition> CollectionDefinition);

	/**
	 * Removes a collection entry by its index.
	 * 通过索引移除集合条目。
	 * @param Idx The index of the collection entry to remove. 要移除的集合条目索引。
	 */
	virtual void RemoveCollectionEntry(int32 Idx);

	/**
	 * Adds a collection entry to the inventory.
	 * 将集合条目添加到库存。
	 * @param NewEntry The new collection entry to add. 要添加的新集合条目。
	 * @return True if the entry was added, false otherwise. 如果条目被添加则返回true，否则返回false。
	 */
	virtual bool AddCollectionEntry(const FSigilCollectionEntry& NewEntry);

	/**
	 * Creates and initializes a new item collection from a definition.
	 * 从定义创建并初始化新的道具集合。
	 * @param CollectionDefinition The collection definition. 集合定义。
	 * @return The newly created uninitialized item collection instance. 新创建的未初始化道具集合实例。
	 */
	virtual USigilItemCollection* CreateCollectionInstance(const USigilItemCollectionDefinition* CollectionDefinition);

	/**
	 * Checks if a collection should be ignored when searching for items.
	 * 检查集合在搜索道具时是否应被忽略。
	 * @param ItemCollection The collection to check. 要检查的集合。
	 * @return True if the collection should be ignored, false otherwise. 如果集合应被忽略则返回true，否则返回false。
	 */
	virtual bool IsIgnoredCollection(USigilItemCollection* ItemCollection) const;

protected:
	/**
	 * Called when a collection is added to the inventory.
	 * 集合添加到库存时调用。
	 * @param Entry The added collection entry. 添加的集合条目。
	 */
	virtual void OnCollectionAdded(const FSigilCollectionEntry& Entry);

	/**
	 * Called when a collection is removed from the inventory.
	 * 集合从库存移除时调用。
	 * @param Entry The removed collection entry. 移除的集合条目。
	 */
	virtual void OnCollectionRemoved(const FSigilCollectionEntry& Entry);

	/**
	 * Called when a collection is updated.
	 * 集合更新时调用。
	 * @param Entry The updated collection entry. 更新的集合条目。
	 */
	virtual void OnCollectionUpdated(const FSigilCollectionEntry& Entry);

	/**
	 * Processes pending collections in the inventory.
	 * 处理库存中的待处理集合。
	 */
	virtual void ProcessPendingCollections();

	/**
	 * Map of pending collection entries.
	 * 待处理集合条目的映射。
	 */
	UPROPERTY(VisibleAnywhere, Category="InventorySystem", Transient)
	TMap<FGuid, FSigilCollectionEntry> PendingCollections;

#pragma endregion

#pragma region Properties
	/**
	 * Predefined collections for the inventory.
	 * 库存的预定义集合。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InventorySystem")
	TArray<TObjectPtr<const USigilItemCollectionDefinition>> CollectionDefinitions;

	/**
	 * Default items for initial collections.
	 * 默认集合的默认道具。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InventorySystem", meta=(TitleProperty="Tag"))
	TArray<FSigilDefaultLoadout> DefaultLoadouts;

	/**
	 * Container for the inventory's collections.
	 * 库存集合的容器。
	 */
	UPROPERTY(VisibleAnywhere, Replicated, Category="InventorySystem", meta=(ShowOnlyInnerProperties))
	FSigilCollectionContainer CollectionContainer;

	/**
	 * Cached map for O(1) collection lookups by ID.
	 * 按ID进行O(1)集合查找的缓存映射。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InventorySystem", Transient, meta=(ForceInlineRow))
	TMap<FGuid, TObjectPtr<USigilItemCollection>> CollectionIdToInstanceMap;

	/**
	 * Whether to initialize the inventory on BeginPlay.
	 * 是否在BeginPlay时初始化库存。
	 */
	UPROPERTY(EditAnywhere, Category="InventorySystem")
	bool bInitializeOnBeginplay = false;

	/**
	 * Indicates if the inventory system is initialized.
	 * 指示库存系统是否已初始化。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InventorySystem", ReplicatedUsing=OnInventorySystemInitialized)
	bool bInventorySystemInitialized{false};

	/**
	 * Collections with these tags will be ignored when searching for items.
	 * 搜索道具时将忽略具有这些标签的集合。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="InventorySystem")
	FGameplayTagContainer IgnoredCollections;

	/**
	 * The associated currency system component.
	 * 关联的货币系统组件。
	 */
	UPROPERTY()
	USigilCurrencySystemComponent* CurrencySystem;

#pragma endregion

#pragma region Editor
#if WITH_EDITOR
	/**
	 * Called after the component is loaded in the editor.
	 * 编辑器中组件加载后调用。
	 */
	virtual void PostLoad() override;

	/**
	 * Called before the component is saved in the editor.
	 * 编辑器中组件保存前调用。
	 * @param SaveContext The save context. 保存上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	/**
	 * Validates the component's data in the editor.
	 * 在编辑器中验证组件的数据。
	 * @param Context The validation context. 验证上下文。
	 * @return The result of the data validation. 数据验证的结果。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
#pragma endregion
};
