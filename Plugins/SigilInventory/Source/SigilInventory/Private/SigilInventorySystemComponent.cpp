// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilInventorySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "SigilInventoryTags.h"
#include "SigilInventorySubsystem.h"
#include "Items/SigilItemInstance.h"
#include "SigilItemCollection.h"
#include "Net/UnrealNetwork.h"
#include "SigilItemSlotCollection.h"
#include "SigilCurrencySystemComponent.h"
#include "SigilInventoryLogChannels.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventorySystemComponent)

USigilInventorySystemComponent::USigilInventorySystemComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), CollectionContainer(this)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bWantsInitializeComponent = true;
}

bool USigilInventorySystemComponent::FindInventorySystemComponent(const AActor* Actor, USigilInventorySystemComponent*& Inventory)
{
	Inventory = GetInventorySystemComponent(Actor);

	return IsValid(Inventory);
}

USigilInventorySystemComponent* USigilInventorySystemComponent::GetInventorySystemComponent(const AActor* Actor)
{
	USigilInventorySystemComponent* Inventory = Actor ? Actor->FindComponentByClass<USigilInventorySystemComponent>() : nullptr;
	if (!IsValid(Inventory))
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (APlayerState* PS = Cast<APlayerState>(Pawn->GetPlayerState()))
			{
				Inventory = PS->FindComponentByClass<USigilInventorySystemComponent>();
			}
		}
	}
	return Inventory;
}

USigilInventorySystemComponent* USigilInventorySystemComponent::FindInventorySystemComponent(const AActor* Actor)
{
	return GetInventorySystemComponent(Actor);
}

void USigilInventorySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bInventorySystemInitialized);
	DOREPLIFETIME(ThisClass, CollectionContainer);
}

bool USigilInventorySystemComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
}

void USigilInventorySystemComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessPendingCollections();
}

bool USigilInventorySystemComponent::CanAddItem(const FSigilItemInfo& InItemInfo, FSigilItemInfo& OutItemInfo) const
{
	if (USigilItemCollection* Collection = DetermineTargetCollection(InItemInfo))
	{
		return Collection->CanAddItem(InItemInfo, OutItemInfo);
	}
	return false;
}

USigilItemCollection* USigilInventorySystemComponent::DetermineTargetCollection(const FSigilItemInfo& ItemInfo) const
{
	USigilItemCollection* Collection = nullptr;

	if (ItemInfo.CollectionId.IsValid())
	{
		Collection = GetCollectionById(ItemInfo.CollectionId);
	}
	if (Collection == nullptr && ItemInfo.CollectionTag.IsValid())
	{
		Collection = GetCollectionByTag(ItemInfo.CollectionTag);
	}
	if (Collection == nullptr)
	{
		Collection = GetDefaultCollection();
	}
	return Collection;
}

FSigilItemInfo USigilInventorySystemComponent::AddItem(const FSigilItemInfo& ItemInfo)
{
	if (USigilItemCollection* Collection = DetermineTargetCollection(ItemInfo))
	{
		const FSigilItemInfo AddedItem = Collection->AddItem(ItemInfo);
		return AddedItem;
	}
	return FSigilItemInfo::None;
}

TArray<FSigilItemInfo> USigilInventorySystemComponent::AddItems(TArray<FSigilItemInfo> ItemInfos)
{
	TArray<FSigilItemInfo> AddedItemInfos;
	for (const FSigilItemInfo& ItemInfo : ItemInfos)
	{
		AddedItemInfos.Add(AddItem(ItemInfo));
	}
	return AddedItemInfos;
}

FSigilItemInfo USigilInventorySystemComponent::AddItemByDefinition(const FGameplayTag CollectionTag, TSoftObjectPtr<USigilItemDefinition> ItemDefinition, const int32 NewAmount)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Has no authority!")
		return FSigilItemInfo::None;
	}
	if (USigilItemInstance* NewItem = USigilInventorySubsystem::Get(GetWorld())->CreateItem(GetOwner(), ItemDefinition))
	{
		return AddItem(FSigilItemInfo(NewItem, NewAmount, CollectionTag));
	}
	return FSigilItemInfo::None;
}

void USigilInventorySystemComponent::ServerAddItemByDefinition_Implementation(const FGameplayTag CollectionTag, const TSoftObjectPtr<USigilItemDefinition>& ItemDefinition, const int32 NewAmount)
{
	AddItemByDefinition(CollectionTag, ItemDefinition, NewAmount);
}

bool USigilInventorySystemComponent::CanMoveItem(const FSigilItemInfo& ItemInfo) const
{
	if (!ItemInfo.IsValid() || ItemInfo.Item->GetOwningCollection() == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Verbose, "item:%s has no source collection!", *ItemInfo.GetDebugString())
		return false;
	}

	USigilItemCollection* SrcCollection = ItemInfo.Item->GetOwningCollection();

	if (ItemInfo.Item->GetOwningInventory() != this)
	{
		SIGIL_INVENTORY_CLOG(Warning, "item:%s not belong to this inventory.", *ItemInfo.GetDebugString())
		return false;
	}

	USigilItemCollection* TargetCollection = DetermineTargetCollection(ItemInfo);

	if (TargetCollection == nullptr || TargetCollection == SrcCollection)
	{
		SIGIL_INVENTORY_CLOG(Warning, "no dest collection for item:%s to move", *ItemInfo.GetDebugString())
		return false;
	}
	if (TargetCollection == SrcCollection)
	{
		SIGIL_INVENTORY_CLOG(Warning, "item:%s already in same collection.", *ItemInfo.GetDebugString())
		return false;
	}

	return true;
}

void USigilInventorySystemComponent::MoveItem(const FSigilItemInfo& ItemInfo)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Has no authority!")
		return;
	}

	if (!CanMoveItem(ItemInfo))
	{
		return;
	}

	USigilItemInstance* Item = ItemInfo.Item;
	USigilItemCollection* SrcCollection = Item->GetOwningCollection();

	USigilItemCollection* DestCollection = DetermineTargetCollection(ItemInfo);

	// This action used to give the item one way and then the other.
	// The action now removes the item, before it adds it to the other collection to allow restrictions to work properly
	FSigilItemInfo OriginalItem = SrcCollection->RemoveItem(ItemInfo);
	FSigilItemInfo MovedItemInfo = ItemInfo.None;

	if (USigilItemSlotCollection* DestSlotCollection = Cast<USigilItemSlotCollection>(DestCollection))
	{
		int32 slotIndex = ItemInfo.StackId.IsValid() ? DestSlotCollection->StackIdToSlotIndex(ItemInfo.StackId) : INDEX_NONE;

		if (slotIndex == INDEX_NONE) // fallback to suitable index.
		{
			slotIndex = DestSlotCollection->GetTargetSlotIndex(Item);
		}
		if (slotIndex != INDEX_NONE)
		{
			FSigilItemInfo previousItemInSlot = DestSlotCollection->GetItemInfoAtSlot(slotIndex);

			if (previousItemInSlot.Item != nullptr)
			{
				// If the previous item is stackable don't remove it.
				if (previousItemInSlot.Item->StackableEquivalentTo(OriginalItem.Item))
				{
					previousItemInSlot = ItemInfo.None;
				}
				else
				{
					previousItemInSlot = DestSlotCollection->RemoveItem(slotIndex);
				}
			}

			MovedItemInfo = DestSlotCollection->AddItem(OriginalItem, slotIndex);

			if (previousItemInSlot.Item != nullptr)
			{
				SrcCollection->AddItem(previousItemInSlot);
			}
		}
	}
	else
	{
		MovedItemInfo = DestCollection->AddItem(OriginalItem);
	}

	// Not all the item was added, return the items to the default collection.
	if (MovedItemInfo.Amount != OriginalItem.Amount)
	{
		int32 AmountToReturn = OriginalItem.Amount - MovedItemInfo.Amount;
		SrcCollection->AddItem(FSigilItemInfo(AmountToReturn, OriginalItem));
	}
}

void USigilInventorySystemComponent::ServerMoveItem_Implementation(const FSigilItemInfo& ItemInfo)
{
	MoveItem(ItemInfo);
}

bool USigilInventorySystemComponent::CanRemoveItem(const FSigilItemInfo& ItemInfo) const
{
	if (ItemInfo.IsValid())
	{
		FSigilItemInfo ItemInfoToRemove;
		return ItemInfo.Item->GetOwningCollection()->RemoveItemCondition(ItemInfo, ItemInfoToRemove);
	}
	return false;
}

FSigilItemInfo USigilInventorySystemComponent::RemoveItem(const FSigilItemInfo& ItemInfo)
{
	if (ItemInfo.Item->GetOwningCollection() != nullptr && ItemInfo.Item->GetOwningCollection()->GetOwningInventory() == this)
	{
		return ItemInfo.Item->GetOwningCollection()->RemoveItem(ItemInfo);
	}
	return DetermineTargetCollection(ItemInfo)->RemoveItem(ItemInfo);
}

void USigilInventorySystemComponent::ServerRemoveItem_Implementation(FSigilItemInfo ItemInfo)
{
	check(GetOwnerRole() == ROLE_Authority)
	RemoveItem(ItemInfo);
}

FSigilItemInfo USigilInventorySystemComponent::RemoveItemByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, const int32 Amount)
{
	if (ItemDefinition.IsNull() || Amount == 0)
	{
		return FSigilItemInfo::None;
	}

	// The item can be in multiple stacks, for example if it is Unique.

	int32 AmountRemoved = 0;
	int32 AmountToRemove = Amount;
	FSigilItemInfo LastItemInfoRemoved = FSigilItemInfo::None;

	for (int32 i = 0; i < Amount; i++)
	{
		FSigilItemInfo ItemInfo;
		if (!GetItemInfoByDefinition(ItemDefinition, ItemInfo))
		{
			break;
		}

		LastItemInfoRemoved = RemoveItem(ItemInfo);

		AmountRemoved += LastItemInfoRemoved.Amount;
		AmountToRemove = Amount - AmountRemoved;

		if (AmountToRemove == 0)
		{
			break;
		}
	}

	return FSigilItemInfo(AmountRemoved, LastItemInfoRemoved);
}

void USigilInventorySystemComponent::RemoveAllItems(bool RemoveItemsFromIgnoredCollections, bool DisableEventsWhileRemoving)
{
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		TObjectPtr<USigilItemCollection> ItemCollection = CollectionContainer.Entries[i].Instance;
		if (RemoveItemsFromIgnoredCollections == false && IsIgnoredCollection(ItemCollection))
		{
			continue;
		}
		ItemCollection->RemoveAll();
	}
}

int32 USigilInventorySystemComponent::GetItemAmount(USigilItemInstance* Item, bool SimilarItem) const
{
	if (Item == nullptr)
	{
		return 0;
	}

	int32 Amount = 0;

	for (int i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}

		Amount += CollectionContainer.Entries[i].Instance->GetItemAmount(Item);
	}

	return Amount;
}

int32 USigilInventorySystemComponent::GetItemAmountByDefinition(TSoftObjectPtr<USigilItemDefinition> ItemDefinition, bool Unique) const
{
	int32 Amount = 0;
	for (int i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}

		Amount += CollectionContainer.Entries[i].Instance->GetItemAmount(ItemDefinition, Unique);
	}

	return Amount;
}

bool USigilInventorySystemComponent::GetItemInfoInCollection(USigilItemInstance* Item, const FGameplayTag CollectionTag, FSigilItemInfo& OutItemInfo) const
{
	if (USigilItemCollection* Collection = GetCollectionByTag(CollectionTag))
	{
		return Collection->GetItemInfo(Item, OutItemInfo);
	}
	return false;
}

bool USigilInventorySystemComponent::FindItemInfoInCollection(USigilItemInstance* Item, const FGameplayTag CollectionTag, FSigilItemInfo& OutItemInfo) const
{
	return GetItemInfoInCollection(Item, CollectionTag, OutItemInfo);
}

bool USigilInventorySystemComponent::GetAllItemInfosInCollection(const FGameplayTag CollectionTag, TArray<FSigilItemInfo>& OutItemInfos) const
{
	if (USigilItemCollection* Collection = GetCollectionByTag(CollectionTag))
	{
		OutItemInfos = Collection->GetAllItemInfos();
		return OutItemInfos.Num() != 0;
	}
	return false;
}

bool USigilInventorySystemComponent::FindAllItemInfosInCollection(const FGameplayTag CollectionTag, TArray<FSigilItemInfo>& OutItemInfos) const
{
	return GetAllItemInfosInCollection(CollectionTag, OutItemInfos);
}

bool USigilInventorySystemComponent::GetItemInfo(USigilItemInstance* Item, FSigilItemInfo& ItemInfo) const
{
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}
		if (CollectionContainer.Entries[i].Instance->GetItemInfo(Item, ItemInfo))
		{
			return true;
		}
	}
	return false;
}

bool USigilInventorySystemComponent::FindItemInfo(USigilItemInstance* Item, FSigilItemInfo& ItemInfo) const
{
	return GetItemInfo(Item, ItemInfo);
}

bool USigilInventorySystemComponent::GetItemInfoByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, FSigilItemInfo& OutItemInfo) const
{
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}
		if (CollectionContainer.Entries[i].Instance->GetItemInfoByDefinition(ItemDefinition, OutItemInfo))
		{
			return OutItemInfo.IsValid();
		}
	}

	return false;
}

bool USigilInventorySystemComponent::FindItemInfoByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, FSigilItemInfo& OutItemInfo) const
{
	return GetItemInfoByDefinition(ItemDefinition, OutItemInfo);
}

bool USigilInventorySystemComponent::GetItemInfosByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TArray<FSigilItemInfo>& OutItemInfos) const
{
	if (ItemDefinition.IsNull())
	{
		return false;
	}

	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}
		TArray<FSigilItemInfo> ItemInfos;
		if (CollectionContainer.Entries[i].Instance->GetItemInfosByDefinition(ItemDefinition, ItemInfos))
		{
			ItemInfos.Append(ItemInfos);
		}
	}

	return !OutItemInfos.IsEmpty();
}

bool USigilInventorySystemComponent::FindItemInfosByDefinition(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TArray<FSigilItemInfo>& OutItemInfos) const
{
	return GetItemInfosByDefinition(ItemDefinition, OutItemInfos);
}

TArray<FSigilItemInfo> USigilInventorySystemComponent::GetItemInfos() const
{
	TArray<FSigilItemInfo> Ret;
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (IsIgnoredCollection(CollectionContainer.Entries[i].Instance))
		{
			continue;
		}

		TArray<FSigilItemInfo> AllItemInfos = CollectionContainer.Entries[i].Instance->GetAllItemInfos();
		Ret.Append(AllItemInfos);
	}
	return Ret;
}

bool USigilInventorySystemComponent::HasEnoughItem(const TSoftObjectPtr<USigilItemDefinition> ItemDefinition, int32 Amount) const
{
	TArray<FSigilItemInfo> OutItemInfos;
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		USigilItemCollection* Collection = CollectionContainer.Entries[i].Instance;
		TArray<FSigilItemInfo> OutItemInfosTemp;
		Collection->GetItemInfosByDefinition(ItemDefinition, OutItemInfosTemp);
		OutItemInfos.Append(OutItemInfosTemp);
	}
	if (OutItemInfos.Num() >= Amount)
	{
		return true;
	}
	for (const auto& It : OutItemInfos)
	{
		if (It.Amount >= Amount)
		{
			return true;
		}
	}
	return false;
}

TArray<USigilItemCollection*> USigilInventorySystemComponent::GetItemCollections() const
{
	TArray<USigilItemCollection*> Ret;
	Ret.Reserve(CollectionContainer.Entries.Num());
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		Ret.Add(CollectionContainer.Entries[i].Instance);
	}
	return Ret;
}

bool USigilInventorySystemComponent::IsDefaultCollectionCreated() const
{
	for (auto& Definition : CollectionDefinitions)
	{
		bool bFound = false;
		for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
		{
			const FSigilCollectionEntry& Entry = CollectionContainer.Entries[i];
			if (Entry.IsValidEntry() && Entry.Definition == Definition && Entry.Instance->GetCollectionTag() == Definition->CollectionTag)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			return false;
		}
	}
	return true;
}

USigilItemCollection* USigilInventorySystemComponent::GetDefaultCollection() const
{
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		auto& Instance = CollectionContainer.Entries[i].Instance;
		if (Instance && Instance->GetCollectionTag().MatchesTagExact(SigilCollectionTags::Main))
		{
			return Instance;
		}
	}
	return nullptr;
}

int32 USigilInventorySystemComponent::GetCollectionCount() const
{
	return CollectionContainer.Entries.Num();
}

USigilItemCollection* USigilInventorySystemComponent::GetCollectionByTag(const FGameplayTag CollectionTag) const
{
	if (!CollectionTag.IsValid())
	{
		return nullptr;
	}
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		auto& Instance = CollectionContainer.Entries[i].Instance;

		if (Instance && Instance->GetCollectionTag() == CollectionTag)
		{
			return Instance;
		}
	}
	return nullptr;
}

USigilItemCollection* USigilInventorySystemComponent::GetCollectionByTags(FGameplayTagContainer Tags)
{
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		if (CollectionContainer.Entries[i].Instance && CollectionContainer.Entries[i].Instance->GetCollectionTag().MatchesAnyExact(Tags))
		{
			return CollectionContainer.Entries[i].Instance;
		}
	}
	return nullptr;
}

USigilItemCollection* USigilInventorySystemComponent::GetCollectionById(FGuid CollectionId) const
{
	if (CollectionIdToInstanceMap.Contains(CollectionId))
	{
		return CollectionIdToInstanceMap[CollectionId];
	}
	return nullptr;
}

USigilItemCollection* USigilInventorySystemComponent::GetTypedCollectionByTag(const FGameplayTag CollectionTag, TSubclassOf<USigilItemCollection> DesiredClass) const
{
	if (UClass* RealClass = DesiredClass)
	{
		if (USigilItemCollection* Collection = GetCollectionByTag(CollectionTag))
		{
			if (Collection->GetClass() == RealClass)
			{
				return Collection;
			}
		}
	}
	return nullptr;
}

bool USigilInventorySystemComponent::FindTypedCollectionByTag(const FGameplayTag CollectionTag, TSubclassOf<USigilItemCollection> DesiredClass, USigilItemCollection*& OutCollection)
{
	if (UClass* RealClass = DesiredClass)
	{
		if (USigilItemCollection* Collection = GetCollectionByTag(CollectionTag))
		{
			if (Collection->GetClass() == RealClass)
			{
				OutCollection = Collection;
				return true;
			}
		}
	}
	return false;
}

USigilItemCollection* USigilInventorySystemComponent::AddCollectionByDefinition(TSoftObjectPtr<const USigilItemCollectionDefinition> CollectionDefinition)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Has no authority!")
		return nullptr;
	}

	if (CollectionDefinition.IsNull())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Try to add collection with invalid definition.")
		return nullptr;
	}

	const USigilItemCollectionDefinition* Definition = CollectionDefinition.LoadSynchronous();
	if (!IsValid(Definition))
	{
		SIGIL_INVENTORY_CLOG(Warning, "Try to add collection with invalid definition.")
		return nullptr;
	}

	USigilItemCollection* NewCollection = CreateCollectionInstance(Definition);

	check(NewCollection)

	FSigilCollectionEntry NewEntry;
	NewEntry.Id = FGuid::NewGuid();
	NewEntry.Instance = NewCollection;
	NewEntry.Definition = Definition;
	if (AddCollectionEntry(NewEntry))
	{
		return NewEntry.Instance;
	}
	return nullptr;
}

bool USigilInventorySystemComponent::AddCollectionEntry(const FSigilCollectionEntry& NewEntry)
{
	if (!NewEntry.IsValidEntry())
	{
		return false;
	}
	if (NewEntry.Instance && NewEntry.Instance->IsInitialized())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Try to add already initialized collection.")
		return false;
	}

	FSigilCollectionEntry& AddedEntry = CollectionContainer.Entries.Add_GetRef(NewEntry);
	if (GetOwnerRole() >= ROLE_Authority && IsReadyForReplication())
	{
		if (!IsReplicatedSubObjectRegistered(AddedEntry.Instance))
		{
			AddReplicatedSubObject(AddedEntry.Instance);
		}
	}
	OnCollectionAdded(NewEntry);
	CollectionContainer.MarkItemDirty(AddedEntry);
	GetOwner()->ForceNetUpdate();
	return true;
}

void USigilInventorySystemComponent::RemoveCollectionEntry(int32 Idx)
{
	if (CollectionContainer.Entries.IsValidIndex(Idx))
	{
		const FSigilCollectionEntry& Entry = CollectionContainer.Entries[Idx];
		if (IsValid(Entry.Instance) && Entry.Instance->IsInitialized())
		{
			OnCollectionRemoved(Entry);
			RemoveReplicatedSubObject(Entry.Instance);
			CollectionContainer.Entries.RemoveAt(Idx);
			CollectionContainer.MarkArrayDirty();
		}
	}
}

USigilItemCollection* USigilInventorySystemComponent::CreateCollectionInstance(const USigilItemCollectionDefinition* CollectionDefinition)
{
	if (!IsValid(CollectionDefinition))
	{
		SIGIL_INVENTORY_CLOG(Warning, "Try to add collection with invalid definition.")
		return nullptr;
	}
	TSubclassOf<USigilItemCollection> CollectionClass = CollectionDefinition->GetCollectionInstanceClass();
	if (CollectionClass == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "definition(%s) doesn't specify valid item collection class.", *CollectionDefinition->GetName())
		return nullptr;
	}
	USigilItemCollection* NewCollection = NewObject<USigilItemCollection>(GetOwner(), CollectionClass);
	if (NewCollection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "failed to create instance of %s", *GetNameSafe(CollectionDefinition))
		return nullptr;
	}
	return NewCollection;
}

USigilCurrencySystemComponent* USigilInventorySystemComponent::GetCurrencySystem() const
{
	return CurrencySystem;
}


void USigilInventorySystemComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing Item Collections.
	if (IsUsingRegisteredSubObjectList())
	{
		for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
		{
			auto& Instance = CollectionContainer.Entries[i].Instance;
			if (IsValid(Instance))
			{
				if (!IsReplicatedSubObjectRegistered(Instance))
				{
					AddReplicatedSubObject(Instance);
				}
				for (const FSigilItemStack& ItemStack : Instance->GetAllItemStacks())
				{
					if (ItemStack.Item == nullptr)
					{
						continue;
					}
					if (!IsReplicatedSubObjectRegistered(ItemStack.Item))
					{
						AddReplicatedSubObject(ItemStack.Item);
					}
				}
			}
		}
	}
}

void USigilInventorySystemComponent::LoadDefaultLoadouts()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	for (int32 i = 0; i < DefaultLoadouts.Num(); i++)
	{
		const FSigilDefaultLoadout& Loadout = DefaultLoadouts[i];
		if (!Loadout.Tag.IsValid())
		{
			continue;
		}
		for (const FSigilItemDefinitionAmount& DefaultItem : Loadout.DefaultItems)
		{
			if (DefaultItem.Amount <= 0)
			{
				continue;
			}

			if (DefaultItem.Definition.IsNull())
			{
				continue;
			}
			USigilItemInstance* Item = USigilInventorySubsystem::Get(GetWorld())->CreateItem(Cast<AActor>(GetOuter()), DefaultItem.Definition);
			if (Item == nullptr)
			{
				SIGIL_INVENTORY_CLOG(Warning, "Failed to add DefaultLoadout{Definition:%s}", *DefaultItem.Definition.ToString())
				continue;
			}
			FSigilItemInfo Info;
			Info.Item = Item;
			Info.Amount = DefaultItem.Amount;
			Info.CollectionTag = Loadout.Tag;
			AddItem(Info);
		}
	}
}

void USigilInventorySystemComponent::ServerLoadDefaultLoadouts_Implementation()
{
	ServerLoadDefaultLoadouts();
}

void USigilInventorySystemComponent::OnInventorySystemInitialized_Implementation()
{
	OnInventorySystemInitializedEvent.Broadcast();

	TArray<FSigilInventorySystem_Initialized_DynamicEvent> Delegates = InitializedDelegates;
	for (FSigilInventorySystem_Initialized_DynamicEvent Delegate : Delegates)
	{
		Delegate.ExecuteIfBound();
	}
	InitializedDelegates.Empty();
}

void USigilInventorySystemComponent::InitializeInventorySystem()
{
	if (bInventorySystemInitialized || !GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "already initialized or has no authority!");
		return;
	}
	CollectionIdToInstanceMap.Empty();
	PendingCollections.Empty();
	for (int32 i = 0; i < CollectionDefinitions.Num(); i++)
	{
		if (CollectionDefinitions[i] == nullptr || !CollectionDefinitions[i]->CollectionTag.IsValid())
		{
			SIGIL_INVENTORY_CLOG(Error, "default collection definition at index(%d) is invalid or mising collection tag!", i);
			continue;
		}
		if (AddCollectionByDefinition(CollectionDefinitions[i]) == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Warning, "failed to initialize collection:%s", *GetNameSafe(CollectionDefinitions[i]))
		}
	}
	LoadDefaultLoadouts();

	bInventorySystemInitialized = true;
	OnInventorySystemInitialized();
}

void USigilInventorySystemComponent::InitializeInventorySystemWithRecord(const FSigilInventoryRecord& InventoryRecord)
{
	if (bInventorySystemInitialized || !GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "already initialized or has no authority!");
		return;
	}

	if (!InventoryRecord.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "provided invalid records");
		return;
	}

	USigilInventorySubsystem* Subsystem = USigilInventorySubsystem::Get(GetWorld());
	if (Subsystem == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Verbose, "missing inventory sub system");
		return;
	}

	CollectionIdToInstanceMap.Empty();
	PendingCollections.Empty();

	ResetInventorySystem();

	Subsystem->DeserializeInventory(this, InventoryRecord);

	bInventorySystemInitialized = true;
	OnInventorySystemInitialized();
}

void USigilInventorySystemComponent::ResetInventorySystem()
{
	if (!bInventorySystemInitialized || !GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "not initialized or has no authority!");
		return;
	}

	//remove all items inside collections.
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		auto& Instance = CollectionContainer.Entries[i].Instance;
		if (IsValid(Instance) && Instance->IsInitialized())
		{
			Instance->RemoveAll();
		}
	}

	//remove all remaining collection itself.
	for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
	{
		RemoveCollectionEntry(i);
	}

	bInventorySystemInitialized = false;
}

bool USigilInventorySystemComponent::IsInventoryInitialized() const
{
	return bInventorySystemInitialized;
}

void USigilInventorySystemComponent::BindToInventorySystemInitialized(FSigilInventorySystem_Initialized_DynamicEvent Delegate)
{
	if (bInventorySystemInitialized)
	{
		Delegate.ExecuteIfBound();
	}
	else
	{
		InitializedDelegates.Add(Delegate);
	}
}

void USigilInventorySystemComponent::OnRegister()
{
	Super::OnRegister();
}

void USigilInventorySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return;
	}

	CollectionContainer.OwningComponent = this;

	if (!GetOwner()->IsUsingRegisteredSubObjectList())
	{
		SIGIL_INVENTORY_CLOG(Error, "requires enable bReplicateUsingRegisteredSubObjectList.")
	}

	CurrencySystem = USigilCurrencySystemComponent::GetCurrencySystemComponent(GetOwner());
}

// Called when the game starts
void USigilInventorySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bInitializeOnBeginplay && GetOwner()->HasAuthority())
	{
		InitializeInventorySystem();
	}
}

bool USigilInventorySystemComponent::IsIgnoredCollection(USigilItemCollection* ItemCollection) const
{
	if (ItemCollection == nullptr)
	{
		return true;
	}
	return IgnoredCollections.HasTagExact(ItemCollection->GetCollectionTag());
}

void USigilInventorySystemComponent::OnCollectionAdded(const FSigilCollectionEntry& Entry)
{
	check(IsValid(Entry.Instance) && Entry.Id.IsValid())
	CollectionIdToInstanceMap.Add(Entry.Id, Entry.Instance);
	Entry.Instance->SetInventory(this);
	Entry.Instance->SetCollectionId(Entry.Id);
	Entry.Instance->SetCollectionTag(Entry.Definition->CollectionTag);
	Entry.Instance->SetDefinition(Entry.Definition);
	OnCollectionAddedEvent.Broadcast(Entry.Instance);
	SIGIL_INVENTORY_CLOG(Verbose, "added collection:%s", *GetNameSafe(Entry.Instance->GetDefinition()))
}

void USigilInventorySystemComponent::OnCollectionRemoved(const FSigilCollectionEntry& Entry)
{
	check(IsValid(Entry.Instance) && Entry.Id.IsValid())

	OnCollectionRemovedEvent.Broadcast(Entry.Instance);
	// clear collection data.
	Entry.Instance->SetInventory(nullptr);
	// remove cache.
	if (CollectionIdToInstanceMap.Contains(Entry.Id))
	{
		CollectionIdToInstanceMap.Remove(Entry.Id);
	}
	Entry.Instance->PendingItemStacks.Empty();
	SIGIL_INVENTORY_CLOG(Verbose, "removed collection:%s", *GetNameSafe(Entry.Instance->GetDefinition()))
}

void USigilInventorySystemComponent::OnCollectionUpdated(const FSigilCollectionEntry& Entry)
{
}

void USigilInventorySystemComponent::ProcessPendingCollections()
{
	if (HasBegunPlay() && GetOwner() != nullptr)
	{
		TArray<FGuid> Added;
		for (const auto& Pending : PendingCollections)
		{
			if (Pending.Value.IsValidEntry())
			{
				Added.AddUnique(Pending.Key);
			}
		}

		for (int32 i = 0; i < Added.Num(); i++)
		{
			FGuid AddedIndex = Added[i];
			const FSigilCollectionEntry& AddedEntry = PendingCollections[AddedIndex];
			OnCollectionAdded(AddedEntry);
			SIGIL_INVENTORY_CLOG(Verbose, "added collection:%s from pending list.", *GetNameSafe(AddedEntry.Instance->GetDefinition()))
			PendingCollections.Remove(AddedIndex);
		}

		for (int32 i = 0; i < CollectionContainer.Entries.Num(); i++)
		{
			USigilItemCollection* Collection = CollectionContainer.Entries[i].Instance;
			if (IsValid(Collection) && Collection->IsInitialized())
			{
				Collection->ProcessPendingItemStacks();
			}
		}
	}
}

#if WITH_EDITOR

void USigilInventorySystemComponent::PostLoad()
{
	Super::PostLoad();
}

void USigilInventorySystemComponent::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);
}

EDataValidationResult USigilInventorySystemComponent::IsDataValid(FDataValidationContext& Context) const
{
	return Super::IsDataValid(Context);
}
#endif
