// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilInventoryFunctionLibrary.h"

#include "SigilItemCollection.h"
#include "Items/SigilItemInstance.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventoryFunctionLibrary)

TArray<FSigilItemDefinitionAmount> USigilInventoryFunctionLibrary::MultiplyItemAmounts(const TArray<FSigilItemDefinitionAmount>& ItemAmounts, int32 Multiplier)
{
	TArray<FSigilItemDefinitionAmount> Results;
	for (int32 i = 0; i < ItemAmounts.Num(); i++)
	{
		Results.Add(FSigilItemDefinitionAmount(ItemAmounts[i].Definition, ItemAmounts[i].Amount * Multiplier));
	}
	return Results;
}

TArray<FSigilCurrencyEntry> USigilInventoryFunctionLibrary::MultiplyCurrencies(const TArray<FSigilCurrencyEntry>& Currencies, float Multiplier)
{
	TArray<FSigilCurrencyEntry> Results;
	for (int32 i = 0; i < Currencies.Num(); i++)
	{
		Results.Add(FSigilCurrencyEntry(Currencies[i].Definition, Currencies[i].Amount * Multiplier));
	}
	return Results;
}

TArray<FSigilItemInfo> USigilInventoryFunctionLibrary::FilterItemInfosByTagQuery(const TArray<FSigilItemInfo>& ItemInfos, const FGameplayTagQuery& Query)
{
	return ItemInfos.FilterByPredicate([&](const FSigilItemInfo& ItemInfo)
	{
		return ItemInfo.Item != nullptr && ItemInfo.Item->GetItemTags().MatchesQuery(Query);
	});
}


TArray<FSigilItemStack> USigilInventoryFunctionLibrary::FilterItemStacksByTagQuery(const TArray<FSigilItemStack>& ItemStacks, const FGameplayTagQuery& TagQuery)
{
	return ItemStacks.FilterByPredicate([TagQuery](const FSigilItemStack& Stack)
	{
		return TagQuery.Matches(Stack.Item->GetItemTags());
	});
}

TArray<FSigilItemStack> USigilInventoryFunctionLibrary::FilterItemStacksByDefinition(const TArray<FSigilItemStack>& ItemStacks, const USigilItemDefinition* Definition)
{
	return ItemStacks.FilterByPredicate([Definition](const FSigilItemStack& Stack)
	{
		return Stack.Item->GetDefinition() == Definition;
	});
}

TArray<FSigilItemStack> USigilInventoryFunctionLibrary::FilterItemStacksByCollectionTags(const TArray<FSigilItemStack>& ItemStacks, const FGameplayTagContainer& CollectionTags)
{
	return ItemStacks.FilterByPredicate([CollectionTags](const FSigilItemStack& Stack)
	{
		return Stack.Collection->GetCollectionTag().MatchesAnyExact(CollectionTags);
	});
}
