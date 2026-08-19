// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SigilEquipmentInterface.generated.h"

class USigilItemInstance;
class Apawn;

/**
 * Interface class for objects that can act as equipment instances (no modifications needed).
 * 可作为装备实例的对象的接口类（无需修改）。
 */
UINTERFACE()
class USigilEquipmentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that wish to function as equipment instances.
 * 希望用作装备实例的对象应实现的接口。
 * @details Any object implementing this interface can be used as an equipment instance in the equipment system.
 * @细节 实现此接口的任何对象都可在装备系统中用作装备实例。
 */
class SIGILINVENTORY_API ISigilEquipmentInterface
{
	GENERATED_BODY()

public:
	/**
	 * Receives the pawn owning this equipment.
	 * 接收拥有此装备的Pawn。
	 * @param NewPawn The pawn with the equipment system component attached. 挂载了装备系统组件的Pawn。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	void ReceiveOwningPawn(APawn* NewPawn);
	virtual void ReceiveOwningPawn_Implementation(APawn* NewPawn);

	/**
	 * Gets the pawn owning this equipment.
	 * 获取拥有此装备的Pawn。
	 * @return The owning pawn, or nullptr if none. 所属Pawn，如果没有则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	APawn* GetOwningPawn() const;
	virtual APawn* GetOwningPawn_Implementation() const;

	/**
	 * Receives the source item from which the equipment was created.
	 * 接收创建此装备的源道具。
	 * @attention The item must be stored and marked as replicated.
	 * @注意 必须存储该道具并标记为网络复制。
	 * @param NewItem The source item instance. 源道具实例。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	void ReceiveSourceItem(USigilItemInstance* NewItem);
	virtual void ReceiveSourceItem_Implementation(USigilItemInstance* NewItem);

	/**
	 * Gets the source item from which the equipment was created.
	 * 获取创建此装备的源道具。
	 * @return The source item instance, or nullptr if none. 源道具实例，如果没有则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	USigilItemInstance* GetSourceItem() const;
	virtual USigilItemInstance* GetSourceItem_Implementation() const;

	/**
	 * Called after the equipment is added to the equipment system's equipment list (deprecated).
	 * 在装备被添加到装备系统的装备列表后调用（已弃用）。
	 * @details Similar to an actor's BeginPlay.
	 * @细节 类似于Actor的BeginPlay。
	 * @attention Deprecated, use OnEquipmentBeginPlay instead.
	 * @注意 已弃用，推荐使用OnEquipmentBeginPlay。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment", meta=(DeprecatedFunction, DeprecationMessage="Use OnEquipment BeginPlay"))
	void OnEquipped();
	virtual void OnEquipped_Implementation();

	/**
	 * Called after the equipment is added to the equipment system's equipment list.
	 * 在装备被添加到装备系统的装备列表后调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	void OnEquipmentBeginPlay();
	virtual void OnEquipmentBeginPlay_Implementation();

	/**
	 * Called before the equipment is removed from the equipment system's equipment list (deprecated).
	 * 在从装备系统的装备列表移除装备之前调用（已弃用）。
	 * @details Similar to an actor's EndPlay.
	 * @细节 类似于Actor的EndPlay。
	 * @attention Deprecated, use OnEquipmentEndPlay instead.
	 * @注意 已弃用，推荐使用OnEquipmentEndPlay。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment", meta=(DeprecatedFunction, DeprecationMessage="Use OnEquipment EndPlay"))
	void OnUnequipped();
	virtual void OnUnequipped_Implementation();

	/**
	 * Called before the equipment is removed from the equipment system's equipment list.
	 * 在从装备系统的装备列表移除装备之前调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	void OnEquipmentEndPlay();
	virtual void OnEquipmentEndPlay_Implementation();

	/**
	 * Responds to changes in the equipment's active state.
	 * 响应装备激活状态的变化。
	 * @param bNewActiveState The new active state of the equipment. 装备的新激活状态。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	void OnActiveStateChanged(bool bNewActiveState);
	virtual void OnActiveStateChanged_Implementation(bool bNewActiveState);

	/**
	 * Checks if the equipment is active.
	 * 检查装备是否处于激活状态。
	 * @return True if the equipment is active, false otherwise. 如果装备处于激活状态则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	bool IsEquipmentActive() const;
	virtual bool IsEquipmentActive_Implementation() const;

	/**
	 * Checks if the equipment instance's replication is managed by the equipment system.
	 * 检查装备实例的复制是否由装备系统管理。
	 * @details By default, the equipment system manages only SigilEquipmentInstance and its subclasses.
	 * @细节 默认情况下，装备系统仅管理SigilEquipmentInstance及其子类。
	 * @attention Do not override this method unless you fully understand its implications.
	 * @注意 除非完全了解其影响，否则不要覆写此方法。
	 * @return True if replication is managed by the equipment system, false otherwise. 如果复制由装备系统管理则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Equipment")
	bool IsReplicationManaged();
	virtual bool IsReplicationManaged_Implementation();
};
