// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilPickupComponent.h"
#include "SigilItemPickupComponent.generated.h"

class USigilInventorySystemComponent;
class USigilWorldItemComponent;
class USigilItemCollection;

/**
 * Component for picking up a single item, requiring a WorldItemComponent on the same actor.
 * 用于拾取单个道具的组件，需要与SigilWorldItemComponent共存于同一演员。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilItemPickupComponent : public USigilPickupComponent
{
	GENERATED_BODY()

public:
	/**
	 * Performs the pickup logic, adding the item to the picker's inventory.
	 * 执行拾取逻辑，将道具添加到拾取者的库存。
	 * @param Picker The inventory system component of the actor performing the pickup. 执行拾取的演员的库存系统组件。
	 * @return True if the pickup was successful, false otherwise. 如果拾取成功则返回true，否则返回false。
	 */
	virtual bool Pickup(USigilInventorySystemComponent* Picker) override;

	/**
	 * Gets the associated world item component.
	 * 获取关联的世界道具组件。
	 * @return The world item component, or nullptr if not found. 世界道具组件，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|Pickup")
	USigilWorldItemComponent* GetWorldItem() const;

protected:
	/**
	 * Specifies the collection in the picker's inventory to add the item to (defaults to Item.Collection.Main if not set).
	 * 指定拾取者库存中要添加道具的集合（如果未设置，默认为Item.Collection.Main）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(Categories="Sigil.Inventory.Collection"))
	FGameplayTag CollectionTag;

	/**
	 * If true, the pickup fails if the full amount cannot be added to the inventory.
	 * 如果为true，当无法将全部数量添加到库存集合时，拾取失败。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	bool bFailIfFullAmountNotFit = false;

	/**
	 * Called when the game starts to initialize the component.
	 * 游戏开始时调用以初始化组件。
	 */
	virtual void BeginPlay() override;

	/**
	 * Attempts to add the item to the picker's inventory collection.
	 * 尝试将道具添加到拾取者的库存集合。
	 * @param Picker The inventory system component of the actor performing the pickup. 执行拾取的演员的库存系统组件。
	 * @return True if the addition was successful, false otherwise. 如果添加成功则返回true，否则返回false。
	 */
	bool TryAddToCollection(USigilInventorySystemComponent* Picker);

	/**
	 * The associated world item component.
	 * 关联的世界道具组件。
	 */
	UPROPERTY()
	TObjectPtr<USigilWorldItemComponent> WorldItemComponent;
};