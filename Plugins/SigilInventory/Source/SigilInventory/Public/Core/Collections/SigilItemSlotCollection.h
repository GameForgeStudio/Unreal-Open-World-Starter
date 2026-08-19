// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemCollection.h"
#include "SigilItemSlotCollection.generated.h"

/**
 * A slot-based item collection definition, suitable for equipment or skill bars.
 * 基于槽的道具集合定义，适合用于装备或技能栏。
 * @details Stores items in a slot-to-item style, ideal for equipment, skill bars, etc.
 * @细节 以槽对应道具的方式存储，适合装备、技能栏等场景。
 */
UCLASS(BlueprintType)
class SIGILINVENTORY_API USigilItemSlotCollectionDefinition : public USigilItemCollectionDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Gets the class for instantiating the collection.
	 * 获取用于实例化集合的类。
	 * @return The collection instance class. 集合实例类。
	 */
	virtual TSubclassOf<USigilItemCollection> GetCollectionInstanceClass() const override;

	/**
	 * Checks if the specified slot index is valid within SlotDefinitions.
	 * 检查指定槽索引在SlotDefinitions中是否有效。
	 * @param SlotIndex The slot index to check. 要检查的槽索引。
	 * @return True if the slot index is valid, false otherwise. 如果槽索引有效则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|CollectionDefinition")
	virtual bool IsValidSlotIndex(int32 SlotIndex) const;

	/**
	 * Gets the index of a slot based on its tag within SlotDefinitions.
	 * 根据槽标签获取SlotDefinitions中的槽索引。
	 * @param SlotName The slot tag to query. 要查询的槽标签。
	 * @return The index of the slot, or INDEX_NONE if not found. 槽的索引，如果未找到则返回INDEX_NONE。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|CollectionDefinition")
	virtual int32 GetIndexOfSlot(const FGameplayTag& SlotName) const;

	/**
	 * Gets the slot tag for a given slot index within SlotDefinitions.
	 * 根据槽索引获取SlotDefinitions中的槽标签。
	 * @param SlotIndex The slot index to query. 要查询的槽索引。
	 * @return The slot tag, or invalid tag if not found. 槽标签，如果未找到则返回无效标签。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|CollectionDefinition")
	virtual FGameplayTag GetSlotOfIndex(int32 SlotIndex) const;

	/**
	 * Gets all slot definitions within this collection definition.
	 * 获取此集合定义中的所有槽定义。
	 * @return Array of slot definitions. 槽定义数组。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|CollectionDefinition")
	const TArray<FSigilItemSlotDefinition>& GetSlotDefinitions() const;

	/**
	 * Gets the slot definition for a given slot index.
	 * 获取指定槽索引的槽定义。
	 * @param SlotIndex The slot index to query. 要查询的槽索引。
	 * @param OutDefinition The slot definition (output). 槽定义（输出）。
	 * @return True if the slot definition was found, false otherwise. 如果找到槽定义则返回true，否则返回false。
	 */
	bool GetSlotDefinition(int32 SlotIndex, FSigilItemSlotDefinition& OutDefinition) const;

	/**
	 * Gets the slot definition for a given slot tag.
	 * 获取指定槽标签的槽定义。
	 * @param SlotName The slot tag to query. 要查询的槽标签。
	 * @param OutDefinition The slot definition (output). 槽定义（输出）。
	 * @return True if the slot definition was found, false otherwise. 如果找到槽定义则返回true，否则返回false。
	 */
	bool GetSlotDefinition(const FGameplayTag& SlotName, FSigilItemSlotDefinition& OutDefinition) const;

	/**
	 * Gets the index of a slot within a specified group.
	 * 获取指定槽在指定槽组内的索引。
	 * @param GroupTag The group tag to query. 要查询的组标签。
	 * @param SlotTag The slot tag to query. 要查询的槽标签。
	 * @return The index of the slot within the group, or -1 if not found. 槽在组内的索引，如果未找到则返回-1。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|CollectionDefinition")
	int32 GetSlotIndexWithinGroup(UPARAM(meta=(Categories="Sigil.Inventory.Slots"))
	                              FGameplayTag GroupTag, FGameplayTag SlotTag) const;

	/**
	 * Whether newly added items replace existing items if they are not stackable.
	 * 新添加的道具如果无法堆叠，是否替换现有道具。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Common", meta=(DisplayAfter="OverflowOptions"))
	bool bNewItemPriority = true;

	/**
	 * Whether replaced items are returned to the source collection of the new item.
	 * 被替换的道具是否返回到新道具的源集合。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Common", meta=(EditCondition="bNewItemPriority"))
	bool bTryGivePrevItemToNewItemCollection = true;

	/**
	 * Defines all available item slots in the collection.
	 * 定义集合中的所有可用道具槽。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SlotSettings", meta=(TitleProperty="SlotName:{Name}"))
	TArray<FSigilItemSlotDefinition> SlotDefinitions;

	/**
	 * List of parent slot tags for grouping slots; only one slot per group can be active at a time.
	 * 用于分组的父级槽标签列表，每个组同时只能激活一个槽。
	 * @attention Used by the equipment system.
	 * @注意 由装备系统使用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SlotSettings", meta=(AllowPrivateAccess=True, Categories="Sigil.Inventory.Slots"))
	TArray<FGameplayTag> SlotGroups;

	/**
	 * Cached mapping of slot indices to slot tags for faster access, updated on save in the editor.
	 * 槽索引到槽标签的缓存映射，用于快速访问，在编辑器保存时更新。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Advanced", meta=(ForceInlineRow))
	TMap<int32, FGameplayTag> IndexToTagMap;

	/**
	 * Cached mapping of slot tags to slot indices for faster access, updated on save in the editor.
	 * 槽标签到槽索引的缓存映射，用于快速访问，在编辑器保存时更新。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Advanced", meta=(ForceInlineRow))
	TMap<FGameplayTag, int32> TagToIndexMap;

	/**
	 * Cached grouping of slot definitions by slot groups, updated on save in the editor.
	 * 按槽组分组的槽定义缓存，在编辑器保存时更新。
	 */
	UPROPERTY(VisibleAnywhere, Category="Advanced", meta=(ForceInlineRow))
	TMap<FGameplayTag, FSigilItemSlotGroup> SlotGroupMap;

#if WITH_EDITOR
	/**
	 * Called before saving the object in the editor.
	 * 在编辑器中保存对象前调用。
	 * @param SaveContext The save context. 保存上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};

/**
 * A slot-based item collection that organizes items in slots, each holding an item stack.
 * 基于槽的道具集合，按槽组织道具，每个槽可持有道具栈。
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, DisplayName="GIS Item Collection (Slotted)")
class SIGILINVENTORY_API USigilItemSlotCollection : public USigilItemCollection
{
	GENERATED_BODY()

	friend USigilItemSlotCollectionDefinition;

public:
	/**
	 * Constructor for the slot-based item collection.
	 * 基于槽的道具集合的构造函数。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilItemSlotCollection(const FObjectInitializer& ObjectInitializer);

	/**
	 * Gets the properties that should be replicated for this object.
	 * 获取需要为此对象复制的属性。
	 * @param OutLifetimeProps Array to store the replicated properties. 存储复制属性的数组。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Gets the definition of this slot-based collection.
	 * 获取此基于槽的集合的定义。
	 * @return The collection definition, or nullptr if not set. 集合定义，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	const USigilItemSlotCollectionDefinition* GetMyDefinition() const;

	/**
	 * Adds an item to the collection, typically for unique items.
	 * 将道具添加到集合，通常用于唯一道具。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @return The item that was not added, the previously equipped item if replaced, or empty if added successfully. 未添加的道具、替换的先前道具或成功添加时为空。
	 */
	virtual FSigilItemInfo AddItem(const FSigilItemInfo& ItemInfo) override;

	/**
	 * Adds an item to the specified slot (authority only).
	 * 将道具添加到指定槽（仅限权限）。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @param SlotIndex The index of the slot to add the item to. 要添加到的槽索引。
	 * @return The item that was actually added. 实际添加的道具。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemCollection")
	virtual FSigilItemInfo AddItem(const FSigilItemInfo& ItemInfo, int32 SlotIndex);

	/**
	 * Server function to add an item to the specified slot.
	 * 服务器函数，将道具添加到指定槽。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @param SlotIndex The index of the slot to add the item to. 要添加到的槽索引。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemCollection")
	void ServerAddItem(const FSigilItemInfo& ItemInfo, int32 SlotIndex);

	/**
	 * Adds an item to the slot with the specified tag (authority only).
	 * 将道具添加到指定标签的槽（仅限权限）。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @param SlotName The tag of the slot to add the item to. 要添加到的槽标签。
	 * @return The item that was actually added. 实际添加的道具。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemCollection")
	FSigilItemInfo AddItemBySlotName(const FSigilItemInfo& ItemInfo, FGameplayTag SlotName);

	/**
	 * Server function to add an item to the slot with the specified tag.
	 * 服务器函数，将道具添加到指定标签的槽。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @param SlotName The tag of the slot to add the item to. 要添加到的槽标签。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemCollection")
	void ServerAddItemBySlotName(const FSigilItemInfo& ItemInfo, FGameplayTag SlotName);

	/**
	 * Removes an item from the collection.
	 * 从集合中移除道具。
	 * @param ItemInfo The item information to remove. 要移除的道具信息。
	 * @return The item that was removed, or empty if nothing was removed. 移除的道具，如果未移除则为空。
	 */
	virtual FSigilItemInfo RemoveItem(const FSigilItemInfo& ItemInfo) override;

	/**
	 * Removes an item from the specified slot.
	 * 从指定槽移除道具。
	 * @param SlotIndex The index of the slot to remove the item from. 要移除道具的槽索引。
	 * @param Amount The amount to remove (-1 to remove all). 要移除的数量（-1表示全部移除）。
	 * @return The item information that was removed. 移除的道具信息。
	 */
	virtual FSigilItemInfo RemoveItem(int32 SlotIndex, int32 Amount = -1);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	bool IsItemFitWithSlot(const USigilItemInstance* Item, int32 SlotIndex) const;

	/**
	 * Gets a suitable slot index for an incoming item.
	 * 获取适合传入道具的槽索引。
	 * @param Item The item instance to check. 要检查的道具实例。
	 * @return A valid slot index, or INDEX_NONE if none is suitable. 有效槽索引，如果没有合适的槽则返回INDEX_NONE。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	int32 GetTargetSlotIndex(const USigilItemInstance* Item) const;

	/**
	 * Gets the item information at a specific slot by tag.
	 * 通过槽标签获取指定槽的道具信息。
	 * @param SlotTag The slot tag to query. 要查询的槽标签。
	 * @param OutItemInfo The item information (output). 道具信息（输出）。
	 * @return True if item information was found, false otherwise. 如果找到道具信息则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	virtual bool GetItemInfoAtSlot(FGameplayTag SlotTag, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Finds the item information at a specific slot by tag.
	 * 通过槽标签查找指定槽的道具信息。
	 * @param SlotTag The slot tag to query. 要查询的槽标签。
	 * @param OutItemInfo The item information (output). 道具信息（输出）。
	 * @return True if item information was found, false otherwise. 如果找到道具信息则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|ItemCollection", meta=(ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindItemInfoAtSlot(FGameplayTag SlotTag, FSigilItemInfo& OutItemInfo) const;

	/**
	 * Gets the item stack at a specific slot by tag (commented out).
	 * 通过槽标签获取指定槽的道具栈（已注释）。
	 */
	virtual bool GetItemStackAtSlot(FGameplayTag SlotTag, FSigilItemStack& OutItemStack) const;

	/**
	 * Finds the item stack at a specific slot by tag (commented out).
	 * 通过槽标签查找指定槽的道具栈（已注释）。
	 */
	virtual bool FindItemStackAtSlot(FGameplayTag SlotTag, FSigilItemStack& OutItemStack) const;

	/**
	 * Gets the item information at a specific slot by index.
	 * 通过槽索引获取指定槽的道具信息。
	 * @param SlotIndex The slot index to query. 要查询的槽索引。
	 * @return The item information at the slot. 槽中的道具信息。
	 */
	FSigilItemInfo GetItemInfoAtSlot(int32 SlotIndex) const;

	/**
	 * Gets the item stack at a specific slot by index.
	 * 通过槽索引获取指定槽的道具栈。
	 * @param SlotIndex The slot index to query. 要查询的槽索引。
	 * @return The item stack at the slot. 槽中的道具栈。
	 */
	FSigilItemStack GetItemStackAtSlot(int32 SlotIndex) const;

	/**
	 * Gets the slot tag where an item is equipped.
	 * 获取道具装备的槽标签。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @return The slot tag, or invalid if not equipped. 槽标签，如果未装备则返回无效标签。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	FGameplayTag GetItemSlotName(const USigilItemInstance* Item) const;

	/**
	 * Gets the slot index where an item is equipped.
	 * 获取道具装备的槽索引。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @return The slot index, or -1 if not equipped. 槽索引，如果未装备则返回-1。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	int32 GetItemSlotIndex(const USigilItemInstance* Item) const;

	/**
	 * Internal function to add an item to a specific slot.
	 * 内部函数，将道具添加到指定槽。
	 * @param ItemInfo The item information to add. 要添加的道具信息。
	 * @param SlotIndex The index of the slot to add the item to. 要添加到的槽索引。
	 * @return The item that was actually added. 实际添加的道具。
	 */
	virtual FSigilItemInfo AddItemInternal(const FSigilItemInfo& ItemInfo, int32 SlotIndex);

	/**
	 * Converts a stack ID to a slot index.
	 * 将栈ID转换为槽索引。
	 * @param InStackId The stack ID to query. 要查询的栈ID。
	 * @return The corresponding slot index, or -1 if not found. 对应的槽索引，如果未找到则返回-1。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	int32 StackIdToSlotIndex(FGuid InStackId) const;

	/**
	 * Converts a slot index to a stack index.
	 * 将槽索引转换为栈索引。
	 * @param InSlotIndex The slot index to query. 要查询的槽索引。
	 * @return The corresponding stack index, or -1 if not found. 对应的栈索引，如果未找到则返回-1。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	int32 SlotIndexToStackIndex(int32 InSlotIndex) const;

	/**
	 * Converts a slot index to a stack ID.
	 * 将槽索引转换为栈ID。
	 * @param InSlotIndex The slot index to query. 要查询的槽索引。
	 * @return The corresponding stack ID, or invalid if not found. 对应的栈ID，如果未找到则返回无效ID。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemCollection")
	FGuid SlotIndexToStackId(int32 InSlotIndex) const;

	/**
	 * Mapping of slot indices to stack IDs for equipped items.
	 * 槽索引到装备道具栈ID的映射。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ItemCollection", ReplicatedUsing=OnRep_ItemsBySlot)
	TArray<FGuid> ItemBySlots;

	UPROPERTY(VisibleInstanceOnly, Category="ItemCollection", Transient)
	TMap<int32, FGuid> SlotToStackMap;

	/**
	 * The definition of this slot-based collection.
	 * 此基于槽的集合的定义。
	 */
	UPROPERTY()
	TObjectPtr<const USigilItemSlotCollectionDefinition> MyDefinition;

	/**
	 * Called when the ItemBySlots array is replicated.
	 * ItemBySlots数组复制时调用。
	 */
	UFUNCTION()
	void OnRep_ItemsBySlot();

protected:
	/**
	 * Sets the collection definition.
	 * 设置集合定义。
	 * @param NewDefinition The new collection definition. 新的集合定义。
	 */
	virtual void SetDefinition(const USigilItemCollectionDefinition* NewDefinition) override;

	/**
	 * Called before an item stack is added to the collection.
	 * 在集合添加道具栈之前调用。
	 * @param Stack The item stack to add. 要添加的道具栈。
	 * @param Idx The index where the stack will be added. 栈将添加的索引。
	 */
	virtual void OnPreItemStackAdded(const FSigilItemStack& Stack, int32 Idx) override;

	virtual void OnItemStackAdded(const FSigilItemStack& Stack) override;
	virtual void OnItemStackRemoved(const FSigilItemStack& Stack) override;

	/**
	 * Sets the amount for an item in a specific slot.
	 * 在指定槽中设置道具数量。
	 * @param ItemInfo The item information to set. 要设置的道具信息。
	 * @param SlotIndex The slot index to set the item in. 要设置的槽索引。
	 * @param RemovePreviousItem Whether to replace the previous item if the slot is at capacity. 如果槽已满，是否替换之前的道具。
	 * @param ItemInfoAdded The item information that was set (output). 已设置的道具信息（输出）。
	 * @return True if the item was set successfully, false otherwise. 如果道具设置成功则返回true，否则返回false。
	 */
	virtual bool SetItemAmount(const FSigilItemInfo& ItemInfo, int32 SlotIndex, bool RemovePreviousItem, FSigilItemInfo& ItemInfoAdded);
};
