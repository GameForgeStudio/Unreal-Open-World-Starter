// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilEquipmentStructLibrary.h"
#include "SigilItemFragment.h"
#include "SigilItemFragment_Equippable.generated.h"

class USigilEquipmentInstance;

/**
 * Item fragment for defining equippable item behavior.
 * 定义可装备道具行为的道具片段。
 * @details Configures equipment instance and spawning behavior for equipped items.
 * @细节 配置装备实例和已装备道具的生成行为。
 */
UCLASS(DisplayName="Equippable Settings", Category="BuiltIn")
class SIGILINVENTORY_API USigilItemFragment_Equippable : public USigilItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the equippable item fragment.
	 * 可装备道具片段的构造函数。
	 */
	USigilItemFragment_Equippable();

	/**
	 * Specifies the equipment instance class handling equipment logic.
	 * 指定处理装备逻辑的装备实例类。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment, NoClear, meta = (MustImplement = "/Script/SigilInventory.SigilEquipmentInterface", AllowAbstract = "false"))
	TSoftClassPtr<UObject> InstanceType;

	/**
	 * Indicates if the equipment instance is actor-based, set automatically based on InstanceType.
	 * 指示装备实例是否基于Actor，根据InstanceType自动设置。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment, meta=(EditCondition="false"))
	bool bActorBased{false};

	/**
	 * Determines if the equipment instance is automatically activated upon equipping.
	 * 确定装备实例在装备时是否自动激活。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment)
	bool bAutoActivate{false};

	/**
	 * List of actors to spawn on the pawn when the item is equipped, such as weapons or armor.
	 * 在道具装备时在Pawn上生成的Actor列表，如武器或盔甲。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment, meta=(EditCondition="!bActorBased", EditConditionHides))
	TArray<FSigilEquipmentActorToSpawn> ActorsToSpawn;

#if WITH_EDITOR
	/**
	 * Called before saving the object to perform editor-specific operations.
	 * 在保存对象之前调用以执行编辑器特定的操作。
	 * @param SaveContext The context for the save operation. 保存操作的上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};