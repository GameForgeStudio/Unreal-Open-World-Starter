// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilSerializationStructLibrary.h"
#include "SigilItemFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilSerializationStructLibrary)

bool FSigilItemRecord::operator==(const FSigilItemRecord& Other) const
{
	return ItemId == Other.ItemId && DefinitionAssetPath == Other.DefinitionAssetPath;
}

bool FSigilItemRecord::IsValid() const
{
	return ItemId.IsValid() && !DefinitionAssetPath.IsEmpty();
}

// bool FSigilItemFragmentStateRecord::operator==(const FSigilItemFragmentStateRecord& Other) const
// {
// 	return FragmentClass == Other.FragmentClass;
// }
//
// bool FSigilItemFragmentStateRecord::IsValid() const
// {
// 	return FragmentClass != nullptr && FragmentState.IsValid();
// }

bool FSigilStackRecord::IsValid() const
{
	return ItemId.IsValid() && Id.IsValid() && CollectionId.IsValid();
}

bool FSigilCollectionRecord::IsValid() const
{
	return Id.IsValid() && !DefinitionAssetPath.IsEmpty();
}

FSigilCurrencyRecord::FSigilCurrencyRecord()
{
	Key = NAME_None;
}
