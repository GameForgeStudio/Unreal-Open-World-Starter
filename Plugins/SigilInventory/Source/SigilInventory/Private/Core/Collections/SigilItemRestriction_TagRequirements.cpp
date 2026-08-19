// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemRestriction_TagRequirements.h"

#include "Items/SigilItemInstance.h"
#include "SigilItemCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemRestriction_TagRequirements)

bool USigilItemRestriction_TagRequirements::CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	if (TagQuery.IsEmpty())
	{
		return true;
	}
	return TagQuery.Matches(ItemInfo.Item->GetItemTags());
}
