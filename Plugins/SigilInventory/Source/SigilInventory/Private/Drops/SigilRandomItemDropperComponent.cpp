// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Drops/SigilRandomItemDropperComponent.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilRandomItemDropperComponent)

TArray<FSigilItemInfo> USigilRandomItemDropperComponent::GetItemsToDropInternal() const
{
	TArray<FSigilItemInfo> Results;

	USigilInventorySystemComponent* Inventory = USigilInventorySystemComponent::FindInventorySystemComponent(GetOwner());
	if (Inventory == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "requires inventory system component to drop items.")
		return Results;
	}
	USigilItemCollection* Collection = Inventory->GetCollectionByTag(CollectionTag);
	if (Collection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, " inventory missing collection with tag:%s'", *CollectionTag.ToString())
		return Results;
	}

	const TArray<FSigilItemStack>& ItemStacks = Collection->GetAllItemStacks();

	TArray<FSigilItemInfo> ItemInfos;
	ItemInfos.Reserve(ItemStacks.Num());
	int32 ProbabilitySum = 0;

	for (int32 i = 0; i < ItemStacks.Num(); ++i)
	{
		//加权
		ProbabilitySum += ItemStacks[i].Amount;
		ItemInfos[i] = FSigilItemInfo(ProbabilitySum, ItemStacks[i]);
	}

	int32 RandomAmount = FMath::RandRange(MinAmount, MaxAmount + 1);
	Results.Empty();

	for (int i = 0; i < RandomAmount; i++)
	{
		auto& selectedItemInfo = GetRandomItemInfo(ItemInfos, ProbabilitySum);
		bool foundMatch = false;
		//去重，多个栈可能指向同一个道具实例，若发现重复
		for (int j = 0; j < Results.Num(); j++)
		{
			if (Results[j].Item == selectedItemInfo.Item)
			{
				Results[j] = FSigilItemInfo(Results[j].Amount + 1, Results[j]);
				foundMatch = true;
				break;
			}
		}

		if (!foundMatch) { Results.Add(FSigilItemInfo(1, selectedItemInfo)); }
	}

	return Results;
}

const FSigilItemInfo& USigilRandomItemDropperComponent::GetRandomItemInfo(const TArray<FSigilItemInfo>& ItemInfos, int32 ProbabilitySum) const
{
	int32 RandomProbabilityIdx = FMath::RandRange(0, ProbabilitySum);

	int32 min = 0;
	int32 max = ItemInfos.Num() - 1;
	int32 mid = 0;

	while (min <= max)
	{
		mid = (min + max) / 2;
		if (ItemInfos[mid].Amount == RandomProbabilityIdx)
		{
			++mid;
			break;
		}

		if (RandomProbabilityIdx < ItemInfos[mid].Amount
			&& (mid == 0 || RandomProbabilityIdx > ItemInfos[mid - 1].Amount)) { break; }

		if (RandomProbabilityIdx < ItemInfos[mid].Amount) { max = mid - 1; }
		else
		{
			min = mid + 1;
		}
	}

	return ItemInfos[mid];
}
