// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilCraftingStructLibrary.h"
#include "Components/ActorComponent.h"
#include "SigilCraftingSystemComponent.generated.h"

class USigilItemFragment_CraftingRecipe;
class USigilCurrencySystemComponent;
class USigilCraftingRecipe;

/**
 * Actor component for handling crafting functionality in the inventory system.
 * 用于在库存系统中处理制作功能的演员组件。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilCraftingSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the crafting system component.
	 * 制作系统组件的构造函数。
	 */
	USigilCraftingSystemComponent();

	/**
	 * Attempts to craft items using a recipe.
	 * 尝试使用配方制作道具。
	 * @param Recipe The item definition containing recipe data. 包含配方数据的道具定义。
	 * @param Inventory The inventory that pays the crafting cost. 用于支付制作成本的库存。
	 * @param Quantity The multiplier for crafting output. 制作输出的倍率。
	 * @return True if crafting succeeds, false otherwise. 如果制作成功返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="GIS|CraftingSystem")
	bool Craft(const USigilItemDefinition* Recipe, USigilInventorySystemComponent* Inventory, int32 Quantity = 1);

	/**
	 * Checks if the parameters are valid for crafting an item.
	 * 检查参数是否满足制作道具的条件。
	 * @param RecipeDefinition The item definition with recipe data. 包含配方数据的道具定义。
	 * @param Inventory The inventory containing the items. 包含道具的库存。
	 * @param Quantity The amount to craft. 要制作的数量。
	 * @return True if crafting is possible, false otherwise. 如果可以制作则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|CraftingSystem")
	bool CanCraft(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity = 1);

protected:
	/**
	 * Internal function to check if crafting is possible.
	 * 内部函数，检查是否可以制作。
	 * @param RecipeDefinition The item definition with recipe data. 包含配方数据的道具定义。
	 * @param Inventory The inventory containing the items. 包含道具的库存。
	 * @param Quantity The amount to craft. 要制作的数量。
	 * @return True if crafting is possible, false otherwise. 如果可以制作则返回true，否则返回false。
	 */
	virtual bool CanCraftInternal(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity = 1);

	/**
	 * Internal function to perform crafting.
	 * 内部函数，执行制作。
	 * @param RecipeDefinition The item definition with recipe data. 包含配方数据的道具定义。
	 * @param Inventory The inventory that pays the crafting cost. 用于支付制作成本的库存。
	 * @param Quantity The multiplier for crafting output. 制作输出的倍率。
	 * @return True if crafting succeeds, false otherwise. 如果制作成功返回true，否则返回false。
	 */
	virtual bool CraftInternal(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity = 1);

	/**
	 * Produces crafting output for the inventory, adding output items by default.
	 * 为库存生成制作输出，默认将输出道具添加到库存。
	 * @details Override to implement custom logic, such as a crafting queue.
	 * @细节 可覆写以实现自定义逻辑，例如制作队列（延迟获得结果）。
	 * @param RecipeDefinition The item definition with recipe data. 包含配方数据的道具定义。
	 * @param Inventory The inventory to receive the output. 接收输出的库存。
	 * @param Quantity The multiplier for crafting output. 制作输出的倍率。
	 */
	virtual void ProduceCraftingOutput(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity = 1);

	/**
	 * Checks if the recipe definition is valid (contains valid crafting data).
	 * 检查配方定义是否有效（包含有效的制作数据）。
	 * @details Override to apply custom validation rules, such as checking for specific fragments.
	 * @细节 可覆写以应用自定义验证规则，例如检查特定道具片段是否存在。
	 * @param RecipeDefinition The item definition to validate. 要验证的道具定义。
	 * @return True if the recipe is valid, false otherwise. 如果配方有效则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="GIS|CraftingSystem")
	bool IsValidRecipe(const USigilItemDefinition* RecipeDefinition) const;

	/**
	 * Gets the recipe fragment for the default crafting implementation.
	 * 获取默认制作实现所需的配方片段。
	 * @param RecipeDefinition The item definition to retrieve data from. 要从中获取数据的道具定义。
	 * @return The crafting recipe fragment, or nullptr if not found. 制作配方片段，如果未找到则返回nullptr。
	 */
	const USigilItemFragment_CraftingRecipe* GetRecipeFragment(const USigilItemDefinition* RecipeDefinition) const;

	/**
	 * Selects the maximum number of item infos for required item ingredients from the inventory.
	 * 从库存中选择满足道具成分需求的最大道具信息。
	 * @param Inventory The inventory to select items from. 要从中选择道具的库存。
	 * @param ItemIngredients The required item ingredients. 所需的道具成分。
	 * @param Quantity The amount to craft. 要制作的数量。
	 * @return True if sufficient items are selected, false otherwise. 如果选择了足够的道具则返回true，否则返回false。
	 */
	virtual bool SelectItemForIngredients(const USigilInventorySystemComponent* Inventory, const TArray<FSigilItemDefinitionAmount>& ItemIngredients, int32 Quantity);

	/**
	 * Checks if selected items are sufficient for the required item ingredients.
	 * 检查已选道具是否足以满足道具成分需求。
	 * @param ItemIngredients The required item ingredients. 所需的道具成分。
	 * @param Quantity The amount to craft. 要制作的数量。
	 * @param SelectedItems The selected items from the inventory. 从库存中选择的道具。
	 * @param ItemsToIgnore Items to ignore during checking. 检查时要忽略的道具。
	 * @param NumOfItemsToIgnore Number of items to ignore (output). 要忽略的道具数量（输出）。
	 * @return True if the selected items are sufficient, false otherwise. 如果选择的道具足够则返回true，否则返回false。
	 */
	virtual bool CheckIfEnoughItemIngredients(const TArray<FSigilItemDefinitionAmount>& ItemIngredients,
	                                          int32 Quantity, const TArray<FSigilItemInfo>& SelectedItems, TArray<FSigilItemInfo>& ItemsToIgnore, int32& NumOfItemsToIgnore);

	/**
	 * Removes item ingredients from the inventory's default collection one by one.
	 * 从库存的默认集合中逐一移除道具成分。
	 * @param Inventory The inventory to remove items from. 要从中移除道具的库存。
	 * @param ItemIngredients The item ingredients to remove. 要移除的道具成分。
	 * @return True if all items were successfully removed, false otherwise. 如果所有道具移除成功则返回true，否则返回false。
	 */
	virtual bool RemoveItemIngredients(USigilInventorySystemComponent* Inventory, const TArray<FSigilItemInfo>& ItemIngredients);

	/**
	 * Cache for selected items during crafting.
	 * 制作过程中选择的道具缓存。
	 */
	TArray<FSigilItemInfo> SelectedItemsCache;

	/**
	 * Number of selected items in the cache.
	 * 缓存中选择的道具数量。
	 */
	int32 NumOfSelectedItemsCache;
};
