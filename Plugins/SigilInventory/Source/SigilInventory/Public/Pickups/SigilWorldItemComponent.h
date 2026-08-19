// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCoreStructLibrary.h"
#include "Items/SigilItemInfo.h"
#include "Components/ActorComponent.h"
#include "SigilWorldItemComponent.generated.h"

class USigilItemInstance;

/**
 * Delegate triggered when an item info is set.
 * 道具信息设置时触发的委托。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilItemInfoSetSignature, const FSigilItemInfo&, ItemInfo);

/**
 * Component for generating or referencing items in the world.
 * 用于在世界中生成或引用道具的组件。
 * @details Used by item pickups to generate item instances or by spawned equipment to hold references to source items.
 * @细节 道具拾取使用此组件生成道具实例，或由生成的装备使用以持有源道具的引用。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilWorldItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the world item component.
	 * 世界道具组件的构造函数。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilWorldItemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Gets the properties that should be replicated for this component.
	 * 获取需要为此组件复制的属性。
	 * @param OutLifetimeProps Array to store the replicated properties. 存储复制属性的数组。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Gets the world item component from an actor.
	 * 从演员获取世界道具组件。
	 * @param Actor The actor to query for the world item component. 要查询世界道具组件的演员。
	 * @return The world item component, or nullptr if not found. 世界道具组件，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|WorldItem", meta=(DefaultToSelf="Actor"))
	static USigilWorldItemComponent* GetWorldItemComponent(const AActor* Actor);

	/**
	 * Creates an item instance from a definition.
	 * 从定义创建道具实例。
	 * @param ItemDefinition The item definition and amount to create the instance from. 用于创建实例的道具定义和数量。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|WorldItem")
	void CreateItemFromDefinition(FSigilItemDefinitionAmount ItemDefinition);

	/**
	 * Checks if the component has a valid item definition.
	 * 检查组件是否具有有效的道具定义。
	 * @return True if the component will auto-create and own an item, false otherwise. 如果组件将自动创建并拥有道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|WorldItem")
	bool HasValidDefinition() const;

	/**
	 * Sets the item info for the component (should be called only once).
	 * 设置组件的道具信息（应仅调用一次）。
	 * @param InItem The item instance to set. 要设置的道具实例。
	 * @param InAmount The amount of the item. 道具数量。
	 */
	virtual void SetItemInfo(USigilItemInstance* InItem, int32 InAmount);

	/**
	 * Resets the item info for the component.
	 * 重置组件的道具信息。
	 */
	void ResetItemInfo();

	/**
	 * Gets the associated item instance.
	 * 获取关联的道具实例。
	 * @return The item instance, or nullptr if not set. 道具实例，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|WorldItem")
	USigilItemInstance* GetItemInstance();

	/**
	 * Creates a duplicated item instance with a new owner.
	 * 创建具有新拥有者的重复道具实例。
	 * @param NewOwner The new owner for the duplicated instance. 重复实例的新拥有者。
	 * @return The duplicated item instance, or nullptr if not possible. 重复的道具实例，如果无法创建则返回nullptr。
	 */
	USigilItemInstance* GetDuplicatedItemInstance(AActor* NewOwner);

	/**
	 * Gets the item info associated with the component.
	 * 获取与组件关联的道具信息。
	 * @return The item info. 道具信息。
	 */
	FSigilItemInfo GetItemInfo() const;

	/**
	 * Gets the amount of the associated item instance.
	 * 获取关联道具实例的数量。
	 * @return The item amount. 道具数量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|WorldItem")
	int32 GetItemAmount() const;

	/**
	 * Called when the game starts to initialize the component.
	 * 游戏开始时调用以初始化组件。
	 */
	virtual void BeginPlay() override;

	/**
	 * Gets the owner cast to a specific type.
	 * 获取特定类型的拥有者。
	 * @return The owner cast to the specified type, or nullptr if the cast fails. 转换为指定类型的拥有者，如果转换失败则返回nullptr。
	 */
	template <class T>
	T* GetTypedOwner()
	{
		return Cast<T>(GetOwner());
	}

	/**
	 * Delegate triggered when a valid item info is set.
	 * 有效道具信息设置时触发的委托。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilItemInfoSetSignature ItemInfoSetEvent;

protected:
	/**
	 * Item definition used to auto-create an item instance (if set, the component owns the instance).
	 * 用于自动创建道具实例的道具定义（如果设置，组件拥有该实例）。
	 */
	UPROPERTY(EditAnywhere, Category="WorldItem")
	FSigilItemDefinitionAmount Definition;

	/**
	 * The item info associated with this component.
	 * 与该组件关联的道具信息。
	 */
	UPROPERTY(VisibleAnywhere, Category="WorldItem", ReplicatedUsing=OnRep_ItemInfo, meta=(ShowInnerProperties))
	FSigilItemInfo ItemInfo;

	/**
	 * Called when the item info is replicated.
	 * 道具信息复制时调用。
	 */
	UFUNCTION()
	void OnRep_ItemInfo();
};
