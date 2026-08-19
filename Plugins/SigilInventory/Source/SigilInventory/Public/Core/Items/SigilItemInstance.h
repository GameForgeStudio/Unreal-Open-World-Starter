// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "Attributes/SigilGameplayTagFloat.h"
#include "Attributes/SigilGameplayTagInteger.h"
#include "SigilMixinContainer.h"
#include "SigilMixinOwnerInterface.h"
#include "UObject/Object.h"
#include "SigilItemInstance.generated.h"

class USigilInventorySystemComponent;
class USigilItemFragment;
struct FSigilItemStack;
class USigilItemCollection;
class USigilItemDefinition;

/**
 * Delegate triggered when fragment data is added, updated, or removed.
 * 当片段数据被添加、更新或移除时触发的委托。
 * @param Fragment The item fragment associated with the event. 与事件关联的道具片段。
 * @param Data The instanced struct containing the fragment data. 包含片段数据的实例化结构体。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSigilItemFragmentStateEventSignature, const USigilItemFragment*, Fragment, const FInstancedStruct&, Data);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSigilItemIntegerAttributeChangedEventSignature, const FGameplayTag&, AttributeTag, int32, OldValue, int32, NewValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSigilItemFloatAttributeChangedEventSignature, const FGameplayTag&, AttributeTag, float, OldValue, float, NewValue);


/**
 * The item instance created via item definition.
 * 通过道具定义创建的道具实例。
 */
UCLASS(BlueprintType, Blueprintable, CollapseCategories)
class SIGILINVENTORY_API USigilItemInstance : public UObject, public IGameplayTagAssetInterface, public ISigilMixinOwnerInterface, public ISigilGameplayTagFloatContainerOwner,
                                                     public ISigilGameplayTagIntegerContainerOwner
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the item instance.
	 * 道具实例的构造函数。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Gets the gameplay tags owned by this item instance.
	 * 获取此道具实例拥有的游戏标签。
	 * @param TagContainer The container to store the tags. 存储标签的容器。
	 */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	/**
	 * Gets the properties that should be replicated for this object.
	 * 获取需要为此对象复制的属性。
	 * @param OutLifetimeProps Array to store the replicated properties. 存储复制属性的数组。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Checks if the item instance supports networking.
	 * 检查道具实例是否支持网络。
	 * @return True if networking is supported, false otherwise. 如果支持网络则返回true，否则返回false。
	 */
	virtual bool IsSupportedForNetworking() const override { return true; };

	/**
	 * Gets the unique ID of the item instance.
	 * 获取道具实例的唯一ID。
	 * @return The item instance's unique ID. 道具实例的唯一ID。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual FGuid GetItemId() const;

	/**
	 * Sets the unique ID of the item instance.
	 * 设置道具实例的唯一ID。
	 * @param NewId The new ID to set. 要设置的新ID。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual void SetItemId(FGuid NewId);

	/**
	 * Checks if the item instance is unique (non-stackable).
	 * 检查道具实例是否唯一（不可堆叠）。
	 * @return True if the item is unique, false otherwise. 如果道具唯一则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual bool IsUnique() const;

	/**
	 * Gets the display name of the item instance.
	 * 获取道具实例的显示名称。
	 * @return The display name of the item. 道具的显示名称。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual FText GetItemName() const;

	/**
	 * Gets the item definition associated with this instance.
	 * 获取与此实例关联的道具定义。
	 * @return The item definition, or nullptr if not set. 道具定义，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	const USigilItemDefinition* GetDefinition() const;

	/**
	 * Sets the item definition for this instance (authority only).
	 * 设置此实例的道具定义（仅限权限）。
	 * @param NewDefinition The new item definition to set. 要设置的新道具定义。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void SetDefinition(const USigilItemDefinition* NewDefinition);

	/**
	 * Gets the description of the item instance.
	 * 获取道具实例的描述。
	 * @return The description of the item. 道具的描述。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual FText GetItemDescription() const;

	/**
	 * Gets the gameplay tags associated with the item instance.
	 * 获取与道具实例关联的游戏标签。
	 * @return The gameplay tag container. 游戏标签容器。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual FGameplayTagContainer GetItemTags() const;

	/**
	 * Gets a fragment from the item definition by its class.
	 * 从道具定义中按类获取片段。
	 * @param FragmentClass The class of the fragment to retrieve. 要检索的片段类。
	 * @return The fragment instance, or nullptr if not found. 片段实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemInstance", meta=(DeterminesOutputType="FragmentClass", DynamicOutputParam="ReturnValue"))
	const USigilItemFragment* GetFragment(TSubclassOf<USigilItemFragment> FragmentClass) const;

	/**
	 * Finds a fragment in the item definition by its class, with validity check.
	 * 在道具定义中按类查找片段，并进行有效性检查。
	 * @param FragmentClass The class of the fragment to find. 要查找的片段类。
	 * @param bValid Output parameter indicating if the fragment was found. 输出参数，指示是否找到片段。
	 * @return The fragment instance, or nullptr if not found. 片段实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|ItemInstance", meta=(DeterminesOutputType="FragmentClass", DynamicOutputParam="ReturnValue", ExpandBoolAsExecs="bValid"))
	const USigilItemFragment* FindFragment(TSubclassOf<USigilItemFragment> FragmentClass, bool& bValid) const;

	/**
	 * Template function to find a fragment by its class.
	 * 按类查找片段的模板函数。
	 * @param ResultClass The class of the fragment to find. 要查找的片段类。
	 * @return The fragment instance cast to the specified class, or nullptr if not found. 转换为指定类的片段实例，如果未找到则返回nullptr。
	 */
	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return static_cast<const ResultClass*>(GetFragment(ResultClass::StaticClass()));
	}

	/**
	 * Checks if the item instance has any attribute with the specified tag.
	 * 检查道具实例是否具有指定标签的任何属性。
	 * @param AttributeTag The tag of the attribute to check. 要检查的属性标签。
	 * @return True if the attribute exists, false otherwise. 如果属性存在则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual bool HasAnyAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Checks if the item instance has a float attribute with the specified tag.
	 * 检查道具实例是否具有指定标签的浮点属性。
	 * @param AttributeTag The tag of the attribute to check. 要检查的属性标签。
	 * @return True if the float attribute exists, false otherwise. 如果浮点属性存在则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual bool HasFloatAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Gets the value of a float attribute.
	 * 获取浮点属性的值。
	 * @param AttributeTag The tag of the attribute to retrieve. 要检索的属性标签。
	 * @return The value of the float attribute, or 0 if not found. 浮点属性的值，如果未找到则返回0。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual float GetFloatAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Sets the value of a float attribute (authority only).
	 * 设置浮点属性的值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to set. 要设置的属性标签。
	 * @param NewValue The new value to set. 要设置的新值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void SetFloatAttribute(FGameplayTag AttributeTag, float NewValue);

	/**
	 * Adds a value to a float attribute (authority only).
	 * 为浮点属性添加值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to modify. 要修改的属性标签。
	 * @param Value The value to add. 要添加的值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void AddFloatAttribute(FGameplayTag AttributeTag, float Value);

	/**
	 * Removes a value from a float attribute (authority only).
	 * 从浮点属性中移除值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to modify. 要修改的属性标签。
	 * @param Value The value to remove. 要移除的值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void RemoveFloatAttribute(FGameplayTag AttributeTag, float Value);

	/**
	 * Checks if the item instance has an integer attribute with the specified tag.
	 * 检查道具实例是否具有指定标签的整数属性。
	 * @param AttributeTag The tag of the attribute to check. 要检查的属性标签。
	 * @return True if the integer attribute exists, false otherwise. 如果整数属性存在则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual bool HasIntegerAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Gets the value of an integer attribute.
	 * 获取整数属性的值。
	 * @param AttributeTag The tag of the attribute to retrieve. 要检索的属性标签。
	 * @return The value of the integer attribute, or 0 if not found. 整数属性的值，如果未找到则返回0。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	virtual int32 GetIntegerAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Sets the value of an integer attribute (authority only).
	 * 设置整数属性的值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to set. 要设置的属性标签。
	 * @param NewValue The new value to set. 要设置的新值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void SetIntegerAttribute(FGameplayTag AttributeTag, int32 NewValue);

	/**
	 * Adds a value to an integer attribute (authority only).
	 * 为整数属性添加值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to modify. 要修改的属性标签。
	 * @param Value The value to add. 要添加的值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void AddIntegerAttribute(FGameplayTag AttributeTag, int32 Value);

	/**
	 * Removes a value from an integer attribute (authority only).
	 * 从整数属性中移除值（仅限权限）。
	 * @param AttributeTag The tag of the attribute to modify. 要修改的属性标签。
	 * @param Value The value to remove. 要移除的值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|ItemInstance")
	virtual void RemoveIntegerAttribute(FGameplayTag AttributeTag, int32 Value);

	/**
	 * Gets the collection where this item belongs to.
	 * 获取此道具的所属集合。
	 * @attention Only available in server side. 只在服务端有效。
	 * @return The owning collection, or nullptr if not set. 所属集合，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemInstance")
	USigilItemCollection* GetOwningCollection() const;

	/**
	 * Gets the inventory that owns this item instance.
	 * 获取拥有此道具实例的库存。
	 * @return The owning inventory, or nullptr if not set. 所属库存，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemInstance")
	USigilInventorySystemComponent* GetOwningInventory() const;

	/**
	 * Assigns a collection to this item instance. server only
	 * 为此道具实例分配集合。仅服务端
	 * @param NewItemCollection The new collection to assign. 要分配的新集合。
	 */
	virtual void AssignCollection(USigilItemCollection* NewItemCollection);

	/**
	 * Unassigns a collection from this item instance. server only
	 * 从此道具实例中取消分配集合。仅服务端
	 * @param ItemCollection The collection to unassign. 要取消分配的集合。
	 */
	virtual void UnassignCollection(USigilItemCollection* ItemCollection);

	/**
	 * Resets the owning collection of this item instance.
	 * 重置此道具实例的所属集合。
	 */
	// void ResetCollection();

	/**
	 * Checks if the item instance is valid.
	 * 检查道具实例是否有效。
	 * @return True if the item instance is valid, false otherwise. 如果道具实例有效则返回true，否则返回false。
	 */
	bool IsItemValid() const;

	/**
	 * Checks if this item instance is stackable equivalent to another.
	 * 检查此道具实例是否与另一个在堆叠上等价。
	 * @param OtherItem The other item instance to compare with. 要比较的其他道具实例。
	 * @return True if the items are stackable equivalent, false otherwise. 如果道具在堆叠上等价则返回true，否则返回false。
	 */
	virtual bool StackableEquivalentTo(const USigilItemInstance* OtherItem) const;

	/**
	 * Checks if this item instance is similar to another.
	 * 检查此道具实例是否与另一个相似。
	 * @param OtherItem The other item instance to compare with. 要比较的其他道具实例。
	 * @return True if the items are similar, false otherwise. 如果道具相似则返回true，否则返回false。
	 */
	virtual bool SimilarTo(const USigilItemInstance* OtherItem) const;

	/**
	 * Static function to check if two item instances are stackable equivalent.
	 * 静态函数，检查两个道具实例是否在堆叠上等价。
	 * @param Lhs The first item instance. 第一个道具实例。
	 * @param Rhs The second item instance. 第二个道具实例。
	 * @return True if the items are stackable equivalent, false otherwise. 如果道具在堆叠上等价则返回true，否则返回false。
	 */
	static bool AreStackableEquivalent(const USigilItemInstance* Lhs, const USigilItemInstance* Rhs);

	/**
	 * Static function to check if two item instances are similar.
	 * 静态函数，检查两个道具实例是否相似。
	 * @param Lhs The first item instance. 第一个道具实例。
	 * @param Rhs The second item instance. 第二个道具实例。
	 * @return True if the items are similar, false otherwise. 如果道具相似则返回true，否则返回false。
	 */
	static bool AreSimilar(const USigilItemInstance* Lhs, const USigilItemInstance* Rhs);

	/**
	 * Called when the item instance is duplicated.
	 * 道具实例被复制时调用。
	 * @param SrcItem The source item instance being duplicated. 被复制的源道具实例。
	 */
	virtual void OnItemDuplicated(const USigilItemInstance* SrcItem);

#pragma region Mixins

	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	const FSigilMixinContainer& GetFragmentStates() const;

	/**
	 * Finds fragment state by its class.
	 * 按类查找片段数据。
	 * @param FragmentClass The class of the fragment to find. 要查找的片段类。
	 * @param OutState The found fragment state (output). 找到的片段状态（输出）。
	 * @return True if the data was found, false otherwise. 如果找到有效数据则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|ItemInstance", meta=(DisplayName="Find Fragment State", ExpandBoolAsExecs="ReturnValue"))
	virtual bool FindFragmentStateByClass(TSubclassOf<USigilItemFragment> FragmentClass, FInstancedStruct& OutState) const;

	/**
	 * Sets fragment data by its class.
	 * 按类设置片段数据。
	 * @param FragmentClass The class of the fragment to set. 要设置的片段类。
	 * @param NewState The fragment state to set. 要设置的片段数据。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|ItemInstance", meta=(DisplayName="Set Fragment State"))
	virtual void SetFragmentStateByClass(TSubclassOf<USigilItemFragment> FragmentClass, UPARAM(ref)
	                                     const FInstancedStruct& NewState);

	/**
	 * Event triggered when fragment data is added to the item instance.
	 * 当片段数据添加到道具实例时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="ItemInstance")
	FSigilItemFragmentStateEventSignature OnFragmentStateAddedEvent;

	/**
	 * Event triggered when fragment data is removed from the item instance.
	 * 当从道具实例移除片段数据时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="ItemInstance")
	FSigilItemFragmentStateEventSignature OnFragmentStateRemovedEvent;

	/**
	 * Event triggered when fragment data is updated in the item instance.
	 * 当道具实例中的片段数据更新时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="ItemInstance")
	FSigilItemFragmentStateEventSignature OnFragmentStateUpdatedEvent;

protected:
	/**
	 * Called when mixin data is added to the item instance.
	 * 当混合数据添加到道具实例时调用。
	 * @param Target The target object for the mixin data. 混合数据的目标对象。
	 * @param Data The instanced struct containing the mixin data. 包含混合数据的实例化结构体。
	 */
	virtual void OnMixinDataAdded(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) override final;

	/**
	 * Called when mixin data is updated in the item instance.
	 * 当道具实例中的混合数据更新时调用。
	 * @param Target The target object for the mixin data. 混合数据的目标对象。
	 * @param Data The instanced struct containing the updated mixin data. 包含更新混合数据的实例化结构体。
	 */
	virtual void OnMixinDataUpdated(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) override final;

	/**
	 * Called when mixin data is removed from the item instance.
	 * 当从道具实例中移除混合数据时调用。
	 * @param Target The target object for the mixin data. 混合数据的目标对象。
	 * @param Data The instanced struct containing the removed mixin data. 包含移除混合数据的实例化结构体。
	 */
	virtual void OnMixinDataRemoved(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) override final;

	/**
	 * Called when fragment data is added to the item instance.
	 * 当片段数据添加到道具实例时调用。
	 * @param Fragment The fragment associated with the data. 与数据关联的片段。
	 * @param Data The instanced struct containing the fragment data. 包含片段数据的实例化结构体。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	void OnFragmentStateAdded(const USigilItemFragment* Fragment, const FInstancedStruct& Data);

	/**
	 * Called when fragment data is updated in the item instance.
	 * 当道具实例中的片段数据更新时调用。
	 * @param Fragment The fragment associated with the data. 与数据关联的片段。
	 * @param Data The instanced struct containing the updated fragment data. 包含更新片段数据的实例化结构体。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	void OnFragmentStateUpdated(const USigilItemFragment* Fragment, const FInstancedStruct& Data);

	/**
	 * Called when fragment data is removed from the item instance.
	 * 当从道具实例移除片段数据时调用。
	 * @param Fragment The fragment associated with the data. 与数据关联的片段。
	 * @param Data The instanced struct containing the removed fragment data. 包含移除片段数据的实例化结构体。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|ItemInstance")
	void OnFragmentStateRemoved(const USigilItemFragment* Fragment, const FInstancedStruct& Data);

#pragma endregion

#pragma region Containers

public:
	/**
	 * Event triggered when a float attribute is changed inside the item instance.
	 * 当道具实例中的浮点型属性变化时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="ItemInstance")
	FSigilItemFloatAttributeChangedEventSignature OnFloatAttributeChangedEvent;

	/**
	 * Event triggered when an integer attribute is changed inside the item instance.
	 * 当道具实例中的整型属性变化时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="ItemInstance")
	FSigilItemFloatAttributeChangedEventSignature OnIntegerAttributeChangedEvent;

	/**
	 * Called when a float attribute is updated.
	 * 浮点型属性更新时调用。
	 * @param Tag The gameplay tag identifying the attribute. 标识属性的游戏标签。
	 * @param OldValue The previous value of the attribute. 属性之前的值。
	 * @param NewValue The new value of the attribute. 属性的新值。
	 */
	virtual void OnTagFloatUpdate(const FGameplayTag& Tag, float OldValue, float NewValue) override final;

	/**
	 * Called when an integer attribute is updated.
	 * 整型属性更新时调用。
	 * @param Tag The gameplay tag identifying the attribute. 标识属性的游戏标签。
	 * @param OldValue The previous value of the attribute. 属性之前的值。
	 * @param NewValue The new value of the attribute. 属性的新值。
	 */
	virtual void OnTagIntegerUpdate(const FGameplayTag& Tag, int32 OldValue, int32 NewValue) override final;

protected:
	/**
	 * Blueprint event triggered when a float attribute changes.
	 * 浮点型属性变化时触发的蓝图事件。
	 * @param Tag The gameplay tag identifying the attribute. 标识属性的游戏标签。
	 * @param OldValue The previous value of the attribute. 属性之前的值。
	 * @param NewValue The new value of the attribute. 属性的新值。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="ItemInstance")
	void OnFloatAttributeChanged(const FGameplayTag& Tag, float OldValue, float NewValue);

	/**
	 * Blueprint event triggered when an integer attribute changes.
	 * 整型属性变化时触发的蓝图事件。
	 * @param Tag The gameplay tag identifying the attribute. 标识属性的游戏标签。
	 * @param OldValue The previous value of the attribute. 属性之前的值。
	 * @param NewValue The new value of the attribute. 属性的新值。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="ItemInstance")
	void OnIntegerAttributeChanged(const FGameplayTag& Tag, int32 OldValue, int32 NewValue);

#pragma endregion

	/**
	 * Unique ID of this item instance, assigned at creation.
	 * 道具实例的唯一ID，在创建时分配。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Category="ItemInstance", Replicated)
	FGuid ItemId;

	/**
	 * The item definition associated with this instance.
	 * 与此实例关联的道具定义。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ItemInstance", Replicated)
	TObjectPtr<const USigilItemDefinition> Definition;

	/**
	 * Container for integer attributes of the item instance.
	 * 道具实例的整数属性容器。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="ItemInstance", SaveGame, meta=(DisplayName="Attributes (Integer)", ShowOnlyInnerProperties))
	FSigilGameplayTagIntegerContainer IntegerAttributes;

	/**
	 * Container for float attributes of the item instance.
	 * 道具实例的浮点属性容器。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="ItemInstance", SaveGame, meta=(DisplayName="Attributes (Float)", ShowOnlyInnerProperties))
	FSigilGameplayTagFloatContainer FloatAttributes;

	/**
	 * Container for each fragment's runtime state.
	 * 每个片段运行时状态的容器。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="ItemInstance", meta=(DisplayName="Fragment States", ShowOnlyInnerProperties))
	FSigilMixinContainer FragmentStates;

private:
#if UE_WITH_IRIS
	/**
	 * Registers replication fragments for networking (Iris-specific).
	 * 为网络注册复制片段（特定于Iris）。
	 * @param Context The fragment registration context. 片段注册上下文。
	 * @param RegistrationFlags The registration flags. 注册标志。
	 */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	/**
	 * The collection that owns this item instance.
	 * 拥有此道具实例的集合。
	 */
	UPROPERTY(Replicated, Transient)
	TObjectPtr<USigilItemCollection> OwningCollection{nullptr};
};
