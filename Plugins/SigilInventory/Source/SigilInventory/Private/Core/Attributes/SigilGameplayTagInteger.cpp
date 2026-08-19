// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Attributes/SigilGameplayTagInteger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilGameplayTagInteger)

FString FSigilGameplayTagInteger::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), Value);
}

void FSigilGameplayTagIntegerContainer::AddItem(FGameplayTag Tag, int32 Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddItem"), ELogVerbosity::Warning);
		return;
	}

	if (Value > 0)
	{
		for (FSigilGameplayTagInteger& Item : Items)
		{
			if (Item.Tag == Tag)
			{
				const int32 OldValue = Item.Value;
				const int32 NewValue = Item.Value + Value;
				Item.Value = NewValue;
				TagToValueMap[Tag] = NewValue;
				MarkItemDirty(Item);
				if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
				{
					Interface->OnTagIntegerUpdate(Tag, OldValue, NewValue);
				}
				return;
			}
		}

		FSigilGameplayTagInteger& NewItem = Items.Emplace_GetRef(Tag, Value);
		TagToValueMap.Add(Tag, Value);
		MarkItemDirty(NewItem);
		if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
		{
			Interface->OnTagIntegerUpdate(Tag, 0, Value);
		}
	}
}

void FSigilGameplayTagIntegerContainer::SetItem(FGameplayTag Tag, int32 Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to SetItem"), ELogVerbosity::Warning);
		return;
	}
	for (FSigilGameplayTagInteger& Item : Items)
	{
		if (Item.Tag == Tag)
		{
			int32 OldValue = Item.Value;
			Item.Value = Value;
			TagToValueMap[Tag] = Value;
			MarkItemDirty(Item);
			if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
			{
				Interface->OnTagIntegerUpdate(Tag, OldValue, Value);
			}
			return;
		}
	}

	FSigilGameplayTagInteger& NewItem = Items.Emplace_GetRef(Tag, Value);
	MarkItemDirty(NewItem);
	TagToValueMap.Add(Tag, Value);
	if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
	{
		Interface->OnTagIntegerUpdate(Tag, 0, Value);
	}
}

void FSigilGameplayTagIntegerContainer::RemoveItem(FGameplayTag Tag, int32 Value)
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
			FSigilGameplayTagInteger& Item = *It;
			if (Item.Tag == Tag)
			{
				if (Item.Value <= Value)
				{
					const int32 OldValue = Item.Value;
					It.RemoveCurrent();
					TagToValueMap.Remove(Tag);
					MarkArrayDirty();
					if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
					{
						Interface->OnTagIntegerUpdate(Tag, OldValue, 0);
					}
				}
				else
				{
					const int32 OldValue = Item.Value;
					const int32 NewValue = Item.Value - Value;
					Item.Value = NewValue;
					TagToValueMap[Tag] = NewValue;
					MarkItemDirty(Item);
					if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
					{
						Interface->OnTagIntegerUpdate(Tag, OldValue, NewValue);
					}
				}
				return;
			}
		}
	}
}

void FSigilGameplayTagIntegerContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FSigilGameplayTagInteger& Item = Items[Index];

		TagToValueMap.Remove(Item.Tag);
		if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
		{
			Interface->OnTagIntegerUpdate(Item.Tag, Item.PrevValue, 0);
		}
		Item.PrevValue = 0;
	}
}

void FSigilGameplayTagIntegerContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FSigilGameplayTagInteger& Item = Items[Index];
		TagToValueMap.Add(Item.Tag, Item.Value);
		if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
		{
			Interface->OnTagIntegerUpdate(Item.Tag, 0, Item.Value);
		}
		Item.PrevValue = Item.Value;
	}
}

void FSigilGameplayTagIntegerContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FSigilGameplayTagInteger& Item = Items[Index];
		TagToValueMap[Item.Tag] = Item.Value;
		if (ISigilGameplayTagIntegerContainerOwner* Interface = Cast<ISigilGameplayTagIntegerContainerOwner>(ContainerOwner))
		{
			Interface->OnTagIntegerUpdate(Item.Tag, Item.PrevValue, Item.Value);
		}
		Item.PrevValue = Item.Value;
	}
}
