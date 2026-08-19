// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemRestriction_StackSizeLimit.h"

#include "SigilItemCollection.h"
#include "SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemRestriction_StackSizeLimit)

USigilItemRestriction_StackSizeLimit::USigilItemRestriction_StackSizeLimit()
{
	DefaultStackSizeLimit = 99;
}

bool USigilItemRestriction_StackSizeLimit::CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	int32 MaxStackSize = GetStackSizeLimit(ItemInfo.Item);

	FSigilItemInfo ExistingItemInfoResult;
	if (!ReceivingCollection->GetItemInfo(ItemInfo.Item, ExistingItemInfoResult))
	{
		ItemInfo.Amount = FMath::Min(ItemInfo.Amount, MaxStackSize);
		return true;
	}

	int32 ItemAmountsThatFit = FMath::Min(MaxStackSize - ExistingItemInfoResult.Amount, ItemInfo.Amount);
	ItemAmountsThatFit = FMath::Max(0, ItemAmountsThatFit);

	if (ItemAmountsThatFit == 0)
	{
		return false;
	}

	ItemInfo.Amount = ItemAmountsThatFit;
	return true;
}


int32 USigilItemRestriction_StackSizeLimit::GetStackSizeLimit(const USigilItemInstance* Item) const
{
	if (Item == nullptr)
	{
		return 0;
	}
	if (StackSizeLimitAttributeTag.IsValid() && Item->GetDefinition()->HasIntegerAttribute(StackSizeLimitAttributeTag))
	{
		return Item->GetDefinition()->GetIntegerAttribute(StackSizeLimitAttributeTag);
	}

	return DefaultStackSizeLimit;
}
