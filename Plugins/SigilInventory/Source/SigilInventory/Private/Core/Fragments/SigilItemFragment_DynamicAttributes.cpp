// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemFragment_DynamicAttributes.h"

#include "Items/SigilItemInstance.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemFragment_DynamicAttributes)

void USigilItemFragment_DynamicAttributes::OnInstanceCreated(USigilItemInstance* Instance) const
{
	for (int32 i = 0; i < InitialFloatAttributes.Num(); i++)
	{
		if (InitialFloatAttributes[i].Tag.IsValid())
		{
			Instance->SetFloatAttribute(InitialFloatAttributes[i].Tag, InitialFloatAttributes[i].Value);
		}
	}
	for (int32 i = 0; i < InitialIntegerAttributes.Num(); i++)
	{
		if (InitialIntegerAttributes[i].Tag.IsValid())
		{
			Instance->SetIntegerAttribute(InitialIntegerAttributes[i].Tag, InitialIntegerAttributes[i].Value);
		}
	}
}

float USigilItemFragment_DynamicAttributes::GetFloatAttributeDefault(FGameplayTag AttributeTag) const
{
	return FloatAttributeMap.Contains(AttributeTag) ? FloatAttributeMap[AttributeTag] : 0;
}

int32 USigilItemFragment_DynamicAttributes::GetIntegerAttributeDefault(FGameplayTag AttributeTag) const
{
	return IntegerAttributeMap.Contains(AttributeTag) ? IntegerAttributeMap[AttributeTag] : 0;
}

#if WITH_EDITOR
void USigilItemFragment_DynamicAttributes::PreSave(FObjectPreSaveContext SaveContext)
{
	FloatAttributeMap.Empty();
	for (const FSigilGameplayTagFloat& Attribute : InitialFloatAttributes)
	{
		FloatAttributeMap.Add(Attribute.Tag, Attribute.Value);
	}

	IntegerAttributeMap.Empty();
	for (const FSigilGameplayTagInteger& Attribute : InitialIntegerAttributes)
	{
		IntegerAttributeMap.Add(Attribute.Tag, Attribute.Value);
	}

	Super::PreSave(SaveContext);
}
#endif
