// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemMultiStackCollection.h"

#include "SigilInventorySystemComponent.h"
#include "SigilInventoryTags.h"
#include "SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemMultiStackCollection)

USigilItemMultiStackCollectionDefinition::USigilItemMultiStackCollectionDefinition()
{
	StackSizeLimitAttribute = SigilAttributeTags::StackSizeLimit;
}

TSubclassOf<USigilItemCollection> USigilItemMultiStackCollectionDefinition::GetCollectionInstanceClass() const
{
	return USigilItemMultiStackCollection::StaticClass();
}

USigilItemMultiStackCollection::USigilItemMultiStackCollection(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
}

bool USigilItemMultiStackCollection::GetItemInfo(const USigilItemInstance* Item, FSigilItemInfo& OutItemInfo) const
{
	if (Item == nullptr)
	{
		return false;
	}

	bool bFoundSimilar = false;
	FSigilItemInfo SimilarItemInfo;

	for (int32 i = Container.Stacks.Num() - 1; i >= 0; i--)
	{
		const FSigilItemStack& ItemStack = Container.Stacks[i];

		if (!ItemStack.IsValidStack())
		{
			continue;
		}

		if (!Item->IsUnique() && ItemStack.Item->GetDefinition() == Item->GetDefinition())
		{
			SimilarItemInfo = FSigilItemInfo(ItemStack);
			bFoundSimilar = true;
		}
		if (Container.Stacks[i].Item->GetItemId() != Item->GetItemId())
		{
			continue;
		}

		OutItemInfo = FSigilItemInfo(Container.Stacks[i]);
		return true;
	}

	if (bFoundSimilar)
	{
		OutItemInfo = SimilarItemInfo;
	}
	return bFoundSimilar;
}

int32 USigilItemMultiStackCollection::GetItemAmountFittingInLimitedAdditionalStacks(const FSigilItemInfo& ItemInfo, int32 AvailableAdditionalStacks) const
{
	int32 AmountToAdd = ItemInfo.Amount;

	int32 MaxStackSize = GetMaxStackSize(ItemInfo.Item);

	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		const FSigilItemStack& itemStack = Container.Stacks[i];
		if (CanItemStack(ItemInfo, itemStack) == false) { continue; }

		if (itemStack.Amount == MaxStackSize) { continue; }

		int32 TotalToSet = itemStack.Amount + AmountToAdd;
		int32 SizeDifference = TotalToSet - MaxStackSize;

		if (SizeDifference <= 0)
		{
			AmountToAdd = 0;
			break;
		}

		AmountToAdd = SizeDifference;
	}

	int32 StacksToAdd = AmountToAdd / MaxStackSize;
	int32 RemainderStack = AmountToAdd % MaxStackSize;


	if (AvailableAdditionalStacks > StacksToAdd)
	{
		return ItemInfo.Amount;
	}
	if (AvailableAdditionalStacks == StacksToAdd)
	{
		return ItemInfo.Amount - RemainderStack;
	}

	return ItemInfo.Amount - RemainderStack - MaxStackSize * (StacksToAdd - AvailableAdditionalStacks);
}

FSigilItemInfo USigilItemMultiStackCollection::AddInternal(const FSigilItemInfo& ItemInfo)
{
	// total amounts of item need to add.
	int32 AmountToAdd = ItemInfo.Amount;
	int32 MaxStackSize = GetMaxStackSize(ItemInfo.Item);

	// temp struct to record added stack.
	FSigilItemStack AddedItemStack;

	{
		//try adding item amount on top of the existing stack.  尝试在指定栈上新增数量。
		int32 TargetStackIdx = Container.IndexOfById(ItemInfo.StackId);
		if (TargetStackIdx != INDEX_NONE)
		{
			check(Container.Stacks.IsValidIndex(TargetStackIdx))
			const FSigilItemStack& TargetStack = Container.Stacks[TargetStackIdx];

			//Make sure the target stack is valid and can be stacked with new item. 确保指定栈有效且可与请求的道具信息堆叠。
			if (TargetStack.IsValidStack() && CanItemStack(ItemInfo, TargetStack))
			{
				AddedItemStack = TargetStack;
				IncreaseStackAmount(TargetStackIdx, MaxStackSize, AmountToAdd);
			}
		}
	}

	//尝试在已经存在的兼容栈上新增数量。
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		const FSigilItemStack& ItemStack = Container.Stacks[i];
		if (CanItemStack(ItemInfo, ItemStack) == false)
		{
			continue;
		}
		AddedItemStack = ItemStack;

		int32 AmountAdded = IncreaseStackAmount(i, MaxStackSize, AmountToAdd);
	}

	/**
	 * 30/10=3,30%10=0  need 3 stacks.
	 * 25/10=2,25%10=1  need 3 stacks. 
	 * 77/21=3,77%21=1  need 4 stacks. (4*21 = 84) > 77
	 */
	int32 StacksToAdd = AmountToAdd / MaxStackSize;
	int32 RemainderStack = AmountToAdd % MaxStackSize;

	for (int32 i = 0; i < StacksToAdd; i++)
	{
		FSigilItemStack NewItemStack;
		NewItemStack.Initialize(FGuid::NewGuid(), ItemInfo.Item, MaxStackSize, this);
		AddedItemStack = NewItemStack;
		AddItemStack(NewItemStack);
	}

	if (RemainderStack != 0)
	{
		FSigilItemStack NewItemStack;
		NewItemStack.Initialize(FGuid::NewGuid(), ItemInfo.Item, RemainderStack, this);
		AddedItemStack = NewItemStack;
		AddItemStack(NewItemStack);
	}

	return FSigilItemInfo(ItemInfo.Item, AddedItemStack.Amount, this);
}

int32 USigilItemMultiStackCollection::GetMaxStackSize(USigilItemInstance* Item) const
{
	const USigilItemMultiStackCollectionDefinition* MyDefinition = CastChecked<USigilItemMultiStackCollectionDefinition>(Definition);

	if (!Item->GetDefinition()->HasIntegerAttribute(MyDefinition->StackSizeLimitAttribute))
	{
		return MyDefinition->DefaultStackSizeLimit;
	}
	return Item->GetDefinition()->GetIntegerAttribute(MyDefinition->StackSizeLimitAttribute);
}

/**
 * 案例: 每个栈最多放10个，现在有35个苹果，占用4个栈，这两个栈的ID分别是A，B，C，D。它们的分布是[(A:10),(B:10)(C:10)(D:5)]
 * 假设传入的ItemInfo是栈A里删除11个苹果：那么先从栈A删除10个，这时候栈A空了，还需要在栈B里移除1个。最后的栈分布是[(B:9),(C:10),(D:5)]
 * 假设传入的ItemInfo是栈A里删除34个苹果：那么先从栈ABC都删除10个，ABC清空，然后剩下栈D里删除4个，最后的栈分布是[(D:1)]
 */
FSigilItemInfo USigilItemMultiStackCollection::RemoveInternal(const FSigilItemInfo& ItemInfo)
{
	int32 AlreadyRemoved = 0;
	int32 AmountToRemove = ItemInfo.Amount;
	//上一次移除的StackIndex
	int32 PreviousStackIndexWithSameItem = INDEX_NONE;
	int32 MaxStackSize = GetMaxStackSize(ItemInfo.Item);
	FSigilItemStack RemovedItemStack;

	{
		// Try remove from existing stack first.
		int32 StackIdx = Container.IndexOfById(ItemInfo.StackId);
		if (StackIdx != INDEX_NONE)
		{
			check(Container.Stacks.IsValidIndex(StackIdx))
			RemovedItemStack = Container.Stacks[StackIdx];
			PreviousStackIndexWithSameItem = RemoveItemFromStack(StackIdx, PreviousStackIndexWithSameItem, MaxStackSize, AmountToRemove, AlreadyRemoved);
		}
	}

	// 继续从其他栈中移除这个道具
	TArray<FSigilItemStack> TempStacks = Container.Stacks;
	for (int i = TempStacks.Num() - 1; i >= 0; i--)
	{
		if (AmountToRemove <= 0) { break; }

		if (TempStacks[i].Item == nullptr || TempStacks[i].Item->GetItemId() != ItemInfo.Item->GetItemId()) { continue; }
		//忽略前面移除的Stack.
		if (RemovedItemStack == TempStacks[i]) { continue; }

		RemovedItemStack = TempStacks[i];

		PreviousStackIndexWithSameItem = RemoveItemFromStack(i, PreviousStackIndexWithSameItem, MaxStackSize, AmountToRemove, AlreadyRemoved);
	}

	//No any stacks contain this item instance; 无任何包含此道具实例的道具栈，因此可以从该集合完全移除。
	if (PreviousStackIndexWithSameItem == INDEX_NONE)
	{
		ItemInfo.Item->UnassignCollection(this);
		if (OwningInventory->IsReplicatedSubObjectRegistered(ItemInfo.Item))
		{
			OwningInventory->RemoveReplicatedSubObject(ItemInfo.Item);
		}
	}

	if (AlreadyRemoved == 0)
	{
		return FSigilItemInfo(ItemInfo.Item, AlreadyRemoved, this);
	}
	return FSigilItemInfo(ItemInfo.Item, AlreadyRemoved, this);
}

int32 USigilItemMultiStackCollection::RemoveItemFromStack(int32 Index, int32 PrevStackIndexWithSameItem, int32 MaxStackSize, int32& AmountToRemove, int32& AlreadyRemoved)
{
	check(Container.Stacks.IsValidIndex(Index));

	int32 AmountInStack = Container.Stacks[Index].Amount;

	if (PrevStackIndexWithSameItem != INDEX_NONE)
	{
		check(Container.Stacks.IsValidIndex(PrevStackIndexWithSameItem))

		int32 MergedAmount = AmountInStack + Container.Stacks[PrevStackIndexWithSameItem].Amount;

		if (MergedAmount > MaxStackSize) //假设每个栈最大100，当前的栈有70，之前的栈有60，即一共130，就超了30.
		{
			UpdateItemStackAmountAtIndex(PrevStackIndexWithSameItem, MaxStackSize); //让之前的栈满，即60+40=100；
			AmountInStack = MergedAmount - MaxStackSize; //再让当前栈变成 130-100=30(即少了70-40=30)
		}
		else //the total size of 2 stacks doesn't reach max stack size. 假设每个栈最大100，当前的栈有70，之前的栈有10，即一共80，没有超过100.
		{
			// merge current stack's amount into prev stack. //让之前栈为10+70=80.
			UpdateItemStackAmountAtIndex(PrevStackIndexWithSameItem, MergedAmount);
			// and empty this one. 当前栈归0.
			AmountInStack = 0;
		}
	}

	if (AmountToRemove == 0) { return PrevStackIndexWithSameItem; }

	int32 NewAmount = AmountInStack - AmountToRemove;
	if (NewAmount <= 0)
	{
		AmountToRemove = -NewAmount;
		AlreadyRemoved += AmountInStack;

		//Item can be stored within multiple stacks, we don't need to remove it from this collection now.  Item在此集合可以属于多个栈，不在这里移除。
		RemoveItemStackAtIndex(Index, false);
	}
	else
	{
		AlreadyRemoved += AmountToRemove;
		AmountToRemove = 0;
		UpdateItemStackAmountAtIndex(Index, NewAmount);
		PrevStackIndexWithSameItem = Index;
	}
	return PrevStackIndexWithSameItem;
}

/**
 * 代入: Stack(item,70);MaxStackSize(100),AmountToAdd(50) 结果：Stack(item,100), AmountToAdd(20), 返回实际添加30
 * 代入: Stack(item,40);MaxStackSize(100),AmountToAdd(50) 结果：Stack(item,90), AmountToAdd(0), 返回实际添加50
 */
int32 USigilItemMultiStackCollection::IncreaseStackAmount(int32 StackIdx, int32 MaxStackSize, int32& AmountToAdd)
{
	check(Container.Stacks.IsValidIndex(StackIdx))

	const FSigilItemStack& ItemStack = Container.Stacks[StackIdx];
	if (ItemStack.Amount == MaxStackSize)
	{
		return 0;
	}
	int32 OriginAmountToAdd = AmountToAdd;
	int32 NewAmount = ItemStack.Amount + AmountToAdd;

	int32 OverflowedAmount = NewAmount - MaxStackSize; // <=0 no overflow >0 means overflow.

	// This stack can hold the new amount. 这个栈装得下。
	if (OverflowedAmount <= 0)
	{
		AmountToAdd = 0; //all amounts have been added. 无需再添加。
	}
	else //This stack overflow. 这个栈溢出了。
	{
		NewAmount = MaxStackSize;
		//Still need to add overflowed amount.  超过最大尺寸后的剩余要增加的数量
		AmountToAdd = OverflowedAmount;
	}

	UpdateItemStackAmountAtIndex(StackIdx, NewAmount);
	//实际增加的数量
	return OriginAmountToAdd - AmountToAdd;
}
