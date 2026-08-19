// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Items/SigilItemInfo.h"
#include "Items/SigilItemInstance.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemInfo)

FSigilItemInfo FSigilItemInfo::None = FSigilItemInfo();

FSigilItemInfo::FSigilItemInfo()
{
	Item = nullptr;
	Amount = 0;
	ItemCollection = nullptr;
	StackId = FSigilItemStack::InvalidId;
	CollectionTag = FGameplayTag::EmptyTag;
	CollectionId = FSigilItemStack::InvalidId;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection)
{
	Item = InItem;
	Amount = InAmount;
	ItemCollection = InCollection;
	StackId = FSigilItemStack::InvalidId;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection, FGuid InStackId)
{
	Item = InItem;
	Amount = InAmount;
	ItemCollection = InCollection;
	CollectionId = InCollection->GetCollectionId();
	StackId = InStackId;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount)
{
	Item = InItem;
	Amount = InAmount;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, FGuid InCollectionId)
{
	Item = InItem;
	Amount = InAmount;
	CollectionId = InCollectionId;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, FGameplayTag InCollectionTag)
{
	Item = InItem;
	Amount = InAmount;
	CollectionTag = InCollectionTag;
}

FSigilItemInfo::FSigilItemInfo(int32 InAmount, const FSigilItemInfo& OtherInfo)
{
	Amount = InAmount;
	Item = OtherInfo.Item;
	ItemCollection = OtherInfo.ItemCollection;
	StackId = OtherInfo.StackId;
	CollectionId = OtherInfo.CollectionId;
	CollectionTag = OtherInfo.CollectionTag;
}

FSigilItemInfo::FSigilItemInfo(int32 InAmount, int32 InIndex, const FSigilItemInfo& OtherInfo)
{
	Amount = InAmount;
	Index = InIndex;
	Item = OtherInfo.Item;
	ItemCollection = OtherInfo.ItemCollection;
	StackId = OtherInfo.StackId;
	CollectionId = OtherInfo.CollectionId;
	CollectionTag = OtherInfo.CollectionTag;
}

FSigilItemInfo::FSigilItemInfo(USigilItemInstance* InItem, int32 InAmount, const FSigilItemInfo& OtherInfo)
{
	Item = InItem;
	Amount = InAmount;
	ItemCollection = OtherInfo.ItemCollection;
	StackId = OtherInfo.StackId;
	CollectionId = OtherInfo.CollectionId;
}

FSigilItemInfo::FSigilItemInfo(const FSigilItemStack& ItemStack)
{
	Item = ItemStack.Item;
	ItemCollection = ItemStack.Collection;
	Amount = ItemStack.Amount;
	StackId = ItemStack.Id;
}

FString FSigilItemInfo::GetDebugString() const
{
	if (!IsValid())
	{
		return TEXT("Invalid item info");
	}

	return FString::Format(TEXT("{0}({1})"), {Item->GetDefinition()->GetName(), Amount});
}

bool FSigilItemInfo::operator==(const FSigilItemInfo& Other) const
{
	return Other.Amount == Amount && Other.Item == Item && Other.ItemCollection == ItemCollection;
}

bool FSigilItemInfo::IsValid() const
{
	return Item != nullptr && Amount > 0;
}

USigilInventorySystemComponent* FSigilItemInfo::GetInventory() const
{
	return ItemCollection != nullptr ? ItemCollection->GetOwningInventory() : nullptr;
}
