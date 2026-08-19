// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilPickupComponent.h"
#include "SigilInventoryPickupComponent.generated.h"

class USigilItemCollection;

/**
 * Component for picking up an entire inventory into a specified collection.
 * 用于将整个库存拾取到指定集合的组件。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilInventoryPickupComponent : public USigilPickupComponent
{
	GENERATED_BODY()

public:
	/**
	 * Called when the game starts to initialize the component.
	 * 游戏开始时调用以初始化组件。
	 */
	virtual void BeginPlay() override;

	/**
	 * Performs the pickup logic, transferring the inventory to the picker's collection.
	 * 执行拾取逻辑，将库存转移到拾取者的集合。
	 * @param Picker The inventory system component of the actor picking up the inventory. 拾取库存的演员的库存系统组件。
	 * @return True if the pickup was successful, false otherwise. 如果拾取成功则返回true，否则返回false。
	 */
	virtual bool Pickup(USigilInventorySystemComponent* Picker) override;

	/**
	 * Gets the owning inventory system component.
	 * 获取拥有的库存系统组件。
	 * @return The inventory system component, or nullptr if not set. 库存系统组件，如果未设置则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|Pickup")
	USigilInventorySystemComponent* GetOwningInventory() const;

protected:
	/**
	 * Adds the pickup inventory to the specified destination collection.
	 * 将拾取的库存添加到指定的目标集合。
	 * @param DestCollection The destination collection to add the inventory to. 要添加库存的目标集合。
	 * @return True if the addition was successful, false otherwise. 如果添加成功则返回true，否则返回false。
	 */
	bool AddPickupToCollection(USigilItemCollection* DestCollection);

	/**
	 * Specifies the collection in the picker's inventory to add the items to (defaults to Item.Collection.Main if not set).
	 * 指定拾取者库存中要添加道具的集合（如果未设置，默认为Item.Collection.Main）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(Categories="Sigil.Inventory.Collection"))
	FGameplayTag CollectionTag;

	/**
	 * The inventory system component associated with this pickup.
	 * 与此拾取关联的库存系统组件。
	 */
	UPROPERTY()
	TObjectPtr<USigilInventorySystemComponent> Inventory;
};