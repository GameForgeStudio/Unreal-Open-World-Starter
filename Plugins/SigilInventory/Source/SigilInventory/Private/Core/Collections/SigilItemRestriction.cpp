// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemRestriction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemRestriction)

bool USigilItemRestriction::CanAddItem(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	return CanAddItemInternal(ItemInfo, ReceivingCollection);
}

bool USigilItemRestriction::CanRemoveItem(FSigilItemInfo& ItemInfo) const
{
	return CanRemoveItemInternal(ItemInfo);
}

bool USigilItemRestriction::CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	return true;
}

bool USigilItemRestriction::CanRemoveItemInternal_Implementation(FSigilItemInfo& ItemInfo) const
{
	return true;
}
