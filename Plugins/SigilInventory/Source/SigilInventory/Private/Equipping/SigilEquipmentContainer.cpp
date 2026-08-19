// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilEquipmentContainer.h"
#include "SigilEquipmentInstance.h"
#include "SigilEquipmentSystemComponent.h"
#include "SigilInventoryLogChannels.h"
#include "Items/SigilItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilEquipmentContainer)

FString FSigilEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s"), *GetNameSafe(Instance));
}

bool FSigilEquipmentEntry::IsValid() const
{
	return Instance != nullptr && ItemInstance != nullptr && EquippedSlot.IsValid();
}

void FSigilEquipmentContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FSigilEquipmentEntry& Entry = Entries[Index];

		// already in the list.
		if (OwningComponent->SlotToIdxMap.Contains(Entry.EquippedSlot))
		{
			OwningComponent->OnEquipmentEntryRemoved(Entry, Index);
		}
		else if (OwningComponent->PendingEquipmentEntries.Contains(Index))
		{
			SIGIL_INVENTORY_OWNED_CLOG(OwningComponent, Warning, "Discard pending equipment(%s).", *OwningComponent->PendingEquipmentEntries[Index].GetDebugString())
			OwningComponent->PendingEquipmentEntries.Remove(Index);
		}
		Entry.bPrevActive = Entry.bActive;
	}
}

void FSigilEquipmentContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FSigilEquipmentEntry& Entry = Entries[Index];
		if (OwningComponent && OwningComponent->GetOwner() && OwningComponent->IsEquipmentSystemInitialized() && Entry.IsValid())
		{
			OwningComponent->OnEquipmentEntryAdded(Entry, Index);
		}
		else
		{
			OwningComponent->PendingEquipmentEntries.Add(Index, Entry);
		}
		Entry.bPrevActive = Entry.bActive;
	}
}

void FSigilEquipmentContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FSigilEquipmentEntry& Entry = Entries[Index];
		if (OwningComponent->SlotToIdxMap.Contains(Entry.EquippedSlot)) //Already Added.
		{
			OwningComponent->OnEquipmentEntryChanged(Entry, Index);
		}
		else if (OwningComponent->PendingEquipmentEntries.Contains(Index)) //In pending list.
		{
			OwningComponent->PendingEquipmentEntries.Emplace(Index, Entry);
		}
		else
		{
			OwningComponent->PendingEquipmentEntries.Add(Index, Entry); //Add to pending list.
		}
		Entry.bPrevActive = Entry.bActive;
	}
}

int32 FSigilEquipmentContainer::IndexOfBySlot(const FGameplayTag& Slot) const
{
	if (!Slot.IsValid())
	{
		return INDEX_NONE;
	}
	return Entries.IndexOfByPredicate([Slot](const FSigilEquipmentEntry& Entry)
	{
		return Entry.EquippedSlot == Slot;
	});
}

int32 FSigilEquipmentContainer::IndexOfByItem(const USigilItemInstance* Item) const
{
	if (!IsValid(Item))
	{
		return INDEX_NONE;
	}
	return Entries.IndexOfByPredicate([Item](const FSigilEquipmentEntry& Entry)
	{
		return Entry.ItemInstance && Entry.ItemInstance == Item;
	});
}

int32 FSigilEquipmentContainer::IndexOfByItemId(const FGuid& ItemId) const
{
	if (!ItemId.IsValid())
	{
		return INDEX_NONE;
	}
	return Entries.IndexOfByPredicate([ItemId](const FSigilEquipmentEntry& Entry)
	{
		return Entry.ItemInstance && Entry.ItemInstance->GetItemId() == ItemId;
	});
}
