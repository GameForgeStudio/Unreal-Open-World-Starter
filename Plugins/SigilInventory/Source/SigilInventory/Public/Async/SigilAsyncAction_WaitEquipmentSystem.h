// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilAsyncAction_Wait.h"
#include "SigilAsyncAction_WaitEquipmentSystem.generated.h"

class USigilEquipmentSystemComponent;

/**
 * Async action to wait for a valid equipment system component on an actor.
 * 在演员上等待有效装备系统组件的异步动作。
 */
UCLASS()
class SIGILINVENTORY_API USigilAsyncAction_WaitEquipmentSystem : public USigilAsyncAction_Wait
{
	GENERATED_BODY()

public:
	/**
	 * Waits for a valid equipment system component on the target actor.
	 * 在目标演员上等待有效的装备系统组件。
	 * @param WorldContext The world context object to get the world reference. 用于获取世界引用的世界上下文对象。
	 * @param TargetActor The target actor to wait for. 要等待的目标演员。
	 * @return The created wait action. 创建的等待动作。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|Async", meta = (WorldContext = "WorldContext", DefaultToSelf="TargetActor", BlueprintInternalUseOnly = "true"))
	static USigilAsyncAction_WaitEquipmentSystem* WaitEquipmentSystem(UObject* WorldContext, AActor* TargetActor);

protected:
	/**
	 * Checks for the presence of a valid equipment system component.
	 * 检查是否存在有效的装备系统组件。
	 */
	virtual void OnExecutionAction() override;
};

/**
 * Async action to wait for a valid and initialized equipment system component on an actor.
 * 在演员上等待有效且已初始化的装备系统组件的异步动作。
 */
UCLASS()
class SIGILINVENTORY_API USigilAsyncAction_WaitEquipmentSystemInitialized : public USigilAsyncAction_WaitEquipmentSystem
{
	GENERATED_BODY()

public:
	/**
	 * Waits for a valid and initialized equipment system component on the target actor.
	 * 在目标演员上等待有效且已初始化的装备系统组件。
	 * @param WorldContext The world context object to get the world reference. 用于获取世界引用的世界上下文对象。
	 * @param TargetActor The target actor to wait for. 要等待的目标演员。
	 * @return The created wait action. 创建的等待动作。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|Async", meta = (WorldContext = "WorldContext", DefaultToSelf="TargetActor", BlueprintInternalUseOnly = "true"))
	static USigilAsyncAction_WaitEquipmentSystem* WaitEquipmentSystemInitialized(UObject* WorldContext, AActor* TargetActor);

protected:
	/**
	 * Checks for the presence and initialization of the equipment system component.
	 * 检查装备系统组件的存在和初始化状态。
	 */
	virtual void OnExecutionAction() override;

	/**
	 * Cleans up resources and event bindings.
	 * 清理资源和事件绑定。
	 */
	virtual void Cleanup() override;

	/**
	 * Called when the equipment system component is initialized.
	 * 装备系统组件初始化时调用。
	 */
	UFUNCTION()
	virtual void OnSystemInitialized();

	/**
	 * Weak reference to the equipment system component being waited for.
	 * 等待的装备系统组件的弱引用。
	 */
	TWeakObjectPtr<USigilEquipmentSystemComponent> EquipmentSystemPtr;
};
