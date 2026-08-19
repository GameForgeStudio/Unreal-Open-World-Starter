// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCoreStructLibrary.h"
#include "SigilCurrencyEntry.h"
#include "SigilItemFragment.h"
#include "SigilItemFragment_CraftingRecipe.generated.h"

/**
 * Item fragment defining a crafting recipe for producing items.
 * 定义用于生产道具的合成配方的道具片段。
 * @details Specifies input items, currencies, and output items for crafting.
 * @细节 指定用于合成的输入道具、货币和输出道具。
 */
UCLASS(DisplayName="Crafting Recipe Settings", Category="BuiltIn")
class SIGILINVENTORY_API USigilItemFragment_CraftingRecipe : public USigilItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * List of required items and their quantities for crafting.
	 * 合成所需的道具及其数量列表。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(TitleProperty="{Definition}->{Amount}"))
	TArray<FSigilItemDefinitionAmount> InputItems;

	/**
	 * List of required currencies and their amounts for crafting.
	 * 合成所需的货币及其数量列表。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ingredients", meta=(TitleProperty="{Tag}->{Amount}"))
	TArray<FSigilCurrencyEntry> InputCurrencies;

	/**
	 * List of items produced by the crafting recipe.
	 * 合成配方产出的道具列表。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(TitleProperty="{DefinitionTag}->{Amount}"))
	TArray<FSigilItemDefinitionAmount> OutputItems;
};
