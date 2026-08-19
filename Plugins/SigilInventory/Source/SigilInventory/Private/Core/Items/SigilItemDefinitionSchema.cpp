// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Items/SigilItemDefinitionSchema.h"
#include "SigilInventorySystemSettings.h"
#include "Items/SigilItemDefinition.h"
#include "Fragments/SigilItemFragment.h"
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemDefinitionSchema)

bool USigilItemDefinitionSchema::TryValidateItemDefinition(const USigilItemDefinition* Definition, FText& OutError)
{
	if (Definition == nullptr)
	{
		OutError = FText::FromString(TEXT("Item definition is null."));
		return false;
	}

	if (const USigilInventorySystemSettings* Settings = USigilInventorySystemSettings::Get())
	{
		FString AssetPath = Definition->GetPathName();
		const USigilItemDefinitionSchema* Schema = Settings->GetItemDefinitionSchemaForAsset(AssetPath);
		if (Schema)
		{
			return Schema->TryValidate(Definition, OutError);
		}

		// Only treat a missing schema as an error when the project actually configured schemas;
		// projects that don't use schema validation should pass silently.
		if (!Settings->ItemDefinitionSchemaMap.IsEmpty() || Settings->DefaultItemDefinitionSchema.IsValid())
		{
			OutError = FText::FromString(FString::Format(TEXT("No valid schema found for item definition at path: {0}."), {AssetPath}));
			return false;
		}
	}
	return true;
}

void USigilItemDefinitionSchema::TryPreSaveItemDefinition(USigilItemDefinition* Definition, FText& OutError)
{
	if (Definition == nullptr)
	{
		OutError = FText::FromString(TEXT("Item definition is null."));
		return;
	}

	if (const USigilInventorySystemSettings* Settings = USigilInventorySystemSettings::Get())
	{
		FString AssetPath = Definition->GetPathName();
		const USigilItemDefinitionSchema* Schema = Settings->GetItemDefinitionSchemaForAsset(AssetPath);
		if (Schema)
		{
			Schema->TryPreSave(Definition, OutError);
		}
		else if (!Settings->ItemDefinitionSchemaMap.IsEmpty() || Settings->DefaultItemDefinitionSchema.IsValid())
		{
			OutError = FText::FromString(FString::Format(TEXT("No valid schema found for item definition at path: {0}."), {AssetPath}));
		}
	}
}

bool USigilItemDefinitionSchema::TryValidate(const USigilItemDefinition* Definition, FText& OutError) const
{
	if (Definition == nullptr)
	{
		OutError = FText::FromString(TEXT("Item definition is null."));
		return false;
	}

	// Validate parent tag: all ItemTags must be children of RequiredParentTag
	if (RequiredParentTag.IsValid())
	{
		for (const FGameplayTag& Tag : Definition->ItemTags.GetGameplayTagArray())
		{
			if (!Tag.MatchesTag(RequiredParentTag))
			{
				OutError = FText::FromString(FString::Format(TEXT("Tag {0} is not a child of required parent tag {1}."), {Tag.ToString(), RequiredParentTag.ToString()}));
				return false;
			}
		}
	}

	FSigilItemDefinitionValidationEntry FoundEntry;
	bool bFoundEntry = false;
	int32 ValidationEntryIndex = -1;

	// Find matching validation entry
	for (int32 Index = 0; Index < ValidationEntries.Num(); ++Index)
	{
		const FSigilItemDefinitionValidationEntry& Entry = ValidationEntries[Index];
		if (!Entry.ItemTagQuery.IsEmpty() && Definition->ItemTags.MatchesQuery(Entry.ItemTagQuery))
		{
			FoundEntry = Entry;
			bFoundEntry = true;
			ValidationEntryIndex = Index;
			break;
		}
	}

	if (!bFoundEntry)
	{
		return true; // No matching entry
	}

	TMap<TSubclassOf<USigilItemFragment>, int32> FragmentClassMap;
	TArray<TSubclassOf<USigilItemFragment>> FragmentClasses;

	// Build fragment class map and check for duplicate fragment types
	for (const USigilItemFragment* Fragment : Definition->Fragments)
	{
		if (Fragment != nullptr)
		{
			TSubclassOf<USigilItemFragment> FragmentClass = Fragment->GetClass();
			int32& Count = FragmentClassMap.FindOrAdd(FragmentClass);
			Count++;
			if (Count > 1)
			{
				OutError = FText::FromString(FString::Format(TEXT("Duplicate fragment type {0} found in Fragments array for schema {1}."), {FragmentClass->GetName(), GetPathName()}));
				return false;
			}
		}
	}

	FragmentClassMap.GetKeys(FragmentClasses);

	// Validate common required fragments
	for (const TSoftClassPtr<USigilItemFragment>& CommonFragment : CommonRequiredFragments)
	{
		if (UClass* LoadedClass = CommonFragment.LoadSynchronous())
		{
			if (!FragmentClasses.Contains(LoadedClass))
			{
				OutError = FText::FromString(
					FString::Format(TEXT("Missing common required fragment of type {0} for ValidationEntry {1} in schema {2}."), {LoadedClass->GetName(), ValidationEntryIndex, GetPathName()}));
				return false;
			}
		}
	}

	// Validate forbidden fragments, excluding common required fragments and required fragments
	TSet<TSoftClassPtr<USigilItemFragment>> EffectiveRequiredFragments;
	for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : FoundEntry.RequiredFragments)
	{
		if (!CommonRequiredFragments.Contains(RequiredFragment))
		{
			EffectiveRequiredFragments.Add(RequiredFragment);
		}
	}
	for (const TSoftClassPtr<USigilItemFragment>& ForbiddenFragment : FoundEntry.ForbiddenFragments)
	{
		if (UClass* LoadedClass = ForbiddenFragment.LoadSynchronous())
		{
			if (CommonRequiredFragments.Contains(ForbiddenFragment))
			{
				OutError = FText::FromString(FString::Format(
					TEXT("Forbidden fragment {0} in ValidationEntry {1} is a common required fragment in schema {2}."), {LoadedClass->GetName(), ValidationEntryIndex, GetPathName()}));
				return false;
			}
			if (EffectiveRequiredFragments.Contains(ForbiddenFragment))
			{
				OutError = FText::FromString(
					FString::Format(TEXT("Forbidden fragment {0} in ValidationEntry {1} is a required fragment in schema {2}."), {LoadedClass->GetName(), ValidationEntryIndex, GetPathName()}));
				return false;
			}
			if (FragmentClasses.Contains(LoadedClass))
			{
				OutError = FText::FromString(FString::Format(
					TEXT("Forbidden fragment of type {0} found in definition for ValidationEntry {1} in schema {2}."), {LoadedClass->GetName(), ValidationEntryIndex, GetPathName()}));
				return false;
			}
		}
	}

	// Validate required fragments, excluding those already in CommonRequiredFragments
	for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : EffectiveRequiredFragments)
	{
		if (UClass* LoadedClass = RequiredFragment.LoadSynchronous())
		{
			if (!FragmentClasses.Contains(LoadedClass))
			{
				OutError = FText::FromString(
					FString::Format(TEXT("Missing required fragment of type {0} for ValidationEntry {1} in schema {2}."), {LoadedClass->GetName(), ValidationEntryIndex, GetPathName()}));
				return false;
			}
		}
	}

	// Validate required float attributes
	for (const FSigilGameplayTagFloat& RequiredFloat : FoundEntry.RequiredFloatAttributes)
	{
		bool bFound = false;
		for (const FSigilGameplayTagFloat& FloatAttr : Definition->StaticFloatAttributes)
		{
			if (FloatAttr.Tag == RequiredFloat.Tag)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			OutError = FText::FromString(
				FString::Format(TEXT("Missing required float attribute {0} for ValidationEntry {1} in schema {2}."), {RequiredFloat.Tag.ToString(), ValidationEntryIndex, GetPathName()}));
			return false;
		}
	}

	// Validate required integer attributes
	for (const FSigilGameplayTagInteger& RequiredInteger : FoundEntry.RequiredIntegerAttributes)
	{
		bool bFound = false;
		for (const FSigilGameplayTagInteger& IntegerAttr : Definition->StaticIntegerAttributes)
		{
			if (IntegerAttr.Tag == RequiredInteger.Tag)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			OutError = FText::FromString(
				FString::Format(TEXT("Missing required integer attribute {0} for ValidationEntry {1} in schema {2}."), {RequiredInteger.Tag.ToString(), ValidationEntryIndex, GetPathName()}));
			return false;
		}
	}

	// Validate bUnique
	if (FoundEntry.bEnforceUnique && Definition->bUnique != FoundEntry.RequiredUniqueValue)
	{
		OutError = FText::FromString(FString::Format(
			TEXT("bUnique must be {0} for ValidationEntry {1} in schema {2}."), {FoundEntry.RequiredUniqueValue ? TEXT("true") : TEXT("false"), ValidationEntryIndex, GetPathName()}));
		return false;
	}

	return true;
}

void USigilItemDefinitionSchema::TryPreSave(USigilItemDefinition* Definition, FText& OutError) const
{
	if (Definition == nullptr)
	{
		OutError = FText::FromString(TEXT("Item definition is null."));
		return;
	}

	FSigilItemDefinitionValidationEntry FoundEntry;
	bool bFoundEntry = false;

	// Find matching validation entry
	for (const FSigilItemDefinitionValidationEntry& Entry : ValidationEntries)
	{
		if (!Entry.ItemTagQuery.IsEmpty() && Definition->ItemTags.MatchesQuery(Entry.ItemTagQuery))
		{
			FoundEntry = Entry;
			bFoundEntry = true;
			break;
		}
	}

	// Collect required fragments
	TSet<TSoftClassPtr<USigilItemFragment>> RequiredFragmentSet;
	for (const TSoftClassPtr<USigilItemFragment>& CommonFragment : CommonRequiredFragments)
	{
		RequiredFragmentSet.Add(CommonFragment);
	}
	if (bFoundEntry)
	{
		for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : FoundEntry.RequiredFragments)
		{
			if (!CommonRequiredFragments.Contains(RequiredFragment))
			{
				RequiredFragmentSet.Add(RequiredFragment);
			}
		}
	}

	// Collect forbidden fragments
	TSet<TSoftClassPtr<USigilItemFragment>> ForbiddenFragmentSet;
	if (bFoundEntry)
	{
		for (const TSoftClassPtr<USigilItemFragment>& ForbiddenFragment : FoundEntry.ForbiddenFragments)
		{
			if (!CommonRequiredFragments.Contains(ForbiddenFragment) && !RequiredFragmentSet.Contains(ForbiddenFragment))
			{
				ForbiddenFragmentSet.Add(ForbiddenFragment);
			}
		}
	}

	// Build map of existing fragments, keeping only the first instance of each type
	TMap<TSubclassOf<USigilItemFragment>, TObjectPtr<USigilItemFragment>> ExistingFragmentMap;
	for (TObjectPtr<USigilItemFragment> Fragment : Definition->Fragments)
	{
		if (Fragment != nullptr)
		{
			TSubclassOf<USigilItemFragment> FragmentClass = Fragment->GetClass();
			if (!ExistingFragmentMap.Contains(FragmentClass))
			{
				ExistingFragmentMap.Add(FragmentClass, Fragment);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Removed duplicate fragment of type %s from ItemDefinition for schema %s."), *FragmentClass->GetName(), *GetPathName());
			}
		}
	}

	// Remove forbidden fragments
	for (const TSoftClassPtr<USigilItemFragment>& ForbiddenFragment : ForbiddenFragmentSet)
	{
		if (UClass* ForbiddenClass = ForbiddenFragment.LoadSynchronous())
		{
			if (ExistingFragmentMap.Remove(ForbiddenClass))
			{
				UE_LOG(LogTemp, Warning, TEXT("Removed forbidden fragment of type %s from ItemDefinition for schema %s."), *ForbiddenClass->GetName(), *GetPathName());
			}
		}
	}

	// Add missing required fragments
	for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : RequiredFragmentSet)
	{
		if (UClass* FragmentClass = RequiredFragment.LoadSynchronous())
		{
			if (!ExistingFragmentMap.Contains(FragmentClass))
			{
				USigilItemFragment* NewFragment = NewObject<USigilItemFragment>(Definition, FragmentClass);
				if (NewFragment)
				{
					ExistingFragmentMap.Add(FragmentClass, NewFragment);
					UE_LOG(LogTemp, Warning, TEXT("Added missing required fragment of type %s to ItemDefinition for schema %s."), *FragmentClass->GetName(), *GetPathName());
				}
			}
		}
	}

	// Sort fragments according to FragmentOrder
	TArray<TObjectPtr<USigilItemFragment>> NewFragments;
	TArray<TObjectPtr<USigilItemFragment>> NonRequiredFragments;

	// Cache loaded fragment classes to avoid repeated LoadSynchronous calls
	TMap<TSoftClassPtr<USigilItemFragment>, UClass*> FragmentClassCache;
	for (const TSoftClassPtr<USigilItemFragment>& OrderedFragment : FragmentOrder)
	{
		UClass* FragmentClass = FragmentClassCache.FindOrAdd(OrderedFragment, OrderedFragment.LoadSynchronous());
		if (FragmentClass)
		{
			if (TObjectPtr<USigilItemFragment>* ExistingFragment = ExistingFragmentMap.Find(FragmentClass))
			{
				NewFragments.Add(*ExistingFragment);
				ExistingFragmentMap.Remove(FragmentClass);
			}
		}
	}

	// Append remaining non-required fragments
	ExistingFragmentMap.GenerateValueArray(NonRequiredFragments);
	for (const TObjectPtr<USigilItemFragment>& Fragment : NonRequiredFragments)
	{
		if (Fragment && !FragmentOrder.Contains(Fragment->GetClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Fragment type %s is not included in FragmentOrder, appended to end for schema %s."), *Fragment->GetClass()->GetName(), *GetPathName());
		}
	}
	NewFragments.Append(NonRequiredFragments);

	// Update the definition's fragment array
	Definition->Fragments = MoveTemp(NewFragments);

	// Auto-fix required attributes and bUnique if a matching entry was found
	if (bFoundEntry)
	{
		// Auto-fix float attributes
		for (const FSigilGameplayTagFloat& RequiredFloat : FoundEntry.RequiredFloatAttributes)
		{
			bool bFound = false;
			for (FSigilGameplayTagFloat& FloatAttr : Definition->StaticFloatAttributes)
			{
				if (FloatAttr.Tag == RequiredFloat.Tag)
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				Definition->StaticFloatAttributes.Add(RequiredFloat);
				UE_LOG(LogTemp, Warning, TEXT("Added missing required float attribute %s to ItemDefinition for schema %s."), *RequiredFloat.Tag.ToString(), *GetPathName());
			}
		}

		// Auto-fix integer attributes
		for (const FSigilGameplayTagInteger& RequiredInteger : FoundEntry.RequiredIntegerAttributes)
		{
			bool bFound = false;
			for (FSigilGameplayTagInteger& IntegerAttr : Definition->StaticIntegerAttributes)
			{
				if (IntegerAttr.Tag == RequiredInteger.Tag)
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				Definition->StaticIntegerAttributes.Add(RequiredInteger);
				UE_LOG(LogTemp, Warning, TEXT("Added missing required integer attribute %s to ItemDefinition for schema %s."), *RequiredInteger.Tag.ToString(), *GetPathName());
			}
		}

		// Auto-fix bUnique
		if (FoundEntry.bEnforceUnique)
		{
			if (Definition->bUnique != FoundEntry.RequiredUniqueValue)
			{
				Definition->bUnique = FoundEntry.RequiredUniqueValue;
				UE_LOG(LogTemp, Warning, TEXT("Set bUnique to %s for ItemDefinition for schema %s."), FoundEntry.RequiredUniqueValue ? TEXT("true") : TEXT("false"), *GetPathName());
			}
		}
	}

	// Mark the definition as modified
	Definition->Modify();
}

#if WITH_EDITOR
void USigilItemDefinitionSchema::PreSave(FObjectPreSaveContext SaveContext)
{
	// Remove RequiredFragments and ForbiddenFragments that overlap with CommonRequiredFragments
	for (int32 EntryIndex = 0; EntryIndex < ValidationEntries.Num(); ++EntryIndex)
	{
		FSigilItemDefinitionValidationEntry& Entry = ValidationEntries[EntryIndex];
		int32 InitialRequiredCount = Entry.RequiredFragments.Num();
		int32 InitialForbiddenCount = Entry.ForbiddenFragments.Num();

		Entry.RequiredFragments.RemoveAll([&](const TSoftClassPtr<USigilItemFragment>& Fragment)
		{
			if (CommonRequiredFragments.Contains(Fragment))
			{
				if (UClass* LoadedClass = Fragment.LoadSynchronous())
				{
					UE_LOG(LogTemp, Warning, TEXT("Removed fragment %s from ValidationEntry %d RequiredFragments as it is already in CommonRequiredFragments for schema %s."), *LoadedClass->GetName(),
					       EntryIndex, *GetPathName());
				}
				return true;
			}
			return false;
		});

		Entry.ForbiddenFragments.RemoveAll([&](const TSoftClassPtr<USigilItemFragment>& Fragment)
		{
			if (CommonRequiredFragments.Contains(Fragment))
			{
				if (UClass* LoadedClass = Fragment.LoadSynchronous())
				{
					UE_LOG(LogTemp, Warning, TEXT("Removed fragment %s from ValidationEntry %d ForbiddenFragments as it is in CommonRequiredFragments for schema %s."), *LoadedClass->GetName(),
					       EntryIndex, *GetPathName());
				}
				return true;
			}
			return false;
		});

		if (Entry.RequiredFragments.Num() < InitialRequiredCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Removed %d fragment(s) from ValidationEntry %d RequiredFragments for schema %s."), InitialRequiredCount - Entry.RequiredFragments.Num(), EntryIndex,
			       *GetPathName());
		}
		if (Entry.ForbiddenFragments.Num() < InitialForbiddenCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Removed %d fragment(s) from ValidationEntry %d ForbiddenFragments for schema %s."), InitialForbiddenCount - Entry.ForbiddenFragments.Num(), EntryIndex,
			       *GetPathName());
		}
	}

	// Mark the schema as modified if changes were made
	Modify();

	Super::PreSave(SaveContext);
}

EDataValidationResult USigilItemDefinitionSchema::IsDataValid(class FDataValidationContext& Context) const
{
	// Validate that RequiredFragments do not overlap with CommonRequiredFragments
	for (int32 EntryIndex = 0; EntryIndex < ValidationEntries.Num(); ++EntryIndex)
	{
		const FSigilItemDefinitionValidationEntry& Entry = ValidationEntries[EntryIndex];
		for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : Entry.RequiredFragments)
		{
			if (CommonRequiredFragments.Contains(RequiredFragment))
			{
				if (UClass* LoadedClass = RequiredFragment.LoadSynchronous())
				{
					Context.AddWarning(FText::FromString(
						FString::Format(
							TEXT("ValidationEntry {0} contains fragment {1} in RequiredFragments, which is already in CommonRequiredFragments for schema {2}."),
							{EntryIndex, LoadedClass->GetName(), GetPathName()})));
				}
			}
		}

		// Validate that ForbiddenFragments do not overlap with CommonRequiredFragments or RequiredFragments
		TSet<TSoftClassPtr<USigilItemFragment>> EffectiveRequiredFragments;
		for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : Entry.RequiredFragments)
		{
			if (!CommonRequiredFragments.Contains(RequiredFragment))
			{
				EffectiveRequiredFragments.Add(RequiredFragment);
			}
		}
		for (const TSoftClassPtr<USigilItemFragment>& ForbiddenFragment : Entry.ForbiddenFragments)
		{
			if (CommonRequiredFragments.Contains(ForbiddenFragment))
			{
				if (UClass* LoadedClass = ForbiddenFragment.LoadSynchronous())
				{
					Context.AddError(FText::FromString(
						FString::Format(
							TEXT("ValidationEntry {0} contains fragment {1} in ForbiddenFragments, which is in CommonRequiredFragments for schema {2}."),
							{EntryIndex, LoadedClass->GetName(), GetPathName()})));
					return EDataValidationResult::Invalid;
				}
			}
			if (EffectiveRequiredFragments.Contains(ForbiddenFragment))
			{
				if (UClass* LoadedClass = ForbiddenFragment.LoadSynchronous())
				{
					Context.AddError(FText::FromString(
						FString::Format(
							TEXT("ValidationEntry {0} contains fragment {1} in ForbiddenFragments, which is in RequiredFragments for schema {2}."),
							{EntryIndex, LoadedClass->GetName(), GetPathName()})));
					return EDataValidationResult::Invalid;
				}
			}
		}
	}

	// Validate FragmentOrder contains no duplicates
	TSet<TSoftClassPtr<USigilItemFragment>> FragmentOrderSet;
	for (const TSoftClassPtr<USigilItemFragment>& Fragment : FragmentOrder)
	{
		if (FragmentOrderSet.Contains(Fragment))
		{
			if (UClass* LoadedClass = Fragment.LoadSynchronous())
			{
				Context.AddError(FText::FromString(FString::Format(TEXT("FragmentOrder contains duplicate fragment {0} for schema {1}."), {LoadedClass->GetName(), GetPathName()})));
				return EDataValidationResult::Invalid;
			}
		}
		FragmentOrderSet.Add(Fragment);
	}

	// Suggest including all CommonRequiredFragments and RequiredFragments in FragmentOrder
	TSet<TSoftClassPtr<USigilItemFragment>> AllRequiredFragments = TSet<TSoftClassPtr<USigilItemFragment>>(CommonRequiredFragments);
	for (const FSigilItemDefinitionValidationEntry& Entry : ValidationEntries)
	{
		for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : Entry.RequiredFragments)
		{
			if (!CommonRequiredFragments.Contains(RequiredFragment))
			{
				AllRequiredFragments.Add(RequiredFragment);
			}
		}
	}
	for (const TSoftClassPtr<USigilItemFragment>& RequiredFragment : AllRequiredFragments)
	{
		if (!FragmentOrder.Contains(RequiredFragment))
		{
			if (UClass* LoadedClass = RequiredFragment.LoadSynchronous())
			{
				Context.AddWarning(FText::FromString(FString::Format(TEXT("Required fragment {0} is not included in FragmentOrder for schema {1}."), {LoadedClass->GetName(), GetPathName()})));
			}
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
