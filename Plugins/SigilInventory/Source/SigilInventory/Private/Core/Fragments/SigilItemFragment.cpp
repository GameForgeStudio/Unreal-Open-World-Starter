// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Fragments/SigilItemFragment.h"

#include "Items/SigilItemStack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemFragment)


bool USigilItemFragment::IsMixinDataSerializable() const
{
	return IsStateSerializable();
}

TObjectPtr<const UScriptStruct> USigilItemFragment::GetCompatibleMixinDataType() const
{
	return GetCompatibleStateType();
}

bool USigilItemFragment::MakeDefaultMixinData(FInstancedStruct& DefaultState) const
{
	return MakeDefaultState(DefaultState);
}

bool USigilItemFragment::MakeDefaultState_Implementation(FInstancedStruct& DefaultState) const
{
	return false;
}

const UScriptStruct* USigilItemFragment::GetCompatibleStateType_Implementation() const
{
	return nullptr;
}

bool USigilItemFragment::IsStateSerializable_Implementation() const
{
	return false;
}
