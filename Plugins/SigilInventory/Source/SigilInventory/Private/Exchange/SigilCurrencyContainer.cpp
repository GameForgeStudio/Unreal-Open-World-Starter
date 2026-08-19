// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilCurrencyContainer.h"

#include "SigilCurrencySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencyContainer)

void FSigilCurrencyContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FSigilCurrencyEntry& Entry = Entries[Index];
		if (OwningComponent)
		{
			OwningComponent->OnCurrencyEntryRemoved(Entry, Index);
		}
		Entry.PrevAmount = 0;
	}
}

void FSigilCurrencyContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FSigilCurrencyEntry& Entry = Entries[Index];
		if (OwningComponent)
		{
			OwningComponent->OnCurrencyEntryAdded(Entry, Index);
		}
		Entry.PrevAmount = Entry.Amount;
	}
}

void FSigilCurrencyContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FSigilCurrencyEntry& Entry = Entries[Index];
		if (OwningComponent)
		{
			OwningComponent->OnCurrencyEntryUpdated(Entry, Index, Entry.PrevAmount, Entry.Amount);
		}
		Entry.PrevAmount = Entry.Amount;
	}
}
