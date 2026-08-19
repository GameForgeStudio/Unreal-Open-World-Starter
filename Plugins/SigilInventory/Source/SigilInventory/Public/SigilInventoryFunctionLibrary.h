// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCoreStructLibrary.h"
#include "SigilCurrencyEntry.h"
#include "Items/SigilItemInfo.h"
#include "Items/SigilItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SigilInventoryFunctionLibrary.generated.h"

/**
 * Blueprint function library for inventory-related utility functions.
 * 用于库存相关实用功能的蓝图函数库。
 */
UCLASS()
class SIGILINVENTORY_API USigilInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Multiplies the amounts of item definitions by a multiplier.
	 * 将道具定义的数量乘以一个倍数。
	 * @param ItemAmounts The array of item definitions with amounts. 包含数量的道具定义数组。
	 * @param Multiplier The multiplier to apply (default is 1). 要应用的倍数（默认为1）。
	 * @return The modified array of item definitions with multiplied amounts. 修改后的包含乘以倍数的道具定义数组。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	static TArray<FSigilItemDefinitionAmount> MultiplyItemAmounts(const TArray<FSigilItemDefinitionAmount>& ItemAmounts, int32 Multiplier = 1);

	/**
	 * Multiplies the amounts of currencies by a multiplier.
	 * 将货币数量乘以一个倍数。
	 * @param Currencies The array of currency entries. 货币条目数组。
	 * @param Multiplier The multiplier to apply (default is 1). 要应用的倍数（默认为1）。
	 * @return The modified array of currency entries with multiplied amounts. 修改后的包含乘以倍数的货币条目数组。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	static TArray<FSigilCurrencyEntry> MultiplyCurrencies(const TArray<FSigilCurrencyEntry>& Currencies, float Multiplier = 1.0f);

	/**
	 * Filters item infos based on a gameplay tag query.
	 * 根据游戏标签查询过滤道具信息。
	 * @param ItemInfos The array of item infos to filter. 要过滤的道具信息数组。
	 * @param Query The gameplay tag query to apply. 要应用的游戏标签查询。
	 * @return The filtered array of item infos. 过滤后的道具信息数组。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS")
	static TArray<FSigilItemInfo> FilterItemInfosByTagQuery(const TArray<FSigilItemInfo>& ItemInfos, const FGameplayTagQuery& Query);

	/**
	 * Filters item stacks based on a gameplay tag query.
	 * 根据游戏标签查询过滤道具堆栈。
	 * @param ItemStacks The item stacks to filter. 要过滤的道具堆栈。
	 * @param TagQuery The tag query for item tags. 用于道具标签的标签查询。
	 * @return The filtered array of item stacks matching the tag query. 匹配标签查询的过滤后的道具堆栈数组。
	 */
	// UFUNCTION(BlueprintPure, Category = "GIS")
	static TArray<FSigilItemStack> FilterItemStacksByTagQuery(const TArray<FSigilItemStack>& ItemStacks, const FGameplayTagQuery& TagQuery);

	/**
	 * Filters item stacks based on an item definition.
	 * 根据道具定义过滤道具堆栈。
	 * @param ItemStacks The item stacks to filter. 要过滤的道具堆栈。
	 * @param Definition The item definition to filter by. 用于过滤的道具定义。
	 * @return The filtered array of item stacks matching the definition. 匹配定义的过滤后的道具堆栈数组。
	 */
	// UFUNCTION(BlueprintPure, Category = "GIS")
	static TArray<FSigilItemStack> FilterItemStacksByDefinition(const TArray<FSigilItemStack>& ItemStacks, const USigilItemDefinition* Definition);

	/**
	 * Filters item stacks based on collection tags.
	 * 根据集合标签过滤道具堆栈。
	 * @param ItemStacks The item stacks to filter. 要过滤的道具堆栈。
	 * @param CollectionTags The collection tags to search. 要搜索的集合标签。
	 * @return The filtered array of item stacks matching the collection tags. 匹配集合标签的过滤后的道具堆栈数组。
	 */
	// UFUNCTION(BlueprintPure, Category = "GIS")
	static TArray<FSigilItemStack> FilterItemStacksByCollectionTags(const TArray<FSigilItemStack>& ItemStacks, const FGameplayTagContainer& CollectionTags);
};
