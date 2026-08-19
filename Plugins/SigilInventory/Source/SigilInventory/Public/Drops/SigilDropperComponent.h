// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SigilDropperComponent.generated.h"

/**
 * Abstract base class for a component that handles dropping items in the game world.
 * 用于在游戏世界中处理道具掉落的抽象基类组件。
 */
UCLASS(Abstract, ClassGroup=(GIS))
class SIGILINVENTORY_API USigilDropperComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the dropper component, sets default values for properties.
	 * 掉落组件的构造函数，设置属性的默认值。
	 */
	USigilDropperComponent();

	/**
	 * Drops an item in the game world (authority only).
	 * 在游戏世界中掉落道具（仅限权限）。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|Dropper")
	virtual void Drop();

protected:
	/**
	 * Creates an instance of the pickup actor using the specified PickupActorClass.
	 * 使用指定的PickupActorClass创建拾取Actor的实例。
	 * @return The spawned pickup actor instance, or nullptr if creation fails. 创建的拾取Actor实例，如果创建失败则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Dropper")
	AActor* CreatePickupActorInstance();

	/**
	 * Calculates the origin point for dropping the item.
	 * 计算道具掉落的原点位置。
	 * @return The calculated drop origin as a vector. 掉落原点的向量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Dropper")
	FVector CalcDropOrigin() const;

	/**
	 * Calculates a random offset to apply to the drop location.
	 * 计算应用于掉落位置的随机偏移量。
	 * @return The random offset vector. 随机偏移量向量。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|Dropper")
	FVector CalcDropOffset() const;

	/**
	 * The class of the actor to spawn as the dropped item.
	 * 作为掉落物生成的Actor类。
	 * @attention Must implement the SigilPickupActorInterface.
	 * @注意 必须实现SigilPickupActorInterface接口。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(ExposeOnSpawn, MustImplement="/Script/SigilInventory.SigilPickupActorInterface"))
	TSoftClassPtr<AActor> PickupActorClass;

	/**
	 * Optional actor whose transform is used as the drop origin.
	 * 可选的Actor，其变换用作掉落原点。
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Dropper")
	TObjectPtr<AActor> DropTransform{nullptr};

	/**
	 * The radius within which the item can be dropped.
	 * 道具掉落的半径。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(ExposeOnSpawn))
	float DropRadius = 50.f;
};
