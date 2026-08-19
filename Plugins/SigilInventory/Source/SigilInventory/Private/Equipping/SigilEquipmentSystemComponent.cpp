// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilEquipmentSystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "SigilEquipmentInstance.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "SigilItemDefinition.h"
#include "SigilItemFragment_Equippable.h"
#include "SigilItemInstance.h"
#include "SigilItemSlotCollection.h"
#include "SigilInventoryLogChannels.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilEquipmentSystemComponent)


USigilEquipmentSystemComponent::USigilEquipmentSystemComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), Container(this)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bWantsInitializeComponent = true;
}

void USigilEquipmentSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Container);
	DOREPLIFETIME(ThisClass, bEquipmentSystemInitialized);
	DOREPLIFETIME_CONDITION(ThisClass, TargetCollectionDefinition, COND_OwnerOnly);
}

void USigilEquipmentSystemComponent::EquipItemToSlot(USigilItemInstance* Item, const FGameplayTag& SlotTag)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}

	if (!SlotTag.IsValid())
	{
		return;
	}

	UObject* EquipmentInstance = CreateEquipmentInstance(GetOwner(), Item);
	if (!IsValid(EquipmentInstance))
	{
		return;
	}

	FSigilEquipmentEntry NewEntry;
	NewEntry.EquippedSlot = SlotTag;
	NewEntry.Instance = EquipmentInstance;
	NewEntry.ItemInstance = Item;
	NewEntry.bActive = false;
	NewEntry.bPrevActive = false;

	//Auto activation.
	if (auto Equippable = NewEntry.ItemInstance->FindFragmentByClass<USigilItemFragment_Equippable>())
	{
		FGameplayTag MatchingGroup;
		int32 MatchingIdxInGroup;
		if (Equippable->bAutoActivate && CanActiveSlotInGroup(NewEntry.EquippedSlot, MatchingGroup, MatchingIdxInGroup))
		{
			NewEntry.bActive = true;
			GroupActiveIdxMap[MatchingGroup] = MatchingIdxInGroup;
		}
	}

	AddEquipmentEntry(NewEntry);
}

void USigilEquipmentSystemComponent::UnequipBySlot(FGameplayTag SlotTag)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	int32 Idx = Container.IndexOfBySlot(SlotTag);
	if (Idx != INDEX_NONE)
	{
		RemoveEquipmentEntry(Idx);
	}
}

void USigilEquipmentSystemComponent::UnequipByItem(const FGuid& ItemId)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	int32 Idx = Container.IndexOfByItemId(ItemId);
	if (Idx != INDEX_NONE)
	{
		RemoveEquipmentEntry(Idx);
	}
}

bool USigilEquipmentSystemComponent::OwnerHasAuthority() const
{
	AActor* Owner = GetOwner();
	return IsValid(Owner) && Owner->HasAuthority();
}

TArray<UObject*> USigilEquipmentSystemComponent::GetEquipments(TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const
{
	TArray<UObject*> Results;
	if (SlotQuery.IsEmpty())
	{
		return Results;
	}

	if (UClass* RealClass = InstanceType)
	{
		const TArray<FSigilEquipmentEntry>& MatchedEntries = Container.Entries.FilterByPredicate([&SlotQuery,&RealClass](const FSigilEquipmentEntry& Entry)
		{
			return Entry.Instance->IsA(RealClass) && SlotQuery.Matches(Entry.EquippedSlot.GetSingleTagContainer());
		});
		for (const FSigilEquipmentEntry& Entry : MatchedEntries)
		{
			Results.AddUnique(Entry.Instance);
		}
	}
	return Results;
}

TArray<UObject*> USigilEquipmentSystemComponent::GetActiveEquipments(TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const
{
	TArray<UObject*> Results;
	if (SlotQuery.IsEmpty())
	{
		return Results;
	}

	if (UClass* RealClass = InstanceType)
	{
		const TArray<FSigilEquipmentEntry>& MatchedEntries = Container.Entries.FilterByPredicate([&SlotQuery,&RealClass](const FSigilEquipmentEntry& Entry)
		{
			return Entry.bActive && Entry.Instance->IsA(RealClass) && SlotQuery.Matches(Entry.EquippedSlot.GetSingleTagContainer());
		});
		for (const FSigilEquipmentEntry& Entry : MatchedEntries)
		{
			Results.AddUnique(Entry.Instance);
		}
	}
	return Results;
}

UObject* USigilEquipmentSystemComponent::GetEquipment(TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const
{
	if (UClass* RealClass = InstanceType)
	{
		const FSigilEquipmentEntry* Found = Container.Entries.FindByPredicate([&SlotQuery,&RealClass](const FSigilEquipmentEntry& Entry)
		{
			return Entry.Instance->IsA(RealClass) && SlotQuery.Matches(Entry.EquippedSlot.GetSingleTagContainer());
		});
		if (Found)
		{
			return Found->Instance;
		}
	}
	return nullptr;
}

UObject* USigilEquipmentSystemComponent::GetActiveEquipment(TSubclassOf<UObject> InstanceType, FGameplayTagQuery SlotQuery) const
{
	if (UClass* RealClass = InstanceType)
	{
		const FSigilEquipmentEntry* Found = Container.Entries.FindByPredicate([&SlotQuery,&RealClass](const FSigilEquipmentEntry& Entry)
		{
			return Entry.bActive && Entry.Instance->IsA(RealClass) && SlotQuery.Matches(Entry.EquippedSlot.GetSingleTagContainer());
		});
		if (Found)
		{
			return Found->Instance;
		}
	}
	return nullptr;
}

USigilEquipmentInstance* USigilEquipmentSystemComponent::GetEquipmentInstanceOfActor(AActor* EquipmentActor) const
{
	if (IsValid(EquipmentActor))
	{
		for (const FSigilEquipmentEntry& Entry : Container.Entries)
		{
			if (USigilEquipmentInstance* Instance = Cast<USigilEquipmentInstance>(Entry.Instance))
			{
				if (Instance->GetEquipmentActors().Contains(EquipmentActor))
				{
					return Instance;
				}
			}
		}
	}
	return nullptr;
}

USigilEquipmentInstance* USigilEquipmentSystemComponent::GetTypedEquipmentInstanceOfActor(TSubclassOf<USigilEquipmentInstance> InstanceType, AActor* EquipmentActor) const
{
	if (UClass* RealClass = InstanceType)
	{
		if (USigilEquipmentInstance* Instance = GetEquipmentInstanceOfActor(EquipmentActor))
		{
			if (Instance->GetClass()->IsChildOf(RealClass))
			{
				return Instance;
			}
		}
	}
	return nullptr;
}

bool USigilEquipmentSystemComponent::IsSlotEquipped(FGameplayTag SlotTag) const
{
	return SlotToIdxMap.Contains(SlotTag);
}

int32 USigilEquipmentSystemComponent::SlotTagToEquipmentIndex(FGameplayTag InSlotTag) const
{
	return Container.IndexOfBySlot(InSlotTag);
}

int32 USigilEquipmentSystemComponent::ItemIdToEquipmentInex(FGuid InItemId) const
{
	return Container.IndexOfByItemId(InItemId);
}

UObject* USigilEquipmentSystemComponent::GetEquipmentInSlot(FGameplayTag SlotTag) const
{
	int32 Idx = Container.IndexOfBySlot(SlotTag);

	if (Idx != INDEX_NONE)
	{
		return Container.Entries[Idx].Instance;
	}
	return nullptr;
}

UObject* USigilEquipmentSystemComponent::GetEquipmentByItem(const USigilItemInstance* Item)
{
	if (Item == nullptr)
	{
		return nullptr;
	}
	int32 Idx = Container.IndexOfByItem(Item);
	if (Idx != INDEX_NONE)
	{
		return Container.Entries[Idx].Instance;
	}
	return nullptr;
}

void USigilEquipmentSystemComponent::SetEquipmentActiveState(FGameplayTag SlotTag, bool NewActiveState)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	int32 Idx = Container.IndexOfBySlot(SlotTag);
	if (Idx != INDEX_NONE)
	{
		SetEquipmentActiveStateWithGroupRestriction(Idx, NewActiveState);
	}
}

void USigilEquipmentSystemComponent::ServerSetEquipmentActiveState(FGameplayTag SlotTag, bool NewActiveState)
{
	SetEquipmentActiveState(SlotTag, NewActiveState);
}

bool USigilEquipmentSystemComponent::CanActiveSlotInGroup(FGameplayTag SlotTag, FGameplayTag& MatchingGroup, int32& MatchingIdxInGroup)
{
	for (auto& Pair : GroupActiveIdxMap)
	{
		if (SlotTag.MatchesTag(Pair.Key))
		{
			MatchingGroup = Pair.Key;
			break;
		}
	}

	if (!MatchingGroup.IsValid())
	{
		return true;
	}
	// with in group but already has active equipment. Already occupied
	if (GroupActiveIdxMap[MatchingGroup] != INDEX_NONE)
	{
		return false;
	}
	MatchingIdxInGroup = TargetCollectionDefinition->GetSlotIndexWithinGroup(MatchingGroup, SlotTag);
	check(MatchingIdxInGroup != INDEX_NONE);
	return true;
}

void USigilEquipmentSystemComponent::SetEquipmentActiveStateWithGroupRestriction(int32 Idx, bool NewActiveState)
{
	if (!Container.Entries.IsValidIndex(Idx))
	{
		return;
	}

	const FSigilEquipmentEntry& Entry = Container.Entries[Idx];
	if (Entry.bActive)
	{
		return;
	}

	FGameplayTag MatchingGroup = FGameplayTag::EmptyTag;
	for (auto& Pair : GroupActiveIdxMap)
	{
		if (Entry.EquippedSlot.MatchesTag(Pair.Key))
		{
			MatchingGroup = Pair.Key;
			break;
		}
	}


	if (MatchingGroup.IsValid())
	{
		// with in group but already has active equipment. Already occupied
		if (NewActiveState && GroupActiveIdxMap[MatchingGroup] != INDEX_NONE)
		{
			return;
		}
		if (NewActiveState)
		{
			int32 NewIdxInGroup = TargetCollectionDefinition->GetSlotIndexWithinGroup(MatchingGroup, Entry.EquippedSlot);
			check(NewIdxInGroup != INDEX_NONE);
			GroupActiveIdxMap[MatchingGroup] = NewIdxInGroup;
		}
		else
		{
			GroupActiveIdxMap[MatchingGroup] = INDEX_NONE;
		}
	}

	SetEquipmentActiveState(Idx, NewActiveState);
}

void USigilEquipmentSystemComponent::SetEquipmentActiveState(int32 Idx, bool NewActiveState)
{
	check(Container.Entries.IsValidIndex(Idx))
	if (Container.Entries[Idx].bActive == NewActiveState)
	{
		// Same state, return.
		return;
	}

	FSigilEquipmentEntry& Entry = Container.Entries[Idx];

	Entry.bActive = NewActiveState;
	OnEquipmentEntryChanged(Entry, Idx);
	Container.MarkItemDirty(Entry);
}


void USigilEquipmentSystemComponent::OnTargetCollectionChanged(const FSigilInventoryStackUpdateMessage& Message)
{
	// only handle equip/unequip on the server side.
	if (!OwnerHasAuthority())
	{
		return;
	}

	// Make use this message comes from the inventory&&collection I care about.
	bool bIsMyConcern = IsValid(Message.Inventory) && Message.Inventory == Inventory && Message.CollectionId == TargetCollection->GetCollectionId();

	if (!bIsMyConcern)
	{
		return;
	}

	switch (Message.ChangeType)
	{
	case ESigilItemStackChangeType::WasAdded:
		{
			FGameplayTag SlotTag = TargetCollection->GetItemSlotName(Message.Instance);
			if (!SlotTag.IsValid())
			{
				return;
			}
			EquipItemToSlot(Message.Instance, SlotTag);
			return;
		}
	case ESigilItemStackChangeType::WasRemoved:
		{
			UnequipByItem(Message.Instance->GetItemId());
		}
	default: break;
	}
}

void USigilEquipmentSystemComponent::ProcessPendingEquipments()
{
	if (HasBegunPlay() && GetOwner() != nullptr)
	{
		TArray<int32> AddedEquipments;
		for (auto& Pair : PendingEquipmentEntries)
		{
			if (Pair.Value.IsValid())
			{
				AddedEquipments.Add(Pair.Key);
			}
		}
		for (int32 i = 0; i < AddedEquipments.Num(); i++)
		{
			int32 idx = AddedEquipments[i];
			const FSigilEquipmentEntry& Entry = PendingEquipmentEntries[idx];
			OnEquipmentEntryAdded(Entry, idx);
			PendingEquipmentEntries.Remove(idx);
		}
	}
}

void USigilEquipmentSystemComponent::OnEquipmentSystemInitialized_Implementation()
{
	OnEquipmentSystemInitializedEvent.Broadcast();

	TArray<FSigilEquipmentSystem_Initialized_DynamicEvent> Delegates = InitializedDelegates;
	for (FSigilEquipmentSystem_Initialized_DynamicEvent Delegate : Delegates)
	{
		Delegate.ExecuteIfBound();
	}
	InitializedDelegates.Empty();
}

void USigilEquipmentSystemComponent::OnTargetCollectionRemoved(USigilItemCollection* Collection)
{
	if (Collection && TargetCollection && TargetCollection == Collection)
	{
		ResetEquipmentSystem();
	}
}

bool USigilEquipmentSystemComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FSigilEquipmentEntry& Entry : Container.Entries)
	{
		if (IsValid(Entry.Instance))
		{
			if (ISigilEquipmentInterface::Execute_IsReplicationManaged(Entry.Instance))
			{
				WroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
			}
		}
	}

	return WroteSomething;
}

void USigilEquipmentSystemComponent::OnRegister()
{
	Super::OnRegister();
}

void USigilEquipmentSystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return;
	}

	Container.OwningComponent = this;
	if (!GetOwner()->IsUsingRegisteredSubObjectList())
	{
		SIGIL_INVENTORY_CLOG(Error, "requires enable bReplicateUsingRegisteredSubObjectList.")
	}
}

void USigilEquipmentSystemComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	// Register existing Equipment Instance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const TObjectPtr<UObject>& PendingObject : PendingReplicatedEquipments)
		{
			if (!IsReplicatedSubObjectRegistered(PendingObject))
			{
				AddReplicatedSubObject(PendingObject);
			}
		}
		PendingReplicatedEquipments.Empty();
	}
}

// Called when the game starts
void USigilEquipmentSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bInitializeOnBeginPlay && OwnerHasAuthority())
	{
		InitializeEquipmentSystem();
	}
}


void USigilEquipmentSystemComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ProcessPendingEquipments();
}

void USigilEquipmentSystemComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void USigilEquipmentSystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bInitializeOnBeginPlay)
	{
		ResetEquipmentSystem();
	}

	Super::EndPlay(EndPlayReason);
}


void USigilEquipmentSystemComponent::InitializeEquipmentSystem()
{
	if (bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "already initialized or has no authority!")
		return;
	}

	USigilInventorySystemComponent* InventorySystem = USigilInventorySystemComponent::FindInventorySystemComponent(GetOwner());

	if (InventorySystem == nullptr)
	{
		InventorySystem = USigilInventorySystemComponent::FindInventorySystemComponent(GetController<AController>());
	}

	if (!InventorySystem)
	{
		SIGIL_INVENTORY_CLOG(Error, "doesn't have valid inventory system component!")
		return;
	}

	InitializeEquipmentSystemWithInventory(InventorySystem);
}

void USigilEquipmentSystemComponent::InitializeEquipmentSystemWithInventory(USigilInventorySystemComponent* InventorySystem)
{
	if (bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "already initialized or has no authority!")
		return;
	}

	if (!IsValid(InventorySystem))
	{
		SIGIL_INVENTORY_CLOG(Error, "the inventory is invalid!")
		return;
	}

	if (!InventorySystem->IsInventoryInitialized())
	{
		SIGIL_INVENTORY_CLOG(Error, "the inventory is not initialized!")
		return;
	}

	if (!TargetCollectionTag.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Error, "doesn't have valid target collection tag!")
		return;
	}

	USigilItemSlotCollection* Collection = Cast<USigilItemSlotCollection>(InventorySystem->GetCollectionByTag(GetTargetCollectionTag()));

	if (Collection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "%s's inventory doesn't have valid item slot collection with name:%s", *InventorySystem->GetOwner()->GetName(), *TargetCollectionTag.ToString());
		return;
	}
	Inventory = InventorySystem;
	TargetCollection = Collection;
	TargetCollectionDefinition = Collection->GetMyDefinition();

	GroupActiveIdxMap.Empty();
	for (const TPair<FGameplayTag, FSigilItemSlotGroup>& Pair : TargetCollectionDefinition->SlotGroupMap)
	{
		GroupActiveIdxMap.Add(Pair.Key, INDEX_NONE);
	}

	Inventory->OnInventoryStackUpdate.AddDynamic(this, &ThisClass::OnTargetCollectionChanged);
	Inventory->OnCollectionRemovedEvent.AddDynamic(this, &ThisClass::OnTargetCollectionRemoved);

	bEquipmentSystemInitialized = true;
	OnEquipmentSystemInitialized();

	for (const FSigilItemInfo& ItemInfo : TargetCollection->GetAllItemInfos())
	{
		FGameplayTag SlotTag = TargetCollection->GetItemSlotName(ItemInfo.Item);
		EquipItemToSlot(ItemInfo.Item, SlotTag);
	}
}

void USigilEquipmentSystemComponent::ResetEquipmentSystem()
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	RemoveAllEquipments();
	if (IsValid(Inventory))
	{
		Inventory->OnCollectionRemovedEvent.RemoveDynamic(this, &ThisClass::OnTargetCollectionRemoved);
		Inventory->OnInventoryStackUpdate.RemoveDynamic(this, &ThisClass::OnTargetCollectionChanged);
		Inventory = nullptr;
		TargetCollection = nullptr;
		TargetCollectionDefinition = nullptr;
		GroupActiveIdxMap.Empty();
	}
	bEquipmentSystemInitialized = false;
	OnEquipmentSystemInitialized();
}

bool USigilEquipmentSystemComponent::IsEquipmentSystemInitialized() const
{
	return bEquipmentSystemInitialized;
}

void USigilEquipmentSystemComponent::BindToEquipmentSystemInitialized(FSigilEquipmentSystem_Initialized_DynamicEvent Delegate)
{
	if (bEquipmentSystemInitialized)
	{
		Delegate.ExecuteIfBound();
	}
	else
	{
		InitializedDelegates.Add(Delegate);
	}
}


USigilEquipmentSystemComponent* USigilEquipmentSystemComponent::GetEquipmentSystemComponent(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<USigilEquipmentSystemComponent>() : nullptr;
}

bool USigilEquipmentSystemComponent::FindEquipmentSystemComponent(const AActor* Actor, USigilEquipmentSystemComponent*& Component)
{
	Component = (Actor ? Actor->FindComponentByClass<USigilEquipmentSystemComponent>() : nullptr);
	return Component != nullptr;
}

bool USigilEquipmentSystemComponent::FindTypedEquipmentSystemComponent(AActor* Actor, TSubclassOf<USigilEquipmentSystemComponent> DesiredClass, USigilEquipmentSystemComponent*& Component)
{
	if (UClass* RealClass = DesiredClass)
	{
		if (FindEquipmentSystemComponent(Actor, Component))
		{
			if (Component->GetClass()->IsChildOf(RealClass))
			{
				return true;
			}
		}
	}
	return false;
}


void USigilEquipmentSystemComponent::RemoveAllEquipments()
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	for (int32 i = 0; i < Container.Entries.Num(); i++)
	{
		RemoveEquipmentEntry(i);
	}
}

void USigilEquipmentSystemComponent::AddEquipmentEntry(const FSigilEquipmentEntry& NewEntry)
{
	check(NewEntry.IsValid())
	int32 Idx = Container.Entries.AddDefaulted();
	Container.Entries[Idx] = NewEntry;

	AddReplicatedEquipmentObject(NewEntry.Instance);
	OnEquipmentEntryAdded(NewEntry, Idx);
	Container.MarkItemDirty(Container.Entries[Idx]);
}

void USigilEquipmentSystemComponent::RemoveEquipmentEntry(int32 Idx)
{
	check(Container.Entries.IsValidIndex(Idx));
	const FSigilEquipmentEntry& Entry = Container.Entries[Idx];
	RemoveReplicatedEquipmentObject(Entry.Instance);
	OnEquipmentEntryRemoved(Entry, Idx);
	Container.Entries.RemoveAt(Idx);
	Container.MarkArrayDirty();
}

UObject* USigilEquipmentSystemComponent::CreateEquipmentInstance_Implementation(AActor* Owner, USigilItemInstance* ItemInstance) const
{
	if (ItemInstance == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "passed in invalid item instance.")
		return nullptr;
	}
	const USigilItemFragment_Equippable* EquippableItem = ItemInstance->FindFragmentByClass<USigilItemFragment_Equippable>();

	if (EquippableItem == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "missing equippable fragment on item(%s)", *ItemInstance->GetDefinition()->GetName())
		return nullptr;
	}

	TSubclassOf<UObject> InstanceType = !EquippableItem->InstanceType.IsNull() ? EquippableItem->InstanceType.LoadSynchronous() : nullptr;
	if (InstanceType == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "missing valid equipment instance type on item(%s)", *ItemInstance->GetDefinition()->GetName()
		)
		return nullptr;
	}
	if (!InstanceType->ImplementsInterface(USigilEquipmentInterface::StaticClass()))
	{
		SIGIL_INVENTORY_CLOG(Error, "equipment instance type doesn't implement:%s", *USigilEquipmentInterface::StaticClass()->GetName())
		return nullptr;
	}

	UObject* Instance;

	if (EquippableItem->bActorBased)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Owner;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(InstanceType, FTransform::Identity, SpawnParameters);
		if (SpawnedActor == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Error, "failed to create equipment instance of type:%s", *InstanceType->GetName())
			return nullptr;
		}
		Instance = SpawnedActor;
	}
	else
	{
		USigilEquipmentInstance* EquipmentInstance = NewObject<USigilEquipmentInstance>(Owner, InstanceType); //Using the actor instead of component as the outer due to UE-127172
		if (EquipmentInstance == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Error, "failed to create equipment instance of type:%s", *InstanceType->GetName())
			return nullptr;
		}
		Instance = EquipmentInstance;
	}

	return Instance;
}


void USigilEquipmentSystemComponent::OnEquipmentEntryAdded(const FSigilEquipmentEntry& Entry, int32 Idx)
{
	APawn* OwningPawn = GetPawn<APawn>();

	SlotToIdxMap.Add(Entry.EquippedSlot, Entry.Instance);

	ISigilEquipmentInterface::Execute_ReceiveOwningPawn(Entry.Instance, OwningPawn);
	ISigilEquipmentInterface::Execute_ReceiveSourceItem(Entry.Instance, Entry.ItemInstance);
	ISigilEquipmentInterface::Execute_OnEquipmentBeginPlay(Entry.Instance);
	OnEquipmentStateChangedEvent.Broadcast(Entry.Instance, Entry.EquippedSlot, true);

	if (Entry.bActive)
	{
		ISigilEquipmentInterface::Execute_OnActiveStateChanged(Entry.Instance, Entry.bActive);
		OnEquipmentActiveStateChangedEvent.Broadcast(Entry.Instance, Entry.EquippedSlot, Entry.bActive);
	}
}

void USigilEquipmentSystemComponent::OnEquipmentEntryChanged(const FSigilEquipmentEntry& Entry, int32 Idx)
{
	ISigilEquipmentInterface::Execute_OnActiveStateChanged(Entry.Instance, Entry.bActive);
	OnEquipmentActiveStateChangedEvent.Broadcast(Entry.Instance, Entry.EquippedSlot, Entry.bActive);
}

void USigilEquipmentSystemComponent::OnEquipmentEntryRemoved(const FSigilEquipmentEntry& Entry, int32 Idx)
{
	// remove but still active, so notify instance to do deactivate behavior.

	SlotToIdxMap.Remove(Entry.EquippedSlot);

	if (IsValid(Entry.Instance)) // The instance may alreay in pending kill state, so no point to continues execution.
	{
		if (Entry.bActive)
		{
			ISigilEquipmentInterface::Execute_OnActiveStateChanged(Entry.Instance, false);
		}
		ISigilEquipmentInterface::Execute_OnEquipmentEndPlay(Entry.Instance);
		ISigilEquipmentInterface::Execute_ReceiveOwningPawn(Entry.Instance, nullptr);
		ISigilEquipmentInterface::Execute_ReceiveSourceItem(Entry.Instance, nullptr);
	}

	OnEquipmentStateChangedEvent.Broadcast(Entry.Instance, Entry.EquippedSlot, false);
}

void USigilEquipmentSystemComponent::AddReplicatedEquipmentObject(TObjectPtr<UObject> Instance)
{
	if (OwnerHasAuthority() && IsValid(Instance))
	{
		checkf(Instance->GetClass()->ImplementsInterface(USigilEquipmentInterface::StaticClass()), TEXT("%s doesn't implement SigilEquipmentInterface"), *Instance->GetClass()->GetName())
		bool IsReplicationManaged = ISigilEquipmentInterface::Execute_IsReplicationManaged(Instance);
		if (!IsReplicationManaged)
		{
			return;
		}
		if (IsReadyForReplication() && !IsReplicatedSubObjectRegistered(Instance))
		{
			AddReplicatedSubObject(Instance);
		}
		else
		{
			PendingReplicatedEquipments.AddUnique(Instance);
		}
	}
}

void USigilEquipmentSystemComponent::RemoveReplicatedEquipmentObject(TObjectPtr<UObject> Instance)
{
	if (OwnerHasAuthority() && IsValid(Instance))
	{
		bool IsReplicationManaged = ISigilEquipmentInterface::Execute_IsReplicationManaged(Instance);

		if (IsReplicationManaged && IsReplicatedSubObjectRegistered(Instance))
		{
			RemoveReplicatedSubObject(Instance);
		}
	}
}

#pragma region Equipment Groups

TMap<int32, FGameplayTag> USigilEquipmentSystemComponent::GetLayoutOfGroup(FGameplayTag GroupTag) const
{
	TMap<int32, FGameplayTag> GroupLayout;
	if (TargetCollectionDefinition == nullptr)
	{
		return GroupLayout;
	}
	if (TargetCollectionDefinition->SlotGroupMap.Contains(GroupTag))
	{
		GroupLayout = TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap;
	}
	return GroupLayout;
}

TMap<FGameplayTag, int32> USigilEquipmentSystemComponent::GetSlottedLayoutOfGroup(FGameplayTag GroupTag) const
{
	TMap<FGameplayTag, int32> GroupLayout;
	if (TargetCollectionDefinition == nullptr)
	{
		return GroupLayout;
	}
	if (TargetCollectionDefinition->SlotGroupMap.Contains(GroupTag))
	{
		GroupLayout = TargetCollectionDefinition->SlotGroupMap[GroupTag].SlotToIndexMap;
	}
	return GroupLayout;
}

TMap<FGameplayTag, UObject*> USigilEquipmentSystemComponent::GetSlottedEquipmentsOfGroup(FGameplayTag GroupTag) const
{
	TMap<FGameplayTag, int32> GroupLayout = GetSlottedLayoutOfGroup(GroupTag);

	TMap<FGameplayTag, UObject*> GroupedEquipments;
	GroupedEquipments.Reserve(GroupLayout.Num());

	for (auto& Pair : GroupLayout)
	{
		int32 Idx = Container.IndexOfBySlot(Pair.Key);
		if (Idx != INDEX_NONE)
		{
			GroupedEquipments.Emplace(Pair.Key, Container.Entries[Idx].Instance);
		}
		else
		{
			GroupedEquipments.Emplace(Pair.Key, nullptr);
		}
	}
	return GroupedEquipments;
}

TMap<int32, UObject*> USigilEquipmentSystemComponent::GetEquipmentsOfGroup(FGameplayTag GroupTag) const
{
	TMap<int32, FGameplayTag> GroupLayout = GetLayoutOfGroup(GroupTag);

	TMap<int32, UObject*> GroupedEquipments;
	GroupedEquipments.Reserve(GroupLayout.Num());

	for (auto& Pair : GroupLayout)
	{
		int32 Idx = Container.IndexOfBySlot(Pair.Value);
		if (Idx != INDEX_NONE)
		{
			GroupedEquipments.Emplace(Pair.Key, Container.Entries[Idx].Instance);
		}
		else
		{
			GroupedEquipments.Emplace(Pair.Key, nullptr);
		}
	}
	return GroupedEquipments;
}

void USigilEquipmentSystemComponent::SetGroupActiveIndex(FGameplayTag GroupTag, int32 NewIndex)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}
	if (!GroupActiveIdxMap.Contains(GroupTag))
	{
		return;
	}
	int32 PrevActiveIndex = GroupActiveIdxMap[GroupTag];
	if (PrevActiveIndex != NewIndex)
	{
		GroupActiveIdxMap[GroupTag] = NewIndex;
		OnGroupActiveIndexChanged(GroupTag, PrevActiveIndex, NewIndex);
	}
}

void USigilEquipmentSystemComponent::ServerSetGroupActiveIndex_Implementation(FGameplayTag GroupTag, int32 NewIndex)
{
	SetGroupActiveIndex(GroupTag, NewIndex);
}


void USigilEquipmentSystemComponent::CycleGroupActiveIndex(FGameplayTag GroupTag, bool bDirection)
{
	if (!bEquipmentSystemInitialized || !OwnerHasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Error, "not initialized or has no authority!")
		return;
	}

	if (!GroupActiveIdxMap.Contains(GroupTag))
	{
		SIGIL_INVENTORY_CLOG(Warning, "has no equipment group named:%s.", *GroupTag.ToString())
		return;
	}

	int32 MaxIndex = TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap.Num() - 1;
	int32 PrevIndex = GroupActiveIdxMap[GroupTag];
	if (PrevIndex == INDEX_NONE)
	{
		int32 NewIndex = bDirection ? 0 : MaxIndex;
		GroupActiveIdxMap[GroupTag] = NewIndex;
		OnGroupActiveIndexChanged(GroupTag, PrevIndex, NewIndex);
	}
	else
	{
		int32 NewIndex = bDirection ? PrevIndex + 1 : PrevIndex - 1;
		if (bDirection && NewIndex > MaxIndex)
		{
			NewIndex = 0;
		}
		if (!bDirection && NewIndex < 0)
		{
			NewIndex = MaxIndex;
		}
		if (NewIndex != PrevIndex)
		{
			GroupActiveIdxMap[GroupTag] = NewIndex;
			OnGroupActiveIndexChanged(GroupTag, PrevIndex, NewIndex);
		}
	}
}

void USigilEquipmentSystemComponent::ServerCycleGroupActiveIndex_Implementation(FGameplayTag GroupTag, bool bDirection)
{
	CycleGroupActiveIndex(GroupTag, bDirection);
}

void USigilEquipmentSystemComponent::OnGroupActiveIndexChanged(const FGameplayTag& GroupTag, int32 PrevIndex, int32 NewIndex)
{
	if (!OwnerHasAuthority())
	{
		return;
	}

	// Deactivate the equipment for prev active index.
	if (PrevIndex != INDEX_NONE && TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap.Contains(PrevIndex))
	{
		FGameplayTag PrevSlot = TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap[PrevIndex];
		int32 EquipmentEntryIdx = Container.IndexOfBySlot(PrevSlot);
		if (EquipmentEntryIdx != INDEX_NONE)
		{
			if (Container.Entries[EquipmentEntryIdx].bActive)
			{
				SetEquipmentActiveState(EquipmentEntryIdx, false);
			}
		}
	}

	// Activate the equipment for new active index.
	if (NewIndex != INDEX_NONE && TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap.Contains(NewIndex))
	{
		FGameplayTag NewSlot = TargetCollectionDefinition->SlotGroupMap[GroupTag].IndexToSlotMap[NewIndex];

		int32 EquipmentEntryIdx = Container.IndexOfBySlot(NewSlot);
		if (EquipmentEntryIdx != INDEX_NONE)
		{
			if (!Container.Entries[EquipmentEntryIdx].bActive)
			{
				SetEquipmentActiveState(EquipmentEntryIdx, true);
			}
		}
	}

	OnEquipmentGroupActiveIndexChangedEvent.Broadcast(GroupTag, PrevIndex, NewIndex);
	ClientNotifyGroupActiveIndexChanged(GroupTag, PrevIndex, NewIndex);
}

void USigilEquipmentSystemComponent::ClientNotifyGroupActiveIndexChanged_Implementation(const FGameplayTag& GroupTag, int32 PrevIndex, int32 NewIndex)
{
	OnEquipmentGroupActiveIndexChangedEvent.Broadcast(GroupTag, PrevIndex, NewIndex);
}

#pragma endregion
