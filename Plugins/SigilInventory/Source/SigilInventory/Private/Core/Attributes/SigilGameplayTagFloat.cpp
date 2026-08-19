// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Attributes/SigilGameplayTagFloat.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilGameplayTagFloat)


FString FSigilGameplayTagFloat::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%f"), *Tag.ToString(), Value);
}

void FSigilGameplayTagFloatContainer::AddItem(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddItem"), ELogVerbosity::Warning);
		return;
	}

	if (Value > 0)
	{
		for (FSigilGameplayTagFloat& Item : Items)
		{
			// handle adding to existing value.
			if (Item.Tag == Tag)
			{
				const float OldValue = Item.Value;
				const float NewValue = Item.Value + Value;
				Item.Value = NewValue;
				TagToValueMap[Tag] = NewValue;
				MarkItemDirty(Item);
				if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
				{
					Interface->OnTagFloatUpdate(Tag, OldValue, NewValue);
				}
				return;
			}
		}

		// handle adding new item.
		FSigilGameplayTagFloat& NewItem = Items.Emplace_GetRef(Tag, Value);
		TagToValueMap.Add(Tag, Value);
		MarkItemDirty(NewItem);
		if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
		{
			Interface->OnTagFloatUpdate(Tag, 0, Value);
		}
	}
}

void FSigilGameplayTagFloatContainer::SetItem(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to SetItem"), ELogVerbosity::Warning);
		return;
	}
	for (FSigilGameplayTagFloat& Item : Items)
	{
		if (Item.Tag == Tag)
		{
			const float OldValue = Item.Value;
			Item.Value = Value;
			TagToValueMap[Tag] = Value;
			MarkItemDirty(Item);
			if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
			{
				Interface->OnTagFloatUpdate(Tag, OldValue, Value);
			}
			return;
		}
	}

	FSigilGameplayTagFloat& NewItem = Items.Emplace_GetRef(Tag, Value);
	MarkItemDirty(NewItem);
	TagToValueMap.Add(Tag, Value);
	if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
	{
		Interface->OnTagFloatUpdate(Tag, 0, Value);
	}
}

void FSigilGameplayTagFloatContainer::SetItems(const TArray<FSigilGameplayTagFloat>& NewItems)
{
	Items = NewItems;
	TagToValueMap.Empty();
	for (const FSigilGameplayTagFloat& NewItem : NewItems)
	{
		TagToValueMap.Add(NewItem.Tag, NewItem.Value);
	}
	MarkArrayDirty();
}

void FSigilGameplayTagFloatContainer::EmptyItems()
{
	Items.Empty();
	TagToValueMap.Empty();
	MarkArrayDirty();
}

void FSigilGameplayTagFloatContainer::RemoveItem(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveItem"), ELogVerbosity::Warning);
		return;
	}

	//@TODO: Should we error if you try to remove a Item that doesn't exist or has a smaller count?
	if (Value > 0)
	{
		for (auto It = Items.CreateIterator(); It; ++It)
		{
			FSigilGameplayTagFloat& Item = *It;
			if (Item.Tag == Tag)
			{
				if (Item.Value <= Value)
				{
					const float OldValue = Item.Value;
					It.RemoveCurrent();
					TagToValueMap.Remove(Tag);
					MarkArrayDirty();
					if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
					{
						Interface->OnTagFloatUpdate(Tag, OldValue, 0);
					}
				}
				else
				{
					const float OldValue = Item.Value;
					const float NewValue = Item.Value - Value;
					Item.Value = NewValue;
					TagToValueMap[Tag] = NewValue;
					MarkItemDirty(Item);
					if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
					{
						Interface->OnTagFloatUpdate(Tag, OldValue, NewValue);
					}
				}
				return;
			}
		}
	}
}

void FSigilGameplayTagFloatContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FSigilGameplayTagFloat& Item = Items[Index];
		TagToValueMap.Remove(Item.Tag);
		if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
		{
			Interface->OnTagFloatUpdate(Item.Tag, Item.PrevValue, 0);
		}
		Item.PrevValue = 0;
	}
}

void FSigilGameplayTagFloatContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FSigilGameplayTagFloat& Item = Items[Index];
		TagToValueMap.Add(Item.Tag, Item.Value);
		if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
		{
			Interface->OnTagFloatUpdate(Item.Tag, 0, Item.Value);
		}
		Item.PrevValue = Item.Value;
	}
}

void FSigilGameplayTagFloatContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FSigilGameplayTagFloat& Item = Items[Index];
		TagToValueMap[Item.Tag] = Item.Value;
		if (ISigilGameplayTagFloatContainerOwner* Interface = Cast<ISigilGameplayTagFloatContainerOwner>(ContainerOwner))
		{
			Interface->OnTagFloatUpdate(Item.Tag, Item.PrevValue, Item.Value);
		}
		Item.PrevValue = Item.Value;
	}
}
