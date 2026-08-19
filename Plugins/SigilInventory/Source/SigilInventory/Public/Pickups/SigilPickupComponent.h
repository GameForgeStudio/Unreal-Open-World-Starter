// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SigilPickupComponent.generated.h"

class USigilInventorySystemComponent;

/**
 * Delegate triggered when a pickup action succeeds or fails.
 * 拾取动作成功或失败时触发的委托。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSigilPickupSignature);

/**
 * Base component for handling pickup logic in the inventory system.
 * 库存系统中处理拾取逻辑的基组件。
 * @details Provides the core functionality for picking up items, currencies, or inventories.
 * @细节 提供拾取道具、货币或库存的核心功能。
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SIGILINVENTORY_API USigilPickupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the pickup component.
	 * 拾取组件的构造函数。
	 */
	USigilPickupComponent();

	/**
	 * Performs the pickup logic, typically called during interaction.
	 * 执行拾取逻辑，通常在交互时调用。
	 * @param Picker The inventory system component of the actor performing the pickup. 执行拾取的演员的库存系统组件。
	 * @return True if the pickup was successful, false otherwise. 如果拾取成功则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|Pickup")
	virtual bool Pickup(USigilInventorySystemComponent* Picker);

	/**
	 * Delegate triggered when the pickup action succeeds.
	 * 拾取动作成功时触发的委托。
	 */
	UPROPERTY(BlueprintAssignable, Category="Pickup")
	FSigilPickupSignature OnPickupSuccess;

	/**
	 * Delegate triggered when the pickup action fails.
	 * 拾取动作失败时触发的委托。
	 */
	UPROPERTY(BlueprintAssignable, Category="Pickup")
	FSigilPickupSignature OnPickupFail;

protected:
	/**
	 * Notifies listeners of a successful pickup.
	 * 通知监听者拾取成功。
	 */
	virtual void NotifyPickupSuccess();

	/**
	 * Notifies listeners of a failed pickup.
	 * 通知监听者拾取失败。
	 */
	virtual void NotifyPickupFailed();
};