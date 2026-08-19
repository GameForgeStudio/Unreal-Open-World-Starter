// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemRestriction_StacksNumLimit.h"

#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemRestriction_StacksNumLimit)

bool USigilItemRestriction_StacksNumLimit::CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	int32 ExistingNum = ReceivingCollection->GetItemStacksNum();

	int32 AvailableAdditionalStacks = MaxStacksNum - ExistingNum;

	int32 ItemAmountsThatFit = ReceivingCollection->GetItemAmountFittingInLimitedAdditionalStacks(ItemInfo, AvailableAdditionalStacks);

	if (ItemAmountsThatFit == 0)
	{
		return false;
	}

	ItemInfo = FSigilItemInfo(ItemAmountsThatFit, ItemInfo);
	return true;
}
