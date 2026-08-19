// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemCollection.h"
#include "SigilInventorySystemComponent.h"
#include "SigilInventoryMessages.h"
#include "SigilInventorySubsystem.h"
#include "SigilInventoryTags.h"
#include "Items/SigilItemInstance.h"
#include "Items/SigilItemDefinition.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "SigilItemRestriction.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemCollection)

bool USigilItemCollectionDefinition::IsSupportedForNetworking() const
{
	return true;
}

TSubclassOf<USigilItemCollection> USigilItemCollectionDefinition::GetCollectionInstanceClass() const
{
	return USigilItemCollection::StaticClass();
}

USigilItemCollection::SigilCollectionNotifyLocker::SigilCollectionNotifyLocker(USigilItemCollection& InItemCollection): ItemCollection(InItemCollection)
{
	ItemCollection.NotifyLocker++;
}

USigilItemCollection::SigilCollectionNotifyLocker::~SigilCollectionNotifyLocker()
{
	ItemCollection.NotifyLocker--;
}

USigilItemCollection::USigilItemCollection(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), Container(this)
{
}

void USigilItemCollection::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Container);

	//fix: https://forums.unrealengine.com/t/subobject-replication-for-blueprint-child-class/106205/4
	UBlueprintGeneratedClass* bpClass = Cast<UBlueprintGeneratedClass>(this->GetClass());
	if (bpClass != nullptr)
	{
		bpClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

bool USigilItemCollection::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != nullptr);

	AActor* Owner = CastChecked<AActor>(GetOuter());

	bool bProcessed = false;

	FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
	if (Context != nullptr)
	{
		for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
		{
			if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(Owner, Function))
			{
				Driver.NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
				bProcessed = true;
			}
		}
	}

	return bProcessed;
}

int32 USigilItemCollection::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool USigilItemCollection::IsInitialized() const
{
	return bInitialized;
}

FString USigilItemCollection::GetCollectionName() const
{
	if (Definition)
	{
		if (Definition->CollectionTag.IsValid())
		{
			TArray<FName> TagNames;
			UGameplayTagsManager::Get().SplitGameplayTagFName(Definition->CollectionTag, TagNames);
			if (!TagNames.IsEmpty())
			{
				return FString::Format(TEXT("{0} Collection"), {TagNames.Last().ToString()});
			}
		}
		return FString::Format(TEXT("{0}"), {GetNameSafe(this)});
	}
	return TEXT("Invalid Collection!!!");
}

FString USigilItemCollection::GetDebugString() const
{
	return FString::Format(TEXT("{0}({1})"), {GetCollectionName(), GetNameSafe(OwningInventory->GetOwner())});
}

bool USigilItemCollection::HasItem(const USigilItemInstance* Item, int32 Amount, bool SimilarItem) const
{
	if (Item == nullptr) { return false; }

	return GetItemAmount(Item, SimilarItem) >= Amount;
}

FSigilItemInfo USigilItemCollection::AddItem(const FSigilItemInfo& ItemInfo)
{
	//The actually added item info; 实际添加的道具信息
	FSigilItemInfo ItemInfoAdded = FSigilItemInfo(ItemInfo.Item, 0, this);

	FSigilItemInfo CanAddItemInfo;
	if (CanAddItem(ItemInfo, CanAddItemInfo))
	{
		//非唯一道具或数量为一，直接加.
		if (!CanAddItemInfo.Item->IsUnique() || CanAddItemInfo.Amount <= 1)
		{
			//要添加的信息
			FSigilItemInfo ItemInfoToAdd(CanAddItemInfo.Item, CanAddItemInfo.Amount, ItemInfo);
			ItemInfoAdded = AddInternal(ItemInfoToAdd);
		}
		else //unique item can stack.
		{
			//先加1个
			FSigilItemInfo OriginalResult = AddItem(FSigilItemInfo(CanAddItemInfo.Item, 1, ItemInfo));
			// 遍历加入数量为1的道具, 每个都有不同的GUID
			for (int32 i = 1; i < CanAddItemInfo.Amount; i++)
			{
				USigilItemInstance* DuplicatedItem = USigilInventorySubsystem::Get(GetWorld())->DuplicateItem(OwningInventory->GetOwner(), CanAddItemInfo.Item);
				check(DuplicatedItem && DuplicatedItem->IsItemValid())
				AddItem(FSigilItemInfo(DuplicatedItem, 1, ItemInfo));
			}
			ItemInfoAdded = FSigilItemInfo(CanAddItemInfo.Amount, OriginalResult);
		}
	}

	// overflow
	if (ItemInfoAdded.Amount < ItemInfo.Amount)
	{
		HandleItemOverflow(ItemInfo, ItemInfoAdded);
	}

	return ItemInfoAdded;
}

int32 USigilItemCollection::AddItems(const TArray<FSigilItemInfo>& ItemInfos)
{
	int32 TotalAdded = 0;
	for (int32 i = 0; i < ItemInfos.Num(); i++)
	{
		TotalAdded += AddItem(ItemInfos[i]).Amount;
	}
	return TotalAdded;
}

FSigilItemInfo USigilItemCollection::AddItem(USigilItemInstance* Item, int32 Amount)
{
	return AddItem(FSigilItemInfo(Item, Amount));
}

bool USigilItemCollection::CanAddItem(const FSigilItemInfo& Input, FSigilItemInfo& Output)
{
	if (!bInitialized || Input.Item == nullptr || !Input.Item->IsItemValid() || Input.Amount < 1)
	{
		return false;
	}

	FSigilItemInfo ModifiedInput = Input;

	/*
	 * Documentation: For none-unique item(means stackable), if you want to add 2 apples with id x, there are following case:
	 * 1.There are already 10 apples with id x in your inventory,this will turn your "add 2 apples of id x" request into "add 2 apples of id y", resulting 12 apples with id y in your inventory.
	 * 2.There's no any apples in your inventory, resulting in 2 apples with id x in your inventory.
	 * 文档: 道具不唯一. 举例: 你要加2个苹果(ID是X)到库存，且苹果定义上它不是唯一的（即可叠加）。
	 * 如果1：库存里如果已经有10个苹果(ID是Y)。就把 “你要加2个ID为X的苹果” 变成“你要加2个ID为Y的苹果”.
	 * 如果2：库存里如果没有任何苹果。那么就不变，其结果就是往库存里加2个ID是X的苹果.
	 */
	if (!ModifiedInput.Item->IsUnique())
	{
		FSigilItemInfo SimilarItemInfo;
		if (GetItemInfo(ModifiedInput.Item, SimilarItemInfo))
		{
			//修改要加入的道具信息为相似道具、但保留Amount.
			ModifiedInput = FSigilItemInfo(Input.Amount, SimilarItemInfo);
		}
	}

	if (ModifiedInput.Item->GetOwningCollection() != nullptr && ModifiedInput.Item->GetOwningCollection() != this)
	{
		USigilItemInstance* DuplicatedItem = USigilInventorySubsystem::Get(GetWorld())->DuplicateItem(OwningInventory->GetOwner(), ModifiedInput.Item);
		check(DuplicatedItem && DuplicatedItem->IsItemValid())
		SIGIL_INVENTORY_LOG(Verbose, "The Item(%s) was duplicated, and the duplicated one was added because the item is already part of another collection on actor %s.", *Input.GetDebugString(),
		        *GetNameSafe(Input.Item->GetOuter()))
		ModifiedInput = FSigilItemInfo(DuplicatedItem, Input.Amount, Input);
	}

	if (ModifiedInput.Item->GetOuter() != nullptr && ModifiedInput.Item->GetOuter() != OwningInventory->GetOwner())
	{
		USigilItemInstance* DuplicatedItem = USigilInventorySubsystem::Get(GetWorld())->DuplicateItem(OwningInventory->GetOwner(), ModifiedInput.Item, false);
		check(DuplicatedItem && DuplicatedItem->IsItemValid())
		SIGIL_INVENTORY_LOG(Verbose, "The Item(%s) was duplicated, and the duplicated one was added because the item was created by another actor %s.", *Input.GetDebugString(),
		        *GetNameSafe(Input.Item->GetOuter()))
		ModifiedInput = FSigilItemInfo(DuplicatedItem, Input.Amount, Input);
	}

	for (int32 i = 0; i < Definition->Restrictions.Num(); i++)
	{
		auto Restriction = Definition->Restrictions[i];
		if (Restriction && !Restriction->CanAddItem(ModifiedInput, this))
		{
			SIGIL_INVENTORY_CLOG(Warning, "can't add item(%s) due to restriction:%s", *Input.GetDebugString(), *Restriction->GetClass()->GetName())
			return false;
		}
	}

	Output = ModifiedInput;
	return true;
}

bool USigilItemCollection::CanItemStack(const FSigilItemInfo& ItemInfo, const FSigilItemStack& ItemStack) const
{
	return !ItemInfo.Item->IsUnique() && ItemStack.Item->StackableEquivalentTo(ItemInfo.Item);
}

bool USigilItemCollection::RemoveItemCondition(const FSigilItemInfo& ItemInfo, FSigilItemInfo& OutItemInfo)
{
	if (!ItemInfo.Item || !ItemInfo.Item->IsItemValid() || ItemInfo.Amount < 1)
	{
		return false;
	}
	OutItemInfo = ItemInfo;

	if (!ItemInfo.Item->IsUnique())
	{
		FSigilItemInfo SimilarItemInfo;
		if (GetItemInfo(ItemInfo.Item, SimilarItemInfo))
		{
			OutItemInfo = FSigilItemInfo(SimilarItemInfo.Item, ItemInfo.Amount);
		}
	}

	// make sure is this collection.
	if (OutItemInfo.ItemCollection != this)
	{
		OutItemInfo.ItemCollection = this;
	}

	for (int32 i = 0; i < Definition->Restrictions.Num(); i++)
	{
		// Check against src item info in restriction.
		if (!Definition->Restrictions[i]->CanRemoveItem(OutItemInfo))
		{
			return false;
		}
	}

	return true;
}

FSigilItemInfo USigilItemCollection::RemoveItem(const FSigilItemInfo& ItemInfo)
{
	FSigilItemInfo ItemInfoToRemove;
	if (!RemoveItemCondition(ItemInfo, ItemInfoToRemove))
	{
		return FSigilItemInfo(ItemInfo.Item, 0, this);
	}
	return RemoveInternal(ItemInfoToRemove);
}

void USigilItemCollection::RemoveAll()
{
	for (int32 i = Container.Stacks.Num() - 1; i >= 0; i--)
	{
		FSigilItemStack& ItemStack = Container.Stacks[i];
		RemoveItem(FSigilItemInfo(ItemStack.Item, ItemStack.Amount));
	}
}

bool USigilItemCollection::GetItemInfoByStackId(FGuid InStackId, FSigilItemInfo& OutItemInfo) const
{
	if (const FSigilItemStack* Stack = Container.Stacks.FindByKey(InStackId))
	{
		OutItemInfo = *Stack;
		return true;
	}
	return false;
}

bool USigilItemCollection::FindItemInfoByStackId(FGuid InStackId, FSigilItemInfo& OutItemInfo) const
{
	return GetItemInfoByStackId(InStackId, OutItemInfo);
}

bool USigilItemCollection::GetItemInfo(const USigilItemInstance* Item, FSigilItemInfo& OutItemInfo) const
{
	if (Item == nullptr)
	{
		return false;
	}

	bool bFoundSimilar = false;
	FSigilItemInfo SimilarItemInfo;
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		const FSigilItemStack& ItemStack = Container.Stacks[i];
		//Is not unique but has same definition.
		if (!Item->IsUnique() && ItemStack.Item->GetDefinition() == Item->GetDefinition())
		{
			SimilarItemInfo = FSigilItemInfo(ItemStack.Item, ItemStack.Amount, ItemStack.Collection);
			bFoundSimilar = true;
		}

		if (ItemStack.Item->GetItemId() != Item->GetItemId())
		{
			continue;
		}

		//Found unique item with same id.
		OutItemInfo = FSigilItemInfo(ItemStack.Item, ItemStack.Amount, ItemStack.Collection);
		return true;
	}

	if (bFoundSimilar)
	{
		OutItemInfo = SimilarItemInfo;
	}
	return bFoundSimilar;
}

bool USigilItemCollection::GetItemInfoByDefinition(const TSoftObjectPtr<USigilItemDefinition>& ItemDefinition, FSigilItemInfo& OutItemInfo)
{
	if (ItemDefinition.IsNull())
	{
		return false;
	}

	USigilItemDefinition* LoadedItemDefinition = ItemDefinition.LoadSynchronous();

	if (LoadedItemDefinition == nullptr)
	{
		return false;
	}

	bool bFoundSimilar = false;
	FSigilItemInfo SimilarItemInfo;
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		FSigilItemStack& ItemStack = Container.Stacks[i];
		if (ItemStack.Item->GetDefinition() == LoadedItemDefinition)
		{
			SimilarItemInfo = FSigilItemInfo(ItemStack.Item, ItemStack.Amount, ItemStack.Collection);
			bFoundSimilar = true;
		}
	}

	if (bFoundSimilar && SimilarItemInfo.IsValid())
	{
		OutItemInfo = SimilarItemInfo;
		return true;
	}
	return false;
}

bool USigilItemCollection::GetItemInfosByDefinition(const TSoftObjectPtr<USigilItemDefinition>& ItemDefinition, TArray<FSigilItemInfo>& OutItemInfos)
{
	if (ItemDefinition.IsNull())
	{
		return false;
	}

	USigilItemDefinition* LoadedItemDefinition = ItemDefinition.LoadSynchronous();

	if (LoadedItemDefinition == nullptr)
	{
		return false;
	}

	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		FSigilItemStack& ItemStack = Container.Stacks[i];
		if (ItemStack.Item->GetDefinition() == LoadedItemDefinition)
		{
			FSigilItemInfo SimilarItemInfo = FSigilItemInfo(ItemStack.Item, ItemStack.Amount, ItemStack.Collection);
			OutItemInfos.Add(SimilarItemInfo);
		}
	}
	return OutItemInfos.Num() > 0;
}

int32 USigilItemCollection::GetItemAmount(const USigilItemInstance* Item, bool SimilarItem) const
{
	if (Item == nullptr) { return 0; }

	int32 Count = 0;

	for (int i = 0; i < Container.Stacks.Num(); i++)
	{
		if (Container.Stacks[i].Item && Container.Stacks[i].Item->SimilarTo(Item))
		{
			Count += Container.Stacks[i].Amount;
		}
	}

	return Count;
}

int32 USigilItemCollection::GetItemAmount(TSoftObjectPtr<USigilItemDefinition> ItemDefinition, bool CountStacks) const
{
	if (ItemDefinition.IsNull())
	{
		return 0;
	}
	USigilItemDefinition* LoadedDefinition = ItemDefinition.LoadSynchronous();
	if (LoadedDefinition == nullptr)
	{
		return 0;
	}
	int32 Count = 0;
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		if (Container.Stacks[i].Item == nullptr)
		{
			continue;
		}
		if (LoadedDefinition != Container.Stacks[i].Item->GetDefinition())
		{
			continue;
		}
		Count += CountStacks ? 1 : Container.Stacks[i].Amount;
	}

	return Count;
}

TArray<FSigilItemInfo> USigilItemCollection::GetAllItemInfos() const
{
	TArray<FSigilItemInfo> Infos;
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		const FSigilItemStack& ItemStack = Container.Stacks[i];
		Infos.Add(FSigilItemInfo(ItemStack));
	}
	return Infos;
}

TArray<USigilItemInstance*> USigilItemCollection::GetAllItems() const
{
	TArray<USigilItemInstance*> Rets;
	for (int32 i = 0; i < Container.Stacks.Num(); i++)
	{
		if (Container.Stacks[i].IsValidStack())
		{
			Rets.AddUnique(Container.Stacks[i].Item);
		}
	}
	return Rets;
}

const TArray<FSigilItemStack>& USigilItemCollection::GetAllItemStacks() const
{
	return Container.Stacks;
}

int32 USigilItemCollection::GetItemStacksNum() const
{
	return Container.Stacks.Num();
}

void USigilItemCollection::AddItemStack(const FSigilItemStack& Stack)
{
	check(Stack.IsValidStack())
	int32 Idx = Container.Stacks.AddDefaulted();
	Container.Stacks[Idx] = Stack;
	FSigilItemStack& AddedStack = Container.Stacks[Idx];

	OnPreItemStackAdded(Stack, Idx);
	OnItemStackAdded(AddedStack);

	Container.MarkItemDirty(AddedStack);
}

void USigilItemCollection::RemoveItemStackAtIndex(int32 Idx, bool bRemoveFromCollection)
{
	check(Container.Stacks.IsValidIndex(Idx))
	const FSigilItemStack RemovedStack = Container.Stacks[Idx];
	OnItemStackRemoved(RemovedStack);
	Container.Stacks.RemoveAt(Idx);
	if (bRemoveFromCollection)
	{
		RemovedStack.Item->UnassignCollection(this);
		if (OwningInventory->IsReplicatedSubObjectRegistered(RemovedStack.Item))
		{
			OwningInventory->RemoveReplicatedSubObject(RemovedStack.Item);
		}
	}
	Container.MarkArrayDirty();
}

void USigilItemCollection::UpdateItemStackAmountAtIndex(int32 Idx, int32 NewAmount)
{
	check(Container.Stacks.IsValidIndex(Idx))
	FSigilItemStack& StackToUpdate = Container.Stacks[Idx];
	StackToUpdate.Amount = NewAmount;
	OnItemStackUpdated(StackToUpdate);
	Container.MarkItemDirty(StackToUpdate);
}

void USigilItemCollection::OnPreItemStackAdded(const FSigilItemStack& Stack, int32 Idx)
{
	if (IsValid(Stack.Item))
	{
		if (Stack.Item->GetOwningCollection() != this)
		{
			Stack.Item->AssignCollection(this);
		}

		if (!OwningInventory->IsReplicatedSubObjectRegistered(Stack.Item))
		{
			OwningInventory->AddReplicatedSubObject(Stack.Item);
		}
	}
}

void USigilItemCollection::OnItemStackAdded(const FSigilItemStack& Stack)
{
	check(Stack.IsValidStack())
	if (OwningInventory)
	{
		StackToIdxMap.Add(Stack.Id, Stack.Index);
		FSigilInventoryStackUpdateMessage Message;
		Message.Inventory = OwningInventory;
		Message.ChangeType = ESigilItemStackChangeType::WasAdded;
		Message.CollectionId = CollectionId;
		Message.Instance = Stack.Item;
		Message.StackId = Stack.Id;
		Message.NewCount = Stack.Amount;
		Message.Delta = Stack.Amount;
		OwningInventory->OnInventoryStackUpdate.Broadcast(Message);

		SIGIL_INVENTORY_CLOG(Verbose, "added item stack:item(%s),amount(%d)", *Stack.Item->GetDefinition()->GetName(), Stack.Amount)
	}
}

void USigilItemCollection::OnItemStackRemoved(const FSigilItemStack& Stack)
{
	if (OwningInventory)
	{
		StackToIdxMap.Remove(Stack.Id);
		FSigilInventoryStackUpdateMessage Message;
		Message.Inventory = OwningInventory;
		Message.ChangeType = ESigilItemStackChangeType::WasRemoved;
		Message.CollectionId = CollectionId;
		Message.Instance = Stack.Item;
		Message.StackId = Stack.Id;
		Message.NewCount = 0;
		Message.Delta = -Stack.LastObservedAmount;
		OwningInventory->OnInventoryStackUpdate.Broadcast(Message);

		SIGIL_INVENTORY_CLOG(Verbose, "removed item stack:item(%s),amount(%d)", *Stack.Item->GetDefinition()->GetName(), Stack.Amount)
	}
}

void USigilItemCollection::OnItemStackUpdated(const FSigilItemStack& Stack)
{
	if (OwningInventory)
	{
		if (StackToIdxMap.Contains(Stack.Id))
		{
			StackToIdxMap[Stack.Id] = Stack.Index;
		}
		FSigilInventoryStackUpdateMessage Message;
		Message.Inventory = OwningInventory;
		Message.ChangeType = ESigilItemStackChangeType::Changed;
		Message.CollectionId = CollectionId;
		Message.Instance = Stack.Item;
		Message.StackId = Stack.Id;
		Message.NewCount = Stack.Amount;
		Message.Delta = Stack.Amount - Stack.LastObservedAmount;
		OwningInventory->OnInventoryStackUpdate.Broadcast(Message);
		SIGIL_INVENTORY_CLOG(Verbose, "updated item stack:item(%s),amount(%d)", *Stack.Item->GetDefinition()->GetName(), Stack.Amount)
	}
}

void USigilItemCollection::ProcessPendingItemStacks()
{
	if (bInitialized)
	{
		TArray<FGuid> Added;
		for (const TPair<FGuid, FSigilItemStack>& Pending : PendingItemStacks)
		{
			if (Pending.Value.IsValidStack())
			{
				Added.AddUnique(Pending.Key);
			}
		}
		for (int32 i = 0; i < Added.Num(); i++)
		{
			FGuid AddedStackId = Added[i];
			const FSigilItemStack& AddedStack = PendingItemStacks[AddedStackId];
			OnItemStackAdded(AddedStack);
			SIGIL_INVENTORY_CLOG(Verbose, "added item stack:item(%s),amount(%d) from pending list.", *AddedStack.Item->GetDefinition()->GetName(), AddedStack.Amount)
			PendingItemStacks.Remove(AddedStackId);
		}
	}
}

void USigilItemCollection::SetDefinition(const USigilItemCollectionDefinition* NewDefinition)
{
	check(OwningInventory != nullptr);
	check(NewDefinition != nullptr);
	if (!bInitialized)
	{
		Definition = NewDefinition;
		CollectionTag = Definition->CollectionTag;
		bInitialized = true;
	}
}

void USigilItemCollection::SetCollectionTag(FGameplayTag NewTag)
{
	CollectionTag = NewTag;
}

void USigilItemCollection::SetCollectionId(FGuid NewId)
{
	if (!CollectionId.IsValid())
	{
		CollectionId = NewId;
	}
}

FSigilItemInfo USigilItemCollection::AddInternal(const FSigilItemInfo& ItemInfo)
{
	bool bFound = false;
	FSigilItemStack AddedItemStack;

	if (!ItemInfo.Item->IsUnique())
	{
		// First, trying to add to existing stack.
		int32 TargetStackIdx = Container.IndexOfById(ItemInfo.StackId);
		if (TargetStackIdx != INDEX_NONE)
		{
			check(Container.Stacks.IsValidIndex(TargetStackIdx))
			const FSigilItemStack& ExistingStack = Container.Stacks[TargetStackIdx];
			if (ExistingStack.Collection == this && ExistingStack.Item == ItemInfo.Item)
			{
				UpdateItemStackAmountAtIndex(TargetStackIdx, ItemInfo.Amount + ExistingStack.Amount);
				AddedItemStack = ExistingStack;
				bFound = true;
			}
		}

		if (!bFound)
		{
			for (int32 i = 0; i < Container.Stacks.Num(); i++)
			{
				if (CanItemStack(ItemInfo, Container.Stacks[i]) == false)
				{
					continue;
				}

				UpdateItemStackAmountAtIndex(i, ItemInfo.Amount + Container.Stacks[i].Amount);
				AddedItemStack = Container.Stacks[i];
				bFound = true;
				break;
			}
		}
	}

	//新增.
	if (!bFound)
	{
		AddedItemStack.Initialize(FGuid::NewGuid(), ItemInfo.Item, ItemInfo.Amount, this, ItemInfo.Index);
		AddItemStack(AddedItemStack);
	}
	FSigilItemInfo AddedItemInfo = FSigilItemInfo(ItemInfo.Item, ItemInfo.Amount, this, AddedItemStack.Id);
	return AddedItemInfo;
}

void USigilItemCollection::HandleItemOverflow(const FSigilItemInfo& OriginalItemInfo, const FSigilItemInfo& ItemInfoAdded)
{
	FSigilItemInfo RejectedItemInfo = FSigilItemInfo(OriginalItemInfo.Amount - ItemInfoAdded.Amount, OriginalItemInfo);

	FSigilItemInfo ReturnedItemInfo;
	if (Definition->OverflowOptions.bReturnOverflow)
	{
		if (OriginalItemInfo.ItemCollection != nullptr && OriginalItemInfo.ItemCollection != this)
		{
			ReturnedItemInfo = OriginalItemInfo.ItemCollection->AddItem(RejectedItemInfo);
		}
	}

	if (Definition->OverflowOptions.bSendRejectedMessage)
	{
		FSigilInventoryAddItemInfoRejectedMessage Message;
		Message.Inventory = OwningInventory;
		Message.Collection = this;
		Message.OriginalItemInfo = OriginalItemInfo;
		Message.ItemInfoAdded = ItemInfoAdded;
		Message.RejectedItemInfo = RejectedItemInfo;
		Message.ReturnedItemInfo = ReturnedItemInfo;
		OwningInventory->OnInventoryAddItemInfo_Rejected.Broadcast(Message);
	}
	SIGIL_INVENTORY_CLOG(Warning, "try add %s, added %d, rejected %d, returned:%d", *OriginalItemInfo.GetDebugString(), ItemInfoAdded.Amount, RejectedItemInfo.Amount, ReturnedItemInfo.Amount);
}

FSigilItemInfo USigilItemCollection::RemoveInternal(const FSigilItemInfo& ItemInfo)
{
	int32 Removed = 0;

	FSigilItemStack ItemStackToRemove;

	{
		int32 Idx = Container.IndexOfByIds(ItemInfo.StackId, ItemInfo.Item->GetItemId());

		if (Idx != INDEX_NONE)
		{
			ItemStackToRemove = SimpleInternalItemRemove(ItemInfo, Removed, Idx);
		}
	}

	//如果已经移除的数量未达到指定移除的数量，就继续移除。
	if (Removed < ItemInfo.Amount)
	{
		TArray<FSigilItemStack> TempStacks = Container.Stacks;
		for (int32 i = TempStacks.Num() - 1; i >= 0; i--)
		{
			if (TempStacks[i].Item == nullptr || TempStacks[i].Item->GetItemId() != ItemInfo.Item->GetItemId())
			{
				continue;
			}
			ItemStackToRemove = SimpleInternalItemRemove(ItemInfo, Removed, i);
			if (Removed >= ItemInfo.Amount)
			{
				break;
			}
		}
	}

	const FSigilItemInfo RemovedItemInfo = FSigilItemInfo(ItemInfo.Item, Removed, this, ItemStackToRemove.Id);

	if (Removed == 0)
	{
		return RemovedItemInfo;
	}

	return RemovedItemInfo;
}

FSigilItemStack USigilItemCollection::SimpleInternalItemRemove(const FSigilItemInfo& ItemInfo, int32& AlreadyRemoved, int32 StackIndex)
{
	check(Container.Stacks.IsValidIndex(StackIndex));
	const FSigilItemStack FoundStack = Container.Stacks[StackIndex];
	int32 RemainingToRemove = ItemInfo.Amount - AlreadyRemoved; //我要减10个，已经减了4个，还要减6个
	int32 NewAmount = FoundStack.Amount - RemainingToRemove;
	if (NewAmount <= 0) //如果栈里还有3个，不够减，那么已经减去的数量就成了7个，然后栈被移除。
	{
		AlreadyRemoved += FoundStack.Amount;
		RemoveItemStackAtIndex(StackIndex);
	}
	else //如果栈里还有7个，够减，那么已经减去的数量为10,栈还剩下7-6=1个。
	{
		AlreadyRemoved += RemainingToRemove;
		UpdateItemStackAmountAtIndex(StackIndex, NewAmount);
	}
	return FoundStack;
}

FSigilItemInfo USigilItemCollection::GiveItem(const FSigilItemInfo& ItemInfo, USigilItemCollection* ItemCollection)
{
	if (OwningInventory->GetOwnerRole() != ROLE_Authority)
	{
		SIGIL_INVENTORY_CLOG(Warning, "Has no authority")
		return FSigilItemInfo::None;
	}
	if (!ItemInfo.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid ItemInfo to give.")
		return FSigilItemInfo::None;
	}

	if (ItemInfo.Item->GetOwningInventory() == ItemCollection->GetOwningInventory())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Item:%s already belongs to this inventory.", *ItemInfo.GetDebugString());
		return FSigilItemInfo::None;
	}

	FSigilItemInfo RemovedItemInfo = RemoveItem(ItemInfo);

	FSigilItemInfo ItemInfoToAdd = FSigilItemInfo(ItemInfo.Item, RemovedItemInfo.Amount, this);
	FSigilItemInfo GivenItemInfo = ItemCollection->AddItem(ItemInfoToAdd);
	if (GivenItemInfo.Amount != RemovedItemInfo.Amount)
	{
		// Failed to add so add it back to the previous collection.
	}
	return GivenItemInfo;
}

void USigilItemCollection::ServerGiveItem_Implementation(const FSigilItemInfo& ItemInfo, USigilItemCollection* ItemCollection)
{
	GiveItem(ItemInfo, ItemCollection);
}

void USigilItemCollection::GiveAllItems(USigilItemCollection* OtherItemCollection)
{
	for (int i = Container.Stacks.Num() - 1; i >= 0; i--)
	{
		auto& itemStack = Container.Stacks[i];
		GiveItem(FSigilItemInfo(itemStack), OtherItemCollection);
	}
}

void USigilItemCollection::ServerGiveAllItems_Implementation(USigilItemCollection* OtherItemCollection)
{
	GiveAllItems(OtherItemCollection);
}

int32 USigilItemCollection::GetItemAmountFittingInLimitedAdditionalStacks(const FSigilItemInfo& ItemInfo, int32 AvailableAdditionalStacks) const
{
	if (ItemInfo.Item->IsUnique())
	{
		//预计还有5个，而要添加10个，那么就只能5个。
		//预计还有10个，而要添加6个，那么就能放6个。
		return FMath::Min(ItemInfo.Amount, AvailableAdditionalStacks);
	}

	//满了，且没有已经存在的Stack可以叠上去。
	if (AvailableAdditionalStacks == 0 && !HasItem(ItemInfo.Item, 1))
	{
		return 0;
	}
	return ItemInfo.Amount;
}
