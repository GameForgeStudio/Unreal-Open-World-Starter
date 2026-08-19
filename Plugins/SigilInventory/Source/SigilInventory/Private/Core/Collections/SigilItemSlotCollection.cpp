// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemSlotCollection.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySystemComponent.h"
#include "Items/SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemSlotCollection)

TSubclassOf<USigilItemCollection> USigilItemSlotCollectionDefinition::GetCollectionInstanceClass() const
{
	return USigilItemSlotCollection::StaticClass();
}

bool USigilItemSlotCollectionDefinition::IsValidSlotIndex(int32 SlotIndex) const
{
	return SlotDefinitions.IsValidIndex(SlotIndex);
}

int32 USigilItemSlotCollectionDefinition::GetIndexOfSlot(const FGameplayTag& SlotName) const
{
	return TagToIndexMap.Contains(SlotName) ? TagToIndexMap[SlotName] : INDEX_NONE;
}

FGameplayTag USigilItemSlotCollectionDefinition::GetSlotOfIndex(int32 SlotIndex) const
{
	return SlotDefinitions.IsValidIndex(SlotIndex) ? SlotDefinitions[SlotIndex].Tag : FGameplayTag::EmptyTag;
}

const TArray<FSigilItemSlotDefinition>& USigilItemSlotCollectionDefinition::GetSlotDefinitions() const
{
	return SlotDefinitions;
}

bool USigilItemSlotCollectionDefinition::GetSlotDefinition(int32 SlotIndex, FSigilItemSlotDefinition& OutDefinition) const
{
	if (SlotDefinitions.IsValidIndex(SlotIndex))
	{
		OutDefinition = SlotDefinitions[SlotIndex];
		return true;
	}
	return false;
}

bool USigilItemSlotCollectionDefinition::GetSlotDefinition(const FGameplayTag& SlotName, FSigilItemSlotDefinition& OutDefinition) const
{
	if (TagToIndexMap.Contains(SlotName))
	{
		check(SlotDefinitions.IsValidIndex(TagToIndexMap[SlotName]));
		OutDefinition = SlotDefinitions[TagToIndexMap[SlotName]];
		return true;
	}
	return false;
}

int32 USigilItemSlotCollectionDefinition::GetSlotIndexWithinGroup(FGameplayTag GroupTag, FGameplayTag SlotTag) const
{
	if (SlotGroupMap.Contains(GroupTag))
	{
		if (SlotGroupMap[GroupTag].SlotToIndexMap.Contains(SlotTag))
		{
			return SlotGroupMap[GroupTag].SlotToIndexMap[SlotTag];
		}
	}
	return INDEX_NONE;
}

USigilItemSlotCollection::USigilItemSlotCollection(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
}

void USigilItemSlotCollection::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME(ThisClass, ItemBySlots)
}

const USigilItemSlotCollectionDefinition* USigilItemSlotCollection::GetMyDefinition() const
{
	return MyDefinition;
}

FSigilItemInfo USigilItemSlotCollection::AddItem(const FSigilItemInfo& ItemInfo)
{
	if (!ItemInfo.IsValid())
	{
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}
	return AddItem(ItemInfo, GetTargetSlotIndex(ItemInfo.Item));
}

FSigilItemInfo USigilItemSlotCollection::AddItem(const FSigilItemInfo& ItemInfo, int32 SlotIndex)
{
	FSigilItemInfo ItemInfoAdded = AddItemInternal(ItemInfo, SlotIndex);

	if (ItemInfoAdded.Amount < ItemInfo.Amount)
	{
		HandleItemOverflow(ItemInfo, ItemInfoAdded);
	}
	return ItemInfoAdded;
}

void USigilItemSlotCollection::ServerAddItem_Implementation(const FSigilItemInfo& ItemInfo, int32 SlotIndex)
{
	AddItem(ItemInfo, SlotIndex);
}

FSigilItemInfo USigilItemSlotCollection::AddItemBySlotName(const FSigilItemInfo& ItemInfo, FGameplayTag SlotName)
{
	int32 Index = MyDefinition->GetIndexOfSlot(SlotName);
	if (Index <= INDEX_NONE)
	{
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	return AddItem(ItemInfo, Index);
}

void USigilItemSlotCollection::ServerAddItemBySlotName_Implementation(const FSigilItemInfo& ItemInfo, FGameplayTag SlotName)
{
	AddItemBySlotName(ItemInfo, SlotName);
}

FSigilItemInfo USigilItemSlotCollection::RemoveItem(const FSigilItemInfo& ItemInfo)
{
	int32 SlotIndex = GetItemSlotIndex(ItemInfo.Item);
	if (SlotIndex == INDEX_NONE)
	{
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	return RemoveItem(SlotIndex, ItemInfo.Amount);
}

FSigilItemInfo USigilItemSlotCollection::RemoveItem(int32 SlotIndex, int32 Amount)
{
	if (!SlotToStackMap.Contains(SlotIndex))
	{
		return FSigilItemInfo(nullptr, 0, this);
	}

	const FSigilItemStack* ItemToRemove = Container.FindById(SlotToStackMap[SlotIndex]);

	if (ItemToRemove == nullptr)
	{
		return FSigilItemInfo(nullptr, 0, this);
	}

	int32 AmountToRemove = Amount > 0 ? Amount : ItemToRemove->Amount;
	FSigilItemInfo Removed = RemoveInternal(FSigilItemInfo(ItemToRemove->Item, AmountToRemove, this, ItemToRemove->Id));

	return Removed;
}

bool USigilItemSlotCollection::IsItemFitWithSlot(const USigilItemInstance* Item, int32 SlotIndex) const
{
	if (!IsValid(Item))
	{
		SIGIL_INVENTORY_CLOG(Error, "invalid item")
		return false;
	}
	const TArray<FSigilItemSlotDefinition>& SlotDefinitions = MyDefinition->SlotDefinitions;
	if (SlotDefinitions.IsEmpty() || !SlotDefinitions.IsValidIndex(SlotIndex))
	{
		SIGIL_INVENTORY_CLOG(Error, "has empty slots!")
		return false;
	}

	const FSigilItemSlotDefinition& SlotDefinition = SlotDefinitions[SlotIndex];

	// doesn't match definition.
	if (!SlotDefinition.MatchItem(Item))
	{
		return false;
	}

	// slot is empty.
	if (!SlotToStackMap.Contains(SlotIndex))
	{
		return true;
	}

	// slot is not empty, check if it can stack.
	if (!Item->IsUnique())
	{
		if (const FSigilItemStack* ExistingStack = Container.FindById(SlotToStackMap[SlotIndex]))
		{
			if (ExistingStack->Item->StackableEquivalentTo(Item))
			{
				return true;
			}
		}
	}

	// can't stack, check new item can replace existing item.
	return MyDefinition->bNewItemPriority;
}

int32 USigilItemSlotCollection::GetTargetSlotIndex(const USigilItemInstance* Item) const
{
	const TArray<FSigilItemSlotDefinition>& SlotDefinitions = MyDefinition->SlotDefinitions;
	if (SlotDefinitions.IsEmpty())
	{
		SIGIL_INVENTORY_CLOG(Error, "has empty slots!")
		return INDEX_NONE;
	}

	int32 FoundUsedSlot = INDEX_NONE;
	int32 FoundEmptySlot = INDEX_NONE;

	for (int32 i = 0; i < SlotDefinitions.Num(); i++)
	{
		const FSigilItemSlotDefinition& SlotDefinition = SlotDefinitions[i];
		if (!SlotDefinition.MatchItem(Item))
		{
			continue;
		}

		FoundUsedSlot = i;

		// i对应的槽为空
		if (!SlotToStackMap.Contains(i))
		{
			if (FoundEmptySlot != INDEX_NONE)
			{
				continue;
			}
			FoundEmptySlot = i;
			continue;
		}

		//非唯一且可堆叠.
		if (!Item->IsUnique())
		{
			if (const FSigilItemStack* ExistingStack = Container.FindById(SlotToStackMap[i]))
			{
				if (ExistingStack->Item->StackableEquivalentTo(Item))
				{
					return i;
				}
			}
		}
	}

	if (FoundEmptySlot != INDEX_NONE)
	{
		return FoundEmptySlot;
	}
	return FoundUsedSlot;
}

bool USigilItemSlotCollection::GetItemInfoAtSlot(FGameplayTag SlotTag, FSigilItemInfo& OutItemInfo) const
{
	int32 Index = MyDefinition->GetIndexOfSlot(SlotTag);

	if (Index != INDEX_NONE)
	{
		OutItemInfo = GetItemInfoAtSlot(Index);

		return OutItemInfo.IsValid();
	}
	return false;
}

bool USigilItemSlotCollection::FindItemInfoAtSlot(FGameplayTag SlotTag, FSigilItemInfo& OutItemInfo) const
{
	return GetItemInfoAtSlot(SlotTag, OutItemInfo);
}

bool USigilItemSlotCollection::GetItemStackAtSlot(FGameplayTag SlotTag, FSigilItemStack& OutItemStack) const
{
	int32 Index = MyDefinition->GetIndexOfSlot(SlotTag);

	if (Index != INDEX_NONE)
	{
		OutItemStack = GetItemStackAtSlot(Index);
		return OutItemStack.IsValidStack();
	}
	return false;
}

bool USigilItemSlotCollection::FindItemStackAtSlot(FGameplayTag SlotTag, FSigilItemStack& OutItemStack) const
{
	return GetItemStackAtSlot(SlotTag, OutItemStack);
}

FSigilItemInfo USigilItemSlotCollection::GetItemInfoAtSlot(int32 SlotIndex) const
{
	if (SlotIndex <= 0) { return FSigilItemInfo::None; }

	FSigilItemStack ItemStack = GetItemStackAtSlot(SlotIndex);
	if (ItemStack.IsValidStack())
	{
		return FSigilItemInfo(ItemStack.Item, ItemStack.Amount, ItemStack.Collection);
	}
	return FSigilItemInfo::None;
}

FSigilItemStack USigilItemSlotCollection::GetItemStackAtSlot(int32 SlotIndex) const
{
	if (SlotToStackMap.Contains(SlotIndex))
	{
		if (const FSigilItemStack* Stack = Container.FindById(SlotToStackMap[SlotIndex]))
		{
			return *Stack;
		}
	}
	return FSigilItemStack();
}

FGameplayTag USigilItemSlotCollection::GetItemSlotName(const USigilItemInstance* Item) const
{
	int32 SlotIndex = GetItemSlotIndex(Item);
	return MyDefinition->GetSlotOfIndex(SlotIndex);
}

int32 USigilItemSlotCollection::GetItemSlotIndex(const USigilItemInstance* Item) const
{
	const TArray<FSigilItemSlotDefinition>& SlotDefinitions = MyDefinition->SlotDefinitions;

	int32 StackableEquivalentItemIndex = INDEX_NONE;
	for (int i = 0; i < SlotDefinitions.Num(); i++)
	{
		const FSigilItemSlotDefinition& SlotDefinition = SlotDefinitions[i];
		if (!SlotDefinition.MatchItem(Item))
		{
			continue;
		}

		if (!SlotToStackMap.Contains(i) || !SlotToStackMap[i].IsValid())
		{
			continue;
		}

		if (const FSigilItemStack* Stack = Container.FindById(SlotToStackMap[i]))
		{
			if (Stack->Item == Item)
			{
				return i;
			}

			if (Stack->Item->StackableEquivalentTo(Item))
			{
				StackableEquivalentItemIndex = i;
			}
		}
	}

	return StackableEquivalentItemIndex;
}

FSigilItemInfo USigilItemSlotCollection::AddItemInternal(const FSigilItemInfo& ItemInfo, int32 SlotIndex)
{
	FSigilItemInfo CanAddItemInfo;
	const bool CanAddResult = CanAddItem(ItemInfo, CanAddItemInfo);
	if (!CanAddResult)
	{
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	if (SlotIndex == INDEX_NONE)
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid valid target slot(%d) to put item:%s", SlotIndex, *ItemInfo.GetDebugString())
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	FSigilItemSlotDefinition SlotDefinition;
	if (!MyDefinition->GetSlotDefinition(SlotIndex, SlotDefinition))
	{
		SIGIL_INVENTORY_CLOG(Verbose, "No slot definition found for slot index:%d", SlotIndex)
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	if (!MyDefinition->bNewItemPriority && CanAddItemInfo.Item->IsUnique() && SlotToStackMap.Contains(SlotIndex) && SlotToStackMap[SlotIndex].IsValid())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "Can't add item info because the target slot:%s was occupied. ItemInfo:%s.", *SlotDefinition.Tag.ToString(), *ItemInfo.GetDebugString())
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}

	// similar item amount for this item.
	int32 CurrentAmount = GetItemAmount(CanAddItemInfo.Item.Get());

	FSigilItemInfo SetItemInfo;
	if (SetItemAmount(FSigilItemInfo(CanAddItemInfo.Amount + CurrentAmount, CanAddItemInfo), SlotIndex, true, SetItemInfo))
	{
		return SetItemInfo;
	}
	return FSigilItemInfo(ItemInfo.Item, 0, this);
}

int32 USigilItemSlotCollection::StackIdToSlotIndex(FGuid InStackId) const
{
	if (!InStackId.IsValid())
	{
		return INDEX_NONE;
	}
	if (StackToIdxMap.Contains(InStackId))
	{
		return StackToIdxMap[InStackId];
	}
	return INDEX_NONE;
}

int32 USigilItemSlotCollection::SlotIndexToStackIndex(int32 InSlotIndex) const
{
	if (SlotToStackMap.Contains(InSlotIndex))
	{
		return Container.IndexOfById(SlotToStackMap[InSlotIndex]);
	}
	return INDEX_NONE;
}

FGuid USigilItemSlotCollection::SlotIndexToStackId(int32 InSlotIndex) const
{
	return SlotToStackMap.Contains(InSlotIndex) && SlotToStackMap[InSlotIndex].IsValid() ? SlotToStackMap[InSlotIndex] : FGuid();
}

void USigilItemSlotCollection::OnRep_ItemsBySlot()
{
}

void USigilItemSlotCollection::SetDefinition(const USigilItemCollectionDefinition* NewDefinition)
{
	Super::SetDefinition(NewDefinition);
	check(OwningInventory != nullptr)
	check(Definition != nullptr)
	if (bInitialized)
	{
		MyDefinition = CastChecked<USigilItemSlotCollectionDefinition>(Definition);
		if (OwningInventory->GetOwnerRole() >= ROLE_Authority)
		{
			if (MyDefinition->SlotDefinitions.Num() == 0)
			{
				SIGIL_INVENTORY_CLOG(Error, "has empty slots")
			}
		}
	}
}

void USigilItemSlotCollection::OnPreItemStackAdded(const FSigilItemStack& Stack, int32 Idx)
{
	Super::OnPreItemStackAdded(Stack, Idx);
}

void USigilItemSlotCollection::OnItemStackAdded(const FSigilItemStack& Stack)
{
	check(Stack.IsValidStack())
	if (OwningInventory)
	{
		SlotToStackMap.Add(Stack.Index, Stack.Id);
	}
	Super::OnItemStackAdded(Stack);
}

void USigilItemSlotCollection::OnItemStackRemoved(const FSigilItemStack& Stack)
{
	if (OwningInventory)
	{
		SlotToStackMap.Remove(Stack.Index);
	}
	Super::OnItemStackRemoved(Stack);
}

bool USigilItemSlotCollection::SetItemAmount(const FSigilItemInfo& ItemInfo, int32 SlotIndex, bool RemovePreviousItem, FSigilItemInfo& ItemInfoAdded)
{
	if (!OwningInventory->GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "has no authority!");
		return false;
	}

	int32 Amount = ItemInfo.Amount;

	int32 StackIdx = SlotIndexToStackIndex(SlotIndex);

	// Found valid stack
	if (StackIdx != INDEX_NONE)
	{
		const FSigilItemStack& CurrentStack = Container.Stacks[StackIdx];

		if (ItemInfo.Item->StackableEquivalentTo(CurrentStack.Item))
		{
			// reduce existing amount to get the amount needed to add.
			Amount -= CurrentStack.Amount;
		}
		else if (RemovePreviousItem)
		{
			FSigilItemInfo RemovedItem = RemoveItem(FSigilItemInfo(CurrentStack));
			if (RemovedItem.Amount > 0)
			{
				FSigilItemInfo CanAddItemInfo;
				if (MyDefinition->bTryGivePrevItemToNewItemCollection && ItemInfo.ItemCollection != nullptr && ItemInfo.ItemCollection->CanAddItem(RemovedItem, CanAddItemInfo))
				{
					SIGIL_INVENTORY_CLOG(Verbose, "An existing item has been replaced by a new one, and the old one has been added to the collection where the new item coming from. prev:%s new:%s",
					         *RemovedItem.GetDebugString(), *ItemInfo.GetDebugString())
					ItemInfo.ItemCollection->AddItem(CanAddItemInfo);
				}
				else
				{
					SIGIL_INVENTORY_CLOG(Verbose, "An item has been replaced by a new one, and the old one has gone forever! prev: %s new:%s",
					         *RemovedItem.GetDebugString(), *ItemInfo.GetDebugString())
				}
			}
		}
		else
		{
			return false;
		}
	}

	// Not found, add new item.
	ItemInfoAdded = AddInternal(FSigilItemInfo(Amount, SlotIndex, ItemInfo));

	return true;
}

#if WITH_EDITOR
void USigilItemSlotCollectionDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	// remove repeated slot by name.
	{
		TArray<FGameplayTag> SlotNames;
		TArray<FSigilItemSlotDefinition> Slots;
		for (int32 i = 0; i < SlotDefinitions.Num(); ++i)
		{
			if (!SlotNames.Contains(SlotDefinitions[i].Tag))
			{
				Slots.Add(SlotDefinitions[i]);
				SlotNames.Add(SlotDefinitions[i].Tag);
			}
		}
		SlotDefinitions = Slots;
	}

	IndexToTagMap.Empty();
	TagToIndexMap.Empty();

	for (int32 i = 0; i < SlotDefinitions.Num(); ++i)
	{
		if (!SlotDefinitions[i].Tag.IsValid())
		{
			continue;
		}
		if (!IndexToTagMap.Contains(i))
		{
			IndexToTagMap.Add(i, SlotDefinitions[i].Tag);
		}
		if (!TagToIndexMap.Contains(SlotDefinitions[i].Tag))
		{
			TagToIndexMap.Add(SlotDefinitions[i].Tag, i);
		}
	}

	TArray<FSigilItemSlotGroup> Groups;
	SlotGroupMap.Empty();
	for (int32 i = 0; i < SlotGroups.Num(); i++)
	{
		FSigilItemSlotGroup Group;
		int32 Idx = 0;
		for (int32 j = 0; j < SlotDefinitions.Num(); j++)
		{
			if (SlotDefinitions[j].Tag.MatchesTag(SlotGroups[i]))
			{
				Group.IndexToSlotMap.Add(Idx, SlotDefinitions[j].Tag);
				Group.SlotToIndexMap.Add(SlotDefinitions[j].Tag, Idx);
				Idx++;
			}
		}
		if (!Group.IndexToSlotMap.IsEmpty())
		{
			SlotGroupMap.Add(SlotGroups[i], Group);
		}
	}

	Super::PreSave(SaveContext);
}
#endif
