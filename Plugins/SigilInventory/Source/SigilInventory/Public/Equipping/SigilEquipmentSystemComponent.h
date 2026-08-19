// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilCoreStructLibrary.h"
#include "SigilEquipmentContainer.h"
#include "SigilInventoryMessages.h"
#include "Items/SigilItemInfo.h"
#include "Components/PawnComponent.h"
#include "SigilEquipmentSystemComponent.generated.h"

class USigilItemSlotCollectionDefinition;
class USigilInventorySystemComponent;
class USigilEquipmentInstance;
class USigilItemSlotCollection;

/**
 * Delegate triggered when the equipment system is initialized.
 * 装备系统初始化时触发的委托。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSigilEquipment_InitializedSignature);

/**
 * Delegate triggered when an equipment's state changes (equipped or unequipped).
 * 装备状态更改（装备或卸下）时触发的委托。
 * @param Equipment The equipment instance. 装备实例。
 * @param SlotTag The slot tag associated with the equipment. 与装备关联的槽标签。
 * @param bEquipped Whether the equipment is equipped. 装备是否已装备。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSigilEquipment_StateChangedSignature, UObject*, Equipment, FGameplayTag, SlotTag, bool, bEquipped);

/**
 * Delegate triggered when an equipment's active state changes.
 * 装备激活状态更改时触发的委托。
 * @param Equipment The equipment instance. 装备实例。
 * @param SlotTag The slot tag associated with the equipment. 与装备关联的槽标签。
 * @param bActive Whether the equipment is active. 装备是否激活。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSigilEquipment_ActiveStateChangedSignature, UObject*, Equipment, FGameplayTag, SlotTag, bool, bActive);

/**
 * Delegate triggered when the active index of an equipment group changes.
 * 装备组的激活索引更改时触发的委托。
 * @param GroupTag The group tag of the equipment group. 装备组的标签。
 * @param PrevIndex The previous active index. 之前的激活索引。
 * @param NextIndex The new active index. 新的激活索引。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSigilEquipment_GroupActiveIndexChangedSignature, const FGameplayTag&, GroupTag, int32, PrevIndex, int32, NextIndex);

/**
 * Dynamic delegate for equipment system initialization events.
 * 装备系统初始化事件的动态委托。
 */
UDELEGATE()
DECLARE_DYNAMIC_DELEGATE(FSigilEquipmentSystem_Initialized_DynamicEvent);

/**
 * Manager of equipment instances for a pawn.
 * 管理棋子装备实例的组件。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilEquipmentSystemComponent : public UPawnComponent
{
	GENERATED_BODY()
	friend FSigilEquipmentContainer;
	friend FSigilEquipmentEntry;

public:
	/**
	 * Sets default values for this component's properties.
	 * 为组件的属性设置默认值。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilEquipmentSystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/**
	 * Uninitializes the component.
	 * 取消初始化组件。
	 */
	virtual void UninitializeComponent() override;

	/**
	 * Called when the game ends.
	 * 游戏结束时调用。
	 * @param EndPlayReason The reason the game ended. 游戏结束的原因。
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

#pragma region Equipment System
	/**
	 * Gets the equipment system component from the specified actor.
	 * 从指定演员获取装备系统组件。
	 * @param Actor The actor to query. 要查询的演员。
	 * @return The equipment system component, or nullptr if not found. 装备系统组件，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem", Meta = (DefaultToSelf="Actor"))
	static USigilEquipmentSystemComponent* GetEquipmentSystemComponent(const AActor* Actor);

	/**
	 * Finds the equipment system component on the specified actor.
	 * 在指定演员上查找装备系统组件。
	 * @param Actor The actor to search for the component. 要查找组件的演员。
	 * @param Component The found equipment system component (output). 找到的装备系统组件（输出）。
	 * @return True if the component was found, false otherwise. 如果找到组件则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem", Meta = (DefaultToSelf="Actor", ExpandBoolAsExecs = "ReturnValue"))
	static bool FindEquipmentSystemComponent(const AActor* Actor, USigilEquipmentSystemComponent*& Component);

	/**
	 * Finds a typed equipment system component on the specified actor.
	 * 在指定演员上查找类型化的装备系统组件。
	 * @param Actor The actor to search for the component. 要查找组件的演员。
	 * @param DesiredClass The desired class of the component. 组件的期望类。
	 * @param Component The found equipment system component (output). 找到的装备系统组件（输出）。
	 * @return True if the component was found, false otherwise. 如果找到组件则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem", meta=(DefaultToSelf="Actor", DynamicOutputParam="Component", DeterminesOutputType="DesiredClass", ExpandBoolAsExecs="ReturnValue"))
	static bool FindTypedEquipmentSystemComponent(AActor* Actor, TSubclassOf<USigilEquipmentSystemComponent> DesiredClass, USigilEquipmentSystemComponent*& Component);

	/**
	 * Initializes the equipment system.
	 * 初始化装备系统。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void InitializeEquipmentSystem();

	/**
	 * Initializes the equipment system with a specified inventory system.
	 * 使用指定的库存系统初始化装备系统。
	 * @param InventorySystem The inventory system to associate with. 要关联的库存系统。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void InitializeEquipmentSystemWithInventory(USigilInventorySystemComponent* InventorySystem);

	/**
	 * Resets the equipment system.
	 * 重置装备系统。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void ResetEquipmentSystem();

	/**
	 * Gets the target collection tag for equipment monitoring.
	 * 获取用于装备监控的目标集合标签。
	 * @return The target collection tag. 目标集合标签。
	 */
	FGameplayTag GetTargetCollectionTag() const { return TargetCollectionTag; };

	/**
	 * Checks if the equipment system is initialized.
	 * 检查装备系统是否已初始化。
	 * @return True if initialized, false otherwise. 如果已初始化则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	bool IsEquipmentSystemInitialized() const;

	/**
	 * Binds a delegate to be called when the equipment system is initialized.
	 * 绑定一个委托，在装备系统初始化时调用。
	 * @param Delegate The delegate to bind. 要绑定的委托。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|InventorySystem")
	void BindToEquipmentSystemInitialized(FSigilEquipmentSystem_Initialized_DynamicEvent Delegate);

	/**
	 * Removes all equipment instances from the system.
	 * 从系统中移除所有装备实例。
	 */
	virtual void RemoveAllEquipments();

	/**
	 * Equips an item to a specific slot.
	 * 将道具装备到指定槽。
	 * @param Item The item instance to equip. 要装备的道具实例。
	 * @param SlotTag The slot tag to equip the item to. 装备道具的槽标签。
	 */
	virtual void EquipItemToSlot(USigilItemInstance* Item, const FGameplayTag& SlotTag);

	/**
	 * Unequips an item from a specific slot.
	 * 从指定槽卸下装备。
	 * @param SlotTag The slot tag to unequip. 要卸下的槽标签。
	 */
	virtual void UnequipBySlot(FGameplayTag SlotTag);

	/**
	 * Unequips an item by its ID.
	 * 通过道具ID卸下装备。
	 * @param ItemId The ID of the item to unequip. 要卸下的道具ID。
	 */
	virtual void UnequipByItem(const FGuid& ItemId);

	/**
	 * Checks if the owning actor has authority.
	 * 检查拥有者演员是否具有权限。
	 * @return True if the owner has authority, false otherwise. 如果拥有者有权限则返回true，否则返回false。
	 */
	bool OwnerHasAuthority() const;

protected:
	/**
	 * Called when the target collection's item stacks change.
	 * 目标集合的道具堆栈更改时调用。
	 * @param Message The update message containing stack details. 包含堆栈详细信息的更新消息。
	 */
	UFUNCTION()
	virtual void OnTargetCollectionChanged(const FSigilInventoryStackUpdateMessage& Message);

	/**
	 * Called when the target collection is removed.
	 * 目标集合移除时调用。
	 * @param Collection The removed collection. 移除的集合。
	 */
	UFUNCTION()
	virtual void OnTargetCollectionRemoved(USigilItemCollection* Collection);

	/**
	 * Called when the equipment system is initialized.
	 * 装备系统初始化时调用。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|EquipmentSystem")
	void OnEquipmentSystemInitialized();

	/**
	 * List of delegates for equipment system initialization.
	 * 装备系统初始化的委托列表。
	 */
	UPROPERTY()
	TArray<FSigilEquipmentSystem_Initialized_DynamicEvent> InitializedDelegates;

#pragma endregion

#pragma region Equipments Query

public:
	/**
	 * Gets all equipment instances matching the specified conditions.
	 * 获取匹配指定条件的所有装备实例。
	 * @param InstanceType The type of equipment instance to query. 要查询的装备实例类型。
	 * @param SlotQuery The gameplay tag query for slots. 槽的游戏标签查询。
	 * @return Array of matching equipment instances, or empty if none found. 匹配的装备实例数组，如果未找到则返回空。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DeterminesOutputType=InstanceType, DynamicOutputParam=ReturnValue))
	TArray<UObject*> GetEquipments(UPARAM(meta=(MustImplement = "/Script/SigilInventory.SigilEquipmentInterface", AllowAbstract = "false"))
	                               TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const;

	/**
	 * Gets all active equipment instances matching the specified conditions.
	 * 获取匹配指定条件的激活装备实例。
	 * @param InstanceType The type of equipment instance to query. 要查询的装备实例类型。
	 * @param SlotQuery The gameplay tag query for slots. 槽的游戏标签查询。
	 * @return Array of active equipment instances, or empty if none found. 激活的装备实例数组，如果未找到则返回空。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DeterminesOutputType=InstanceType, DynamicOutputParam=ReturnValue))
	TArray<UObject*> GetActiveEquipments(UPARAM(meta=(MustImplement = "/Script/SigilInventory.SigilEquipmentInterface", AllowAbstract = "false"))
	                                     TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const;

	/**
	 * Gets the first equipment instance matching the specified query.
	 * 获取匹配指定查询的第一个装备实例。
	 * @param InstanceType The type of equipment instance to query. 要查询的装备实例类型。
	 * @param SlotQuery The gameplay tag query for slots. 槽的游戏标签查询。
	 * @return The first matching equipment instance, or nullptr if none found. 第一个匹配的装备实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DeterminesOutputType=InstanceType, DynamicOutputParam=ReturnValue))
	UObject* GetEquipment(UPARAM(meta=(MustImplement = "/Script/SigilInventory.SigilEquipmentInterface", AllowAbstract = "false"))
	                      TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const;

	/**
	 * Gets the first active equipment instance matching the specified query.
	 * 获取匹配指定查询的第一个激活装备实例。
	 * @param InstanceType The type of equipment instance to query. 要查询的装备实例类型。
	 * @param SlotQuery The gameplay tag query for slots. 槽的游戏标签查询。
	 * @return The first active equipment instance, or nullptr if none found. 第一个激活的装备实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DeterminesOutputType=InstanceType, DynamicOutputParam=ReturnValue))
	UObject* GetActiveEquipment(UPARAM(meta=(MustImplement = "/Script/SigilInventory.SigilEquipmentInterface", AllowAbstract = "false"))
	                            TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const;

	/**
	 * Gets the equipment instance that spawned the specified equipment actor.
	 * 获取生成指定装备演员的装备实例。
	 * @param EquipmentActor The equipment actor to query. 要查询的装备演员。
	 * @return The equipment instance, or nullptr if not found. 装备实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DefaultToSelf="EquipmentActor"))
	USigilEquipmentInstance* GetEquipmentInstanceOfActor(AActor* EquipmentActor) const;

	/**
	 * Gets the typed equipment instance that spawned the specified equipment actor.
	 * 获取生成指定装备演员的类型化装备实例。
	 * @param InstanceType The desired type of equipment instance. 期望的装备实例类型。
	 * @param EquipmentActor The equipment actor to query. 要查询的装备演员。
	 * @return The typed equipment instance, or nullptr if not found. 类型化的装备实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|EquipmentSystem", meta=(DefaultToSelf="EquipmentActor", DeterminesOutputType=InstanceType, DynamicOutputParam=ReturnValue))
	USigilEquipmentInstance* GetTypedEquipmentInstanceOfActor(TSubclassOf<USigilEquipmentInstance> InstanceType, AActor* EquipmentActor) const;

	/**
	 * Gets the equipment instance in a specific slot.
	 * 获取特定槽中的装备实例。
	 * @param SlotTag The slot tag to query. 要查询的槽标签。
	 * @return The equipment instance in the slot, or nullptr if none. 槽中的装备实例，如果没有则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem")
	UObject* GetEquipmentInSlot(FGameplayTag SlotTag) const;

	/**
	 * Gets the equipment instance associated with a specific item instance.
	 * 获取与特定道具实例关联的装备实例。
	 * @param Item The item instance to query. 要查询的道具实例。
	 * @return The equipment instance, or nullptr if none. 装备实例，如果没有则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem")
	UObject* GetEquipmentByItem(const USigilItemInstance* Item);

#pragma endregion

#pragma region Equipments Activation/Deactivation
	/**
	 * Sets the active state of an equipment in a specific slot.
	 * 设置特定槽中装备的激活状态。
	 * @param SlotTag The slot tag of the equipment. 装备的槽标签。
	 * @param NewActiveState The new active state. 新的激活状态。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void SetEquipmentActiveState(FGameplayTag SlotTag, bool NewActiveState);

	/**
	 * Server-side function to set the active state of an equipment in a specific slot.
	 * 服务器端函数，设置特定槽中装备的激活状态。
	 * @param SlotTag The slot tag of the equipment. 装备的槽标签。
	 * @param NewActiveState The new active state. 新的激活状态。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void ServerSetEquipmentActiveState(FGameplayTag SlotTag, bool NewActiveState);

protected:
	/**
	 * Checks if a slot can be activated within its group.
	 * 检查槽是否可以在其组内激活。
	 * @param SlotTag The slot tag to check. 要检查的槽标签。
	 * @param OutMatchedGroup The matched group tag (output). 匹配的组标签（输出）。
	 * @param OutIdxWithinGroup The index within the group (output). 组内的索引（输出）。
	 * @return True if the slot can be activated, false otherwise. 如果槽可以激活则返回true，否则返回false。
	 */
	virtual bool CanActiveSlotInGroup(FGameplayTag SlotTag, FGameplayTag& OutMatchedGroup, int32& OutIdxWithinGroup);

	/**
	 * Updates the active state of an equipment with group restrictions.
	 * 更新装备的激活状态，考虑组限制。
	 * @param Idx The index of the equipment entry. 装备条目索引。
	 * @param NewActiveState The new active state. 新的激活状态。
	 */
	virtual void SetEquipmentActiveStateWithGroupRestriction(int32 Idx, bool NewActiveState);

	/**
	 * Directly sets the active state of an equipment.
	 * 直接设置装备的激活状态。
	 * @param Idx The index of the equipment entry. 装备条目索引。
	 * @param NewActiveState The new active state. 新的激活状态。
	 */
	virtual void SetEquipmentActiveState(int32 Idx, bool NewActiveState);

#pragma endregion

#pragma region Events

public:
	/**
	 * Event triggered when the equipment system is initialized.
	 * 装备系统初始化时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilEquipment_InitializedSignature OnEquipmentSystemInitializedEvent;

	/**
	 * Event triggered when an equipment's state changes.
	 * 装备状态更改时触发的事件。
	 * @attention Called after equipping, before unequipping.
	 * @attention 在装备后、卸下前调用。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilEquipment_StateChangedSignature OnEquipmentStateChangedEvent;

	/**
	 * Event triggered when an equipment's active state changes.
	 * 装备激活状态更改时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilEquipment_ActiveStateChangedSignature OnEquipmentActiveStateChangedEvent;

	/**
	 * Event triggered when the active index of an equipment group changes.
	 * 装备组的激活索引更改时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilEquipment_GroupActiveIndexChangedSignature OnEquipmentGroupActiveIndexChangedEvent;

#pragma endregion

#pragma region Helpers
	/**
	 * Checks if a specific slot is equipped.
	 * 检查特定槽是否已装备。
	 * @param SlotTag The slot tag to check. 要检查的槽标签。
	 * @return True if the slot is equipped, false otherwise. 如果槽已装备则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem")
	bool IsSlotEquipped(FGameplayTag SlotTag) const;

	/**
	 * Converts a slot tag to the corresponding equipment entry index.
	 * 将槽标签转换为对应的装备条目索引。
	 * @param InSlotTag The slot tag to convert. 要转换的槽标签。
	 * @return The equipment entry index, or INDEX_NONE if not found. 装备条目索引，如果未找到则返回INDEX_NONE。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem")
	int32 SlotTagToEquipmentIndex(FGameplayTag InSlotTag) const;

	/**
	 * Converts an item ID to the corresponding equipment entry index.
	 * 将道具ID转换为对应的装备条目索引。
	 * @param InItemId The item ID to convert. 要转换的道具ID。
	 * @return The equipment entry index, or INDEX_NONE if not found. 装备条目索引，如果未找到则返回INDEX_NONE。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentSystem")
	int32 ItemIdToEquipmentInex(FGuid InItemId) const;

#pragma endregion

#pragma region Equipment Entries

protected:
	/**
	 * Adds an equipment entry to the system.
	 * 将装备条目添加到系统中。
	 * @param NewEntry The new equipment entry to add. 要添加的新装备条目。
	 */
	virtual void AddEquipmentEntry(const FSigilEquipmentEntry& NewEntry);

	/**
	 * Removes an equipment entry by its index.
	 * 通过索引移除装备条目。
	 * @param Idx The index of the equipment entry to remove. 要移除的装备条目索引。
	 */
	virtual void RemoveEquipmentEntry(int32 Idx);

	/**
	 * Creates an equipment instance for the specified item.
	 * 为指定道具创建装备实例。
	 * @attention The returned instance must implement SigilEquipmentInterface.
	 * @attention 返回的实例必须实现SigilEquipmentInterface。
	 * @param Owner The owning actor of the created equipment instance. 创建的装备实例的所属演员。
	 * @param ItemInstance The item instance containing equipment-related data. 包含装备相关数据的道具实例。
	 * @return The created equipment instance (UObject or AActor based on design). 创建的装备实例（根据设计为UObject或AActor）。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|EquipmentSystem")
	UObject* CreateEquipmentInstance(AActor* Owner, USigilItemInstance* ItemInstance) const;

	/**
	 * Called when an equipment entry is added.
	 * 装备条目添加时调用。
	 * @param Entry The added equipment entry. 添加的装备条目。
	 * @param Idx The index of the added entry. 添加条目的索引。
	 */
	virtual void OnEquipmentEntryAdded(const FSigilEquipmentEntry& Entry, int32 Idx);

	/**
	 * Called when an equipment entry is changed.
	 * 装备条目更改时调用。
	 * @param Entry The changed equipment entry. 更改的装备条目。
	 * @param Idx The index of the changed entry. 更改条目的索引。
	 */
	virtual void OnEquipmentEntryChanged(const FSigilEquipmentEntry& Entry, int32 Idx);

	/**
	 * Called when an equipment entry is removed.
	 * 装备条目移除时调用。
	 * @param Entry The removed equipment entry. 移除的装备条目。
	 * @param Idx The index of the removed entry. 移除条目的索引。
	 */
	virtual void OnEquipmentEntryRemoved(const FSigilEquipmentEntry& Entry, int32 Idx);

	/**
	 * Adds a replicated equipment object to the system.
	 * 将复制的装备对象添加到系统中。
	 * @param Instance The equipment instance to add. 要添加的装备实例。
	 */
	virtual void AddReplicatedEquipmentObject(TObjectPtr<UObject> Instance);

	/**
	 * Removes a replicated equipment object from the system.
	 * 从系统中移除复制的装备对象。
	 * @param Instance The equipment instance to remove. 要移除的装备实例。
	 */
	virtual void RemoveReplicatedEquipmentObject(TObjectPtr<UObject> Instance);

	/**
	 * Processes pending equipment entries.
	 * 处理待处理的装备条目。
	 */
	virtual void ProcessPendingEquipments();

	/**
	 * List of pending replicated equipment objects.
	 * 待复制的装备对象列表。
	 */
	TArray<TObjectPtr<UObject>> PendingReplicatedEquipments;

	/**
	 * Map of pending equipment entries.
	 * 待处理装备条目的映射。
	 */
	TMap<int32, FSigilEquipmentEntry> PendingEquipmentEntries;

#pragma endregion

#pragma region Equipment Groups

public:
	/**
	 * Gets the layout of an equipment group.
	 * 获取装备组的布局。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @return Map of indices to slot tags in the group. 组内索引到槽标签的映射。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual TMap<int32, FGameplayTag> GetLayoutOfGroup(FGameplayTag GroupTag) const;

	/**
	 * Gets the layout of an equipment group.
	 * 获取装备组的布局。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @return Map of indices to slot tags in the group. 组内索引到槽标签的映射。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual TMap<FGameplayTag, int32> GetSlottedLayoutOfGroup(FGameplayTag GroupTag) const;

	/**
	 * Gets all equipment instances in an equipment group.
	 * 获取装备组中的所有装备实例。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @return Map of indices to equipment instances in the group. 组内索引到装备实例的映射。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual TMap<int32, UObject*> GetEquipmentsOfGroup(FGameplayTag GroupTag) const;

	UFUNCTION(BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual TMap<FGameplayTag, UObject*> GetSlottedEquipmentsOfGroup(FGameplayTag GroupTag) const;

	/**
	 * Sets the active index within an equipment group.
	 * 设置装备组中的激活索引。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param NewIndex The new active index. 新的激活索引。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void SetGroupActiveIndex(FGameplayTag GroupTag, int32 NewIndex);

	/**
	 * Server-side function to set the active index within an equipment group.
	 * 服务器端函数，设置装备组中的激活索引。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param NewIndex The new active index. 新的激活索引。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual void ServerSetGroupActiveIndex(FGameplayTag GroupTag, int32 NewIndex);

	/**
	 * Cycles the active index within an equipment group in the specified direction.
	 * 按指定方向在装备组中循环激活索引。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param bDirection The cycle direction (true for right, false for left). 循环方向（true为右，false为左）。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|EquipmentSystem")
	virtual void CycleGroupActiveIndex(FGameplayTag GroupTag, bool bDirection = true);

	/**
	 * Server-side function to cycle the active index within an equipment group.
	 * 服务器端函数，按指定方向在装备组中循环激活索引。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param bDirection The cycle direction (true for right, false for left). 循环方向（true为右，false为左）。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="GIS|EquipmentSystem")
	virtual void ServerCycleGroupActiveIndex(FGameplayTag GroupTag, bool bDirection = true);

protected:
	/**
	 * Notifies the system of a change in an equipment group's active index.
	 * 通知系统装备组的激活索引发生更改。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param PrevIndex The previous active index. 之前的激活索引。
	 * @param NewIndex The new active index. 新的激活索引。
	 */
	virtual void OnGroupActiveIndexChanged(const FGameplayTag& GroupTag, int32 PrevIndex, int32 NewIndex);

	/**
	 * Notifies the client of a change in an equipment group's active index.
	 * 通知客户端装备组的激活索引发生更改。
	 * @param GroupTag The tag of the equipment group. 装备组的标签。
	 * @param PrevIndex The previous active index. 之前的激活索引。
	 * @param NewIndex The new active index. 新的激活索引。
	 */
	UFUNCTION(Client, Reliable)
	virtual void ClientNotifyGroupActiveIndexChanged(const FGameplayTag& GroupTag, int32 PrevIndex, int32 NewIndex);

#pragma endregion

#pragma region Properties
	/**
	 * Whether to initialize the equipment system automatically on BeginPlay.
	 * 是否在BeginPlay时自动初始化装备系统。
	 */
	UPROPERTY(EditDefaultsOnly, Category="EquipmentSystem")
	bool bInitializeOnBeginPlay = false;

	/**
	 * Indicates if the equipment system is initialized.
	 * 指示装备系统是否已初始化。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EquipmentSystem", ReplicatedUsing=OnEquipmentSystemInitialized)
	bool bEquipmentSystemInitialized = false;

	/**
	 * The target collection tag for monitoring equipment-related items.
	 * 用于监控装备相关道具的目标集合标签。
	 * @attention Monitors the specified item slot collection in the actor's inventory and generates/removes equipment instances based on changes.
	 * @attention 监听同一演员库存中的指定道具槽集合，并根据变化生成/移除装备实例。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EquipmentSystem", meta=(AllowPrivateAccess=True, Categories="Sigil.Inventory.Collection"))
	FGameplayTag TargetCollectionTag;

	/**
	 * Container for the equipment entries.
	 * 装备条目的容器。
	 */
	UPROPERTY(VisibleInstanceOnly, Category="EquipmentSystem", Replicated, meta=(ShowOnlyInnerProperties))
	FSigilEquipmentContainer Container;

	/**
	 * The definition of the target collection where equipment groups are defined.
	 * 定义装备组的目标集合定义。
	 */
	UPROPERTY(VisibleInstanceOnly, Category="EquipmentSystem", Replicated)
	TObjectPtr<const USigilItemSlotCollectionDefinition> TargetCollectionDefinition;

	/**
	 * Map tracking the active index within each equipment group.
	 * 跟踪每个装备组中激活索引的映射。
	 */
	UPROPERTY(VisibleInstanceOnly, Category="EquipmentSystem", meta=(ForceInlineRow))
	TMap<FGameplayTag, int32> GroupActiveIdxMap;

	/**
	 * The associated inventory system component.
	 * 关联的库存系统组件。
	 */
	UPROPERTY()
	TObjectPtr<USigilInventorySystemComponent> Inventory;

	/**
	 * The target item slot collection.
	 * 目标道具槽集合。
	 */
	UPROPERTY()
	TObjectPtr<USigilItemSlotCollection> TargetCollection;

	/**
	 * Mapping of slot tags to equipment entry indices.
	 * 槽标签到装备条目索引的映射。
	 */
	UPROPERTY(VisibleInstanceOnly, Category="EquipmentSystem", meta=(ForceInlineRow), Transient)
	TMap<FGameplayTag, TObjectPtr<UObject>> SlotToIdxMap;

#pragma endregion
};
