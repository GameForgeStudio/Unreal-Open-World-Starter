// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCurrencyEntry.h"
#include "SigilItemFragment.h"
#include "SigilItemFragment_Shoppable.generated.h"

/**
 * Item fragment for defining shop-related properties of an item.
 * 定义道具商店相关属性的道具片段。
 * @details Specifies buy and sell prices for the item in various currencies.
 * @细节 指定道具在不同货币中的购买和出售价格。
 */
UCLASS(DisplayName="Shoppable Settings", Category="BuiltIn")
class SIGILINVENTORY_API USigilItemFragment_Shoppable : public USigilItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * Initializes shop-related data for the item instance upon creation.
	 * 在道具实例创建时初始化商店相关数据。
	 * @param Instance The item instance to initialize. 要初始化的道具实例。
	 */
	virtual void OnInstanceCreated(USigilItemInstance* Instance) const override;

	/**
	 * List of currencies and amounts required to purchase the item.
	 * 购买道具所需的货币和金额列表。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Shoppable, meta=(TitleProperty="Definition"))
	TArray<FSigilCurrencyEntry> BuyCurrencyAmounts;

	/**
	 * List of currencies and amounts received when selling the item.
	 * 出售道具时获得的货币和金额列表。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Shoppable, meta=(TitleProperty="Definition"))
	TArray<FSigilCurrencyEntry> SellCurrencyAmounts;
};