// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Items/SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"
#include "Engine/Texture2D.h"
#include "Items/SigilItemDefinitionSchema.h"
#include "Fragments/SigilItemFragment.h"
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemDefinition)

USigilItemDefinition::USigilItemDefinition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bUnique = false;
}

const USigilItemFragment* USigilItemDefinition::GetFragment(TSubclassOf<USigilItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (USigilItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

bool USigilItemDefinition::HasFloatAttribute(FGameplayTag AttributeTag) const
{
	return FloatAttributeMap.Contains(AttributeTag);
}

bool USigilItemDefinition::HasIntegerAttribute(FGameplayTag AttributeTag) const
{
	return IntegerAttributeMap.Contains(AttributeTag);
}

float USigilItemDefinition::GetFloatAttribute(FGameplayTag AttributeTag) const
{
	return FloatAttributeMap.Contains(AttributeTag) ? FloatAttributeMap[AttributeTag] : 0;
}

int32 USigilItemDefinition::GetIntegerAttribute(FGameplayTag AttributeTag) const
{
	return IntegerAttributeMap.Contains(AttributeTag) ? IntegerAttributeMap[AttributeTag] : 0;
}

const USigilItemFragment* USigilItemDefinition::FindFragmentOfItemDefinition(TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TSubclassOf<USigilItemFragment> FragmentClass)
{
	if (!ItemDefinition.IsNull())
	{
		const USigilItemDefinition* LoadedDefinition = ItemDefinition.LoadSynchronous();
		if (LoadedDefinition != nullptr)
		{
			return LoadedDefinition->GetFragment(FragmentClass);
		}
	}
	return nullptr;
}

#if WITH_EDITOR
void USigilItemDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	FloatAttributeMap.Empty();
	for (const FSigilGameplayTagFloat& Attribute : StaticFloatAttributes)
	{
		FloatAttributeMap.Add(Attribute.Tag, Attribute.Value);
	}

	IntegerAttributeMap.Empty();
	for (const FSigilGameplayTagInteger& Attribute : StaticIntegerAttributes)
	{
		IntegerAttributeMap.Add(Attribute.Tag, Attribute.Value);
	}

	FText SchemaError;
	USigilItemDefinitionSchema::TryPreSaveItemDefinition(this, SchemaError);
	if (!SchemaError.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemDefinition PreSave validation warning for %s: %s"), *GetPathName(), *SchemaError.ToString());
	}

	Super::PreSave(SaveContext);
}

EDataValidationResult USigilItemDefinition::IsDataValid(class FDataValidationContext& Context) const
{
	if (ItemTags.IsEmpty())
	{
		Context.AddWarning(FText::FromString(FString::Format(TEXT("Item tags should not be empty for %s, or it can't be queried by external system."), {GetPathName()})));
	}

	// Check for duplicate fragment types
	TSet<TSubclassOf<USigilItemFragment>> FragmentClassSet;
	for (const USigilItemFragment* Fragment : Fragments)
	{
		if (Fragment != nullptr)
		{
			TSubclassOf<USigilItemFragment> FragmentClass = Fragment->GetClass();
			if (FragmentClassSet.Contains(FragmentClass))
			{
				Context.AddError(FText::FromString(FString::Format(TEXT("Duplicate fragment type {0} found in Fragments array for %s."), {FragmentClass->GetName(), GetPathName()})));
				return EDataValidationResult::Invalid;
			}
			FragmentClassSet.Add(FragmentClass);
		}
	}

	FText SchemaError;
	if (!USigilItemDefinitionSchema::TryValidateItemDefinition(this, SchemaError))
	{
		Context.AddError(SchemaError);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
