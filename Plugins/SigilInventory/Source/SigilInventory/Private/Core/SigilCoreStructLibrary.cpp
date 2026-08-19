// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilCoreStructLibrary.h"

#include "Items/SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCoreStructLibrary)


FSigilItemDefinitionAmount::FSigilItemDefinitionAmount()
{
	Definition = nullptr;
	Amount = 1;
}

FSigilItemDefinitionAmount::FSigilItemDefinitionAmount(TSoftObjectPtr<USigilItemDefinition> InDefinition, int32 InAmount)
{
	Definition = InDefinition;
	Amount = InAmount;
}

bool FSigilItemSlotDefinition::MatchItem(const USigilItemInstance* Item) const
{
	if (!IsValid(Item))
	{
		return false;
	}

	if (TagQuery.IsEmpty())
	{
		return false;
	}

	if (TagQuery.Matches(Item->GetDefinition()->ItemTags))
	{
		return true;
	}

	return false;
}
