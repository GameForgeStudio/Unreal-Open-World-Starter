// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilCollectionContainer.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCollectionContainer)


bool FSigilCollectionEntry::IsValidEntry() const
{
	return Id.IsValid() && IsValid(Instance) && IsValid(Definition);
}

FSigilCollectionContainer::FSigilCollectionContainer(USigilInventorySystemComponent* InInventory)
{
	OwningComponent = InInventory;
}

void FSigilCollectionContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FSigilCollectionEntry& Entry = Entries[Index];

		// already in the list.
		if (Entry.IsValidEntry() && OwningComponent->CollectionIdToInstanceMap.Contains(Entry.Instance->GetCollectionId()))
		{
			OwningComponent->OnCollectionRemoved(Entry);
		}
		else if (OwningComponent->PendingCollections.Contains(Entry.Id))
		{
			SIGIL_INVENTORY_OWNED_CLOG(OwningComponent, Warning, "Discard pending collection(%s).", *GetNameSafe(OwningComponent->PendingCollections[Entry.Id].Definition))
			OwningComponent->PendingCollections.Remove(Entry.Id);
		}
	}
}

void FSigilCollectionContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FSigilCollectionEntry& Entry = Entries[Index];

		if (OwningComponent->GetOwner() && Entry.IsValidEntry())
		{
			OwningComponent->OnCollectionAdded(Entry);
		}
		else
		{
			OwningComponent->PendingCollections.Add(Entry.Id, Entry);
		}
	}
}

void FSigilCollectionContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FSigilCollectionEntry& Entry = Entries[Index];

		if (Entry.IsValidEntry() && OwningComponent->CollectionIdToInstanceMap.Contains(Entry.Instance->GetCollectionId())) //Already Added.
		{
			OwningComponent->OnCollectionUpdated(Entry);
		}
		else if (OwningComponent->PendingCollections.Contains(Entry.Id)) //In pending list.
		{
			OwningComponent->PendingCollections.Emplace(Entry.Id, Entry); // Updated to pending.
		}
		else
		{
			OwningComponent->PendingCollections.Add(Entry.Id, Entry); //Add to pending list.
		}
	}
}
