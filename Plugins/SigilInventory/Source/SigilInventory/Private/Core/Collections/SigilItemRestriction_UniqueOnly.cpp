// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemRestriction_UniqueOnly.h"

#include "Items/SigilItemInstance.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemRestriction_UniqueOnly)

bool USigilItemRestriction_UniqueOnly::CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const
{
	// this restriction will not modify the item info will be added, so just set it to OutItemInfo
	return ItemInfo.Item->IsUnique();
}
