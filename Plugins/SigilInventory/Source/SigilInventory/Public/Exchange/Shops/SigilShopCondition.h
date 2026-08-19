// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SigilItemInfo.h"
#include "UObject/Interface.h"
#include "SigilShopCondition.generated.h"

class USigilCurrencySystemComponent;
class USigilShopSystemComponent;
class USigilInventorySystemComponent;

// This class does not need to be modified.
/**
 * Interface for defining custom buy condition checks.
 * 定义自定义购买条件检查的接口。
 */
UINTERFACE()
class USigilShopBuyCondition : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors or components to implement custom buy condition logic.
 * 供Actor或组件实现自定义购买条件逻辑的接口。
 * @details Allows checking if an item can be bought based on shop, inventory, and currency conditions.
 * @细节 允许根据商店、库存和货币条件检查道具是否可以购买。
 */
class SIGILINVENTORY_API ISigilShopBuyCondition
{
	GENERATED_BODY()

public:
	/**
	 * Checks if an item can be bought.
	 * 检查道具是否可以购买。
	 * @param Shop The shop system component. 商店系统组件。
	 * @param BuyerInventory The inventory of the buyer. 购买者的库存。
	 * @param CurrencySystem The currency system component. 货币系统组件。
	 * @param ItemInfo Information about the item to buy. 要购买的道具信息。
	 * @return True if the item can be bought, false otherwise. 如果道具可以购买则返回true，否则返回false。
	 */
	virtual bool CanBuy(const USigilShopSystemComponent* Shop, USigilInventorySystemComponent* BuyerInventory, USigilCurrencySystemComponent* CurrencySystem,
	                    FSigilItemInfo ItemInfo) = 0;
};

// This class does not need to be modified.
/**
 * Interface for defining custom sell condition checks.
 * 定义自定义出售条件检查的接口。
 */
UINTERFACE()
class USigilShopSellCondition : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors or components to implement custom sell condition logic.
 * 供Actor或组件实现自定义出售条件逻辑的接口。
 * @details Allows checking if an item can be sold based on shop, inventory, and currency conditions.
 * @细节 允许根据商店、库存和货币条件检查道具是否可以出售。
 */
class SIGILINVENTORY_API ISigilShopSellCondition
{
	GENERATED_BODY()

public:
	/**
	 * Checks if an item can be sold.
	 * 检查道具是否可以出售。
	 * @param Shop The shop system component. 商店系统组件。
	 * @param SellerInventory The inventory of the seller. 出售者的库存。
	 * @param CurrencySystem The currency system component. 货币系统组件。
	 * @param ItemInfo Information about the item to sell. 要出售的道具信息。
	 * @return True if the item can be sold, false otherwise. 如果道具可以出售则返回true，否则返回false。
	 */
	virtual bool CanSell(const USigilShopSystemComponent* Shop, USigilInventorySystemComponent* SellerInventory, USigilCurrencySystemComponent* CurrencySystem,
	                     FSigilItemInfo ItemInfo) = 0;
};