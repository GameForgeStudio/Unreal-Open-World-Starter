// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilCraftingSystemComponent.h"
#include "Engine/World.h"
#include "SigilInventorySystemComponent.h"
#include "SigilCurrencySystemComponent.h"
#include "SigilInventoryFunctionLibrary.h"
#include "SigilInventorySubsystem.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"
#include "SigilItemFragment_CraftingRecipe.h"
#include "Items/SigilItemInstance.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCraftingSystemComponent)

// Sets default values for this component's properties
USigilCraftingSystemComponent::USigilCraftingSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	NumOfSelectedItemsCache = 0;
	// ...
}

bool USigilCraftingSystemComponent::Craft(const USigilItemDefinition* Recipe, USigilInventorySystemComponent* CostInventory, int32 Quantity)
{
	const USigilItemFragment_CraftingRecipe* RecipeFragment = GetRecipeFragment(Recipe);
	if (RecipeFragment == nullptr)
	{
		return false;
	}

	return CraftInternal(Recipe, CostInventory, Quantity);
}

bool USigilCraftingSystemComponent::CanCraft(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity)
{
	return CanCraftInternal(RecipeDefinition, Inventory, Quantity);
}

bool USigilCraftingSystemComponent::RemoveItemIngredients(USigilInventorySystemComponent* Inventory, const TArray<FSigilItemInfo>& ItemIngredients)
{
	for (int32 i = 0; i < ItemIngredients.Num(); i++)
	{
		FSigilItemInfo ItemInfoToRemove;
		ItemInfoToRemove.Item = ItemIngredients[i].Item;
		ItemInfoToRemove.Amount = ItemIngredients[i].Amount;
		if (Inventory->GetDefaultCollection()->RemoveItem(ItemInfoToRemove).Amount == ItemInfoToRemove.Amount) { continue; }
		return false;
	}
	return false;
}

bool USigilCraftingSystemComponent::IsValidRecipe_Implementation(const USigilItemDefinition* RecipeDefinition) const
{
	if (const USigilItemFragment_CraftingRecipe* Fragment = GetRecipeFragment(RecipeDefinition))
	{
		return !Fragment->InputItems.IsEmpty() && !Fragment->OutputItems.IsEmpty();
	}
	return false;
}

bool USigilCraftingSystemComponent::CanCraftInternal(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory,
                                                    int32 Quantity)
{
	if (!IsValidRecipe(RecipeDefinition))
	{
		return false;
	}

	const USigilItemFragment_CraftingRecipe* RecipeFragment = GetRecipeFragment(RecipeDefinition);
	check(RecipeFragment != nullptr);

	if (!SelectItemForIngredients(Inventory, RecipeFragment->InputItems, Quantity))
	{
		return false;
	}

	TArray<FSigilItemInfo> Selected;
	int32 SelectedCount = 0;
	// Has enough items? 有足够的道具？
	if (!CheckIfEnoughItemIngredients(RecipeFragment->InputItems, Quantity, SelectedItemsCache, Selected, SelectedCount))
	{
		return false;
	}

	USigilCurrencySystemComponent* CurrencySystem = Inventory->GetCurrencySystem();
	if (CurrencySystem == nullptr)
	{
		return false;
	}

	// Has enough currencies? 有足够货币？
	if (!CurrencySystem->HasCurrencies(USigilInventoryFunctionLibrary::MultiplyCurrencies(RecipeFragment->InputCurrencies, Quantity)))
	{
		return false;
	}

	return true;
}

bool USigilCraftingSystemComponent::CraftInternal(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity)
{
	const USigilItemFragment_CraftingRecipe* RecipeFragment = GetRecipeFragment(RecipeDefinition);
	if (RecipeFragment == nullptr)
	{
		return false;
	}

	if (!SelectItemForIngredients(Inventory, RecipeFragment->InputItems, Quantity))
	{
		return false;
	}

	if (!CanCraftInternal(RecipeDefinition, Inventory, Quantity))
	{
		return false;
	}

	USigilCurrencySystemComponent* CurrencySystem = Inventory->GetCurrencySystem();

	bool bCurrencyRemoveSuccess = CurrencySystem->RemoveCurrencies(USigilInventoryFunctionLibrary::MultiplyCurrencies(RecipeFragment->InputCurrencies, Quantity));

	bool bItemRemoveSuccess = RemoveItemIngredients(Inventory, SelectedItemsCache);

	if (bCurrencyRemoveSuccess && bItemRemoveSuccess)
	{
		ProduceCraftingOutput(RecipeDefinition, Inventory, Quantity);
		return true;
	}
	return false;
}

void USigilCraftingSystemComponent::ProduceCraftingOutput(const USigilItemDefinition* RecipeDefinition, USigilInventorySystemComponent* Inventory, int32 Quantity)
{
	const USigilItemFragment_CraftingRecipe* RecipeFragment = GetRecipeFragment(RecipeDefinition);
	if (RecipeFragment == nullptr)
	{
		return;
	}

	TArray<FSigilItemDefinitionAmount> ItemAmounts = USigilInventoryFunctionLibrary::MultiplyItemAmounts(RecipeFragment->OutputItems, Quantity);

	for (int32 i = 0; i < ItemAmounts.Num(); i++)
	{
		FSigilItemInfo ItemInfoToAdd;
		ItemInfoToAdd.Item = USigilInventorySubsystem::Get(GetWorld())->CreateItem(Inventory->GetOwner(), ItemAmounts[i].Definition);
		ItemInfoToAdd.Amount = ItemAmounts[i].Amount;
		Inventory->AddItem(ItemInfoToAdd);
	}
}

const USigilItemFragment_CraftingRecipe* USigilCraftingSystemComponent::GetRecipeFragment(const USigilItemDefinition* RecipeDefinition) const
{
	return RecipeDefinition ? RecipeDefinition->FindFragment<USigilItemFragment_CraftingRecipe>() : nullptr;
}

bool USigilCraftingSystemComponent::SelectItemForIngredients(const USigilInventorySystemComponent* Inventory, const TArray<FSigilItemDefinitionAmount>& ItemIngredients, int32 Quantity)
{
	SelectedItemsCache.Empty();
	NumOfSelectedItemsCache = 0;
	for (int32 i = 0; i < ItemIngredients.Num(); i++)
	{
		auto RequiredItem = ItemIngredients[i];
		int32 NeededAmount = RequiredItem.Amount * Quantity;

		TArray<FSigilItemInfo> FilteredItemInfos;
		if (Inventory->GetItemInfosByDefinition(RequiredItem.Definition, FilteredItemInfos))
		{
			for (int32 j = 0; j < FilteredItemInfos.Num(); j++)
			{
				const FSigilItemInfo& itemInfo = FilteredItemInfos[j];

				int32 HaveAmount = itemInfo.Amount;

				bool FoundSelectedItem = false;
				for (int k = 0; k < NumOfSelectedItemsCache; k++)
				{
					const FSigilItemInfo& SelectedItemInfo = SelectedItemsCache[k];
					if (itemInfo.Item != SelectedItemInfo.Item) { continue; }
					if (itemInfo.StackId != SelectedItemInfo.StackId) { continue; }

					FoundSelectedItem = true;

					HaveAmount = FMath::Max(0, HaveAmount - SelectedItemInfo.Amount);
					int32 additionalIgnoreAmount = FMath::Clamp(HaveAmount, 0, NeededAmount);
					//更新已经选择道具信息。
					SelectedItemsCache[k] = FSigilItemInfo(SelectedItemInfo.Amount + additionalIgnoreAmount, SelectedItemInfo);
				}

				int32 SelectedAmount = FMath::Clamp(HaveAmount, 0, NeededAmount);

				//记录已选择道具信息。
				if (FoundSelectedItem == false)
				{
					SelectedItemsCache.SetNum(NumOfSelectedItemsCache + 1);
					SelectedItemsCache[NumOfSelectedItemsCache] = FSigilItemInfo(SelectedAmount, itemInfo);
					NumOfSelectedItemsCache++;
				}

				NeededAmount -= SelectedAmount;
				if (NeededAmount <= 0) { break; }
			}
		}
		if (NeededAmount <= 0) { continue; }
		return false;
	}
	return true;
}

bool USigilCraftingSystemComponent::CheckIfEnoughItemIngredients(const TArray<FSigilItemDefinitionAmount>& ItemIngredients, int32 Quantity, const TArray<FSigilItemInfo>& SelectedItems,
                                                                TArray<FSigilItemInfo>& ItemsToIgnore, int32& NumOfItemsToIgnore)
{
	for (int32 i = 0; i < ItemIngredients.Num(); i++)
	{
		const FSigilItemDefinitionAmount& ingredientAmount = ItemIngredients[i];

		int32 neededAmount = Quantity * ingredientAmount.Amount;

		for (int32 j = 0; j < SelectedItems.Num(); j++)
		{
			const FSigilItemInfo& itemInfo = SelectedItems[j];
			if (ingredientAmount.Definition != itemInfo.Item->GetDefinition()) { continue; }

			int32 haveAmount = itemInfo.Amount;
			bool foundMatch = false;


			int32 selectedAmount = FMath::Clamp(haveAmount, 0, neededAmount);

			if (foundMatch == false)
			{
				ItemsToIgnore.SetNum(NumOfItemsToIgnore + 1);
				ItemsToIgnore[NumOfItemsToIgnore] = FSigilItemInfo(selectedAmount, itemInfo);
				NumOfItemsToIgnore++;
			}

			neededAmount -= selectedAmount;
			if (neededAmount <= 0) { break; }
		}

		if (neededAmount > 0)
		{
			return false;
		}
	}

	return true;
}
