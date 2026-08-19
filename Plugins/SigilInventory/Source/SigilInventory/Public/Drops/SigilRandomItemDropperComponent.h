// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemDropperComponent.h"
#include "SigilRandomItemDropperComponent.generated.h"

/**
 * Component for handling random item drop logic.
 * 处理随机道具掉落逻辑的组件。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilRandomItemDropperComponent : public USigilItemDropperComponent
{
	GENERATED_BODY()

protected:
	/**
	 * Retrieves the list of items to drop randomly.
	 * 随机获取要掉落的道具列表。
	 * @return Array of item information for dropped items. 掉落道具的信息数组。
	 */
	virtual TArray<FSigilItemInfo> GetItemsToDropInternal() const override;

	/**
	 * Selects a random item from the provided list based on weights.
	 * 根据权重从提供的列表中随机选择一个道具。
	 * @param ItemInfos List of item information to choose from. 可选择的道具信息列表。
	 * @param Sum Total weight sum for randomization. 用于随机化的总权重和。
	 * @return Reference to the selected item information. 选中的道具信息的引用。
	 */
	const FSigilItemInfo& GetRandomItemInfo(const TArray<FSigilItemInfo>& ItemInfos, int32 Sum) const;

	/**
	 * Determines if items are dropped randomly.
	 * 确定是否随机掉落道具。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(DisplayAfter="bDropAsInventory"))
	bool bRandomDrop{false};

	/**
	 * Minimum number of items to drop when random drop is enabled.
	 * 启用随机掉落时掉落的最小道具数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(EditCondition="bRandomDrop", ClampMin=1))
	int32 MinAmount{1};

	/**
	 * Maximum number of items to drop when random drop is enabled.
	 * 启用随机掉落时掉落的最大道具数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dropper", meta=(EditCondition="bRandomDrop", ClampMin=1))
	int32 MaxAmount{2};
};