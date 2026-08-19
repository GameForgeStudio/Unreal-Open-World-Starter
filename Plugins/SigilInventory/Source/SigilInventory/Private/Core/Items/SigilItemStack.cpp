// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Items/SigilItemStack.h"
#include "Components/ActorComponent.h"
#include "SigilInventoryMessages.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"
#include "SigilInventorySystemComponent.h"
#include "Items/SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"

#include "SigilItemSlotCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemStack)

FGuid FSigilItemStack::InvalidId = FGuid(0, 0, 0, 0);

FSigilItemStack::FSigilItemStack()
{
	Item = nullptr;
	Id = InvalidId;
	Amount = 0;
	Collection = nullptr;
	LastObservedAmount = -1;
}

FString FSigilItemStack::GetDebugString()
{
	return FString::Format(TEXT("Item({0}),Amount({1}),ID({2})"), {Item && Item->GetDefinition() ? GetNameSafe(Item->GetDefinition()) : TEXT("None"), Amount, Id.ToString()});
}

void FSigilItemStack::Initialize(FGuid InStackId, USigilItemInstance* InItem, int32 InAmount, USigilItemCollection* InCollection, int32 InIndex)
{
	Id = InStackId;
	Item = InItem;
	Amount = InAmount;
	Collection = InCollection;
	Index = InIndex;
	LastObservedAmount = InAmount;
}

bool FSigilItemStack::IsValidStack() const
{
	return Id.IsValid() && IsValid(Item) && IsValid(Collection) && Amount > 0;
}

void FSigilItemStack::Reset()
{
	Id.Invalidate();
	Item = nullptr;
	Amount = 0;
	Collection = nullptr;
	LastObservedAmount = INDEX_NONE;
}

bool FSigilItemStack::operator==(const FSigilItemStack& Other) const
{
	return Id == Other.Id && Item->GetItemId() == Other.Item->GetItemId();
}

bool FSigilItemStack::operator!=(const FSigilItemStack& Other) const
{
	return !(operator==(Other));
}

bool FSigilItemStack::operator==(const FGuid& OtherId) const
{
	return Id == OtherId;
}

bool FSigilItemStack::operator!=(const FGuid& OtherId) const
{
	return Id != OtherId;
}

void FSigilItemStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FSigilItemStack& Stack = Stacks[Index];

		if (OwningCollection->StackToIdxMap.Contains(Stack.Id))
		{
			OwningCollection->OnItemStackRemoved(Stack);
		}
		else if (OwningCollection->PendingItemStacks.Contains(Stack.Id))
		{
			SIGIL_INVENTORY_OWNED_CLOG(OwningCollection, Warning, "Discard pending item stack(%s).", *OwningCollection->PendingItemStacks[Stack.Id].GetDebugString())
			OwningCollection->PendingItemStacks.Remove(Stack.Id);
		}

		Stack.LastObservedAmount = 0;
	}
}

void FSigilItemStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FSigilItemStack& Stack = Stacks[Index];

		if (OwningCollection && OwningCollection->IsInitialized() && Stack.IsValidStack())
		{
			OwningCollection->OnItemStackAdded(Stack);
		}
		else if (OwningCollection->PendingItemStacks.Contains(Stack.Id))
		{
			OwningCollection->PendingItemStacks[Stack.Id] = Stack;
		}
		else
		{
			OwningCollection->PendingItemStacks.Add(Stack.Id, Stack);
		}
		Stack.LastObservedAmount = Stack.Amount;
	}
}

void FSigilItemStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FSigilItemStack& Stack = Stacks[Index];
		check(Stack.LastObservedAmount != INDEX_NONE);

		if (OwningCollection->StackToIdxMap.Contains(Stack.Id)) //Already Added.
		{
			OwningCollection->OnItemStackUpdated(Stack);
		}
		else if (OwningCollection->PendingItemStacks.Contains(Stack.Id)) //In pending list.
		{
			OwningCollection->PendingItemStacks.Emplace(Stack.Id, Stack); // Updated to pending.
		}
		else
		{
			OwningCollection->PendingItemStacks.Add(Stack.Id, Stack); //Add to pending list.
		}
		Stack.LastObservedAmount = Stack.Amount;
	}
}

const FSigilItemStack* FSigilItemStackContainer::FindById(const FGuid& StackId) const
{
	if (!StackId.IsValid())
	{
		return nullptr;
	}
	return Stacks.FindByPredicate([StackId](const FSigilItemStack& Stack)
	{
		return Stack.Id == StackId;
	});
}

const FSigilItemStack* FSigilItemStackContainer::FindByItemId(const FGuid& ItemId) const
{
	if (!ItemId.IsValid())
	{
		return nullptr;
	}
	return Stacks.FindByPredicate([ItemId](const FSigilItemStack& Stack)
	{
		return Stack.Item->GetItemId() == ItemId;
	});
}

int32 FSigilItemStackContainer::IndexOfById(const FGuid& StackId) const
{
	if (!StackId.IsValid())
	{
		return INDEX_NONE;
	}
	return Stacks.IndexOfByPredicate([StackId](const FSigilItemStack& Stack)
	{
		return Stack.Id == StackId;
	});
}

int32 FSigilItemStackContainer::IndexOfByItemId(const FGuid& ItemId) const
{
	return Stacks.IndexOfByPredicate([ItemId](const FSigilItemStack& Stack)
	{
		return Stack.Item->GetItemId() == ItemId;
	});
}

int32 FSigilItemStackContainer::IndexOfByIds(const FGuid& StackId, const FGuid& ItemId) const
{
	if (!StackId.IsValid() || !ItemId.IsValid())
	{
		return INDEX_NONE;
	}
	return Stacks.IndexOfByPredicate([StackId,ItemId](const FSigilItemStack& Stack)
	{
		return Stack.Id == StackId && Stack.Item->GetItemId() == ItemId;
	});
}
