// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Items/SigilItemInstance.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"
#include "SigilItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "Engine/BlueprintGeneratedClass.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemInstance)


USigilItemInstance::USigilItemInstance(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), IntegerAttributes(this), FloatAttributes(this), FragmentStates(this)
{
	OwningCollection = nullptr;
}

void USigilItemInstance::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = GetItemTags();
}

void USigilItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);


	// FastArray don't need push model.
	DOREPLIFETIME(ThisClass, IntegerAttributes);
	DOREPLIFETIME(ThisClass, FloatAttributes);
	DOREPLIFETIME(ThisClass, FragmentStates);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemId, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Definition, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OwningCollection, SharedParams);

	SharedParams.Condition = COND_InitialOrOwner;

	//fix: https://forums.unrealengine.com/t/subobject-replication-for-blueprint-child-class/106205/4
	UBlueprintGeneratedClass* bpClass = Cast<UBlueprintGeneratedClass>(this->GetClass());
	if (bpClass != nullptr)
	{
		bpClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

FGuid USigilItemInstance::GetItemId() const
{
	return ItemId;
}

void USigilItemInstance::SetItemId(FGuid NewId)
{
	ItemId = NewId;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemId, this);
}

bool USigilItemInstance::IsUnique() const
{
	return Definition->bUnique;
}

FText USigilItemInstance::GetItemName() const
{
	return Definition->DisplayName;
}

const USigilItemDefinition* USigilItemInstance::GetDefinition() const
{
	return Definition;
}

void USigilItemInstance::SetDefinition(const USigilItemDefinition* NewDefinition)
{
	Definition = NewDefinition;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Definition, this);
}

FGameplayTagContainer USigilItemInstance::GetItemTags() const
{
	return Definition->ItemTags;
}

const USigilItemFragment* USigilItemInstance::GetFragment(TSubclassOf<USigilItemFragment> FragmentClass) const
{
	if (Definition != nullptr && FragmentClass != nullptr)
	{
		return Definition->GetFragment(FragmentClass);
	}
	return nullptr;
}

const USigilItemFragment* USigilItemInstance::FindFragment(TSubclassOf<USigilItemFragment> FragmentClass, bool& bValid) const
{
	bValid = false;
	if (const USigilItemFragment* Fragment = GetFragment(FragmentClass))
	{
		bValid = true;
		return Fragment;
	}
	return nullptr;
}

bool USigilItemInstance::HasAnyAttribute(FGameplayTag AttributeTag) const
{
	return HasFloatAttribute(AttributeTag) || HasIntegerAttribute(AttributeTag);
}

bool USigilItemInstance::HasFloatAttribute(FGameplayTag AttributeTag) const
{
	if (FloatAttributes.ContainsTag(AttributeTag))
	{
		return true;
	}
	return false;
}

FText USigilItemInstance::GetItemDescription() const
{
	return Definition->Description;
}

float USigilItemInstance::GetFloatAttribute(FGameplayTag AttributeTag) const
{
	if (FloatAttributes.ContainsTag(AttributeTag))
	{
		return FloatAttributes.GetValue(AttributeTag);
	}
	return 0;
}

int32 USigilItemInstance::GetIntegerAttribute(FGameplayTag AttributeTag) const
{
	if (IntegerAttributes.ContainsTag(AttributeTag))
	{
		return IntegerAttributes.GetValue(AttributeTag);
	}
	return 0;
}

void USigilItemInstance::SetFloatAttribute(FGameplayTag AttributeTag, float NewValue)
{
	FloatAttributes.SetItem(AttributeTag, NewValue);
}

void USigilItemInstance::AddFloatAttribute(FGameplayTag AttributeTag, float Value)
{
	FloatAttributes.AddItem(AttributeTag, Value);
}

void USigilItemInstance::RemoveFloatAttribute(FGameplayTag AttributeTag, float Value)
{
	FloatAttributes.RemoveItem(AttributeTag, Value);
}

bool USigilItemInstance::HasIntegerAttribute(FGameplayTag AttributeTag) const
{
	if (IntegerAttributes.ContainsTag(AttributeTag))
	{
		return true;
	}
	return false;
}

void USigilItemInstance::SetIntegerAttribute(FGameplayTag AttributeTag, int32 NewValue)
{
	IntegerAttributes.SetItem(AttributeTag, NewValue);
}

void USigilItemInstance::AddIntegerAttribute(FGameplayTag AttributeTag, int32 NewValue)
{
	IntegerAttributes.AddItem(AttributeTag, NewValue);
}

void USigilItemInstance::RemoveIntegerAttribute(FGameplayTag AttributeTag, int32 Value)
{
	IntegerAttributes.RemoveItem(AttributeTag, Value);
}

USigilItemCollection* USigilItemInstance::GetOwningCollection() const
{
	return OwningCollection;
}

USigilInventorySystemComponent* USigilItemInstance::GetOwningInventory() const
{
	return IsValid(OwningCollection) ? OwningCollection->GetOwningInventory() : nullptr;
}

bool USigilItemInstance::FindFragmentStateByClass(TSubclassOf<USigilItemFragment> FragmentClass, FInstancedStruct& OutState) const
{
	return FragmentStates.GetDataByTarget(GetFragment(FragmentClass), OutState);
}

void USigilItemInstance::SetFragmentStateByClass(TSubclassOf<USigilItemFragment> FragmentClass, const FInstancedStruct& NewState)
{
	if (const USigilItemFragment* Fragment = Definition->GetFragment(FragmentClass))
	{
		FragmentStates.SetDataForTarget(Fragment, NewState);
	}
}

void USigilItemInstance::AssignCollection(USigilItemCollection* NewItemCollection)
{
	if (NewItemCollection == nullptr || OwningCollection == NewItemCollection)
	{
		return;
	}

	if (OwningCollection != nullptr && OwningCollection->GetOwningInventory() != nullptr && OwningCollection != NewItemCollection)
	{
		SIGIL_INVENTORY_CLOG(Error, "is unable to be added to a new item collection:%s when it is already a member of an existing item collection.", *OwningCollection->GetDebugString());
		return;
	}
	OwningCollection = NewItemCollection;
	SIGIL_INVENTORY_CLOG(Verbose, "item(%s) assigned new collection(%s)", *GetNameSafe(this), *OwningCollection->GetDebugString());
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OwningCollection, this);
}

void USigilItemInstance::UnassignCollection(USigilItemCollection* ItemCollection)
{
	if (OwningCollection != nullptr && ItemCollection != nullptr && OwningCollection != ItemCollection)
	{
		SIGIL_INVENTORY_CLOG(Warning, "belong to %s, but trying to remove from %s.", *OwningCollection->GetDebugString(), *ItemCollection->GetDebugString());
		return;
	}
	SIGIL_INVENTORY_CLOG(Verbose, "item(%s) removed from collection(%s)", *GetNameSafe(this), *OwningCollection->GetDebugString());
	OwningCollection = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OwningCollection, this);
}

// void USigilItemInstance::ResetCollection()
// {
// 	OwningCollection = nullptr;
// 	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OwningCollection, this);
// }

bool USigilItemInstance::IsItemValid() const
{
	return ItemId.IsValid() && Definition != nullptr;
}

bool USigilItemInstance::StackableEquivalentTo(const USigilItemInstance* OtherItem) const
{
	return AreStackableEquivalent(this, OtherItem);
}

bool USigilItemInstance::SimilarTo(const USigilItemInstance* OtherItem) const
{
	return AreSimilar(this, OtherItem);
}

bool USigilItemInstance::AreStackableEquivalent(const USigilItemInstance* Lhs, const USigilItemInstance* Rhs)
{
	if (Lhs == nullptr || Rhs == nullptr)
	{
		return false;
	}
	// 有必要比较同一个指针吗?
	if (Lhs == Rhs)
	{
		return true;
	}
	if (Lhs->GetClass() != Rhs->GetClass())
	{
		return false;
	}
	if (Lhs->GetDefinition() != nullptr && Rhs->GetDefinition() != nullptr)
	{
		if (Lhs->GetDefinition() != Rhs->GetDefinition())
		{
			return false;
		}
	}
	if (Lhs->IsUnique())
	{
		return false;
	}
	return true;
}

bool USigilItemInstance::AreSimilar(const USigilItemInstance* Lhs, const USigilItemInstance* Rhs)
{
	if (Lhs == nullptr || Rhs == nullptr)
	{
		return false;
	}
	if (Lhs == Rhs)
	{
		return true;
	}
	if (Lhs->GetClass() != Rhs->GetClass())
	{
		return false;
	}
	if (Lhs->GetDefinition() != Rhs->GetDefinition())
	{
		return false;
	}
	if (Lhs->IsUnique())
	{
		return false;
	}
	return true;
}

void USigilItemInstance::OnItemDuplicated(const USigilItemInstance* SrcItem)
{
}

const FSigilMixinContainer& USigilItemInstance::GetFragmentStates() const
{
	return FragmentStates;
}

void USigilItemInstance::OnMixinDataAdded(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data)
{
	const USigilItemFragment* Fragment = CastChecked<USigilItemFragment>(Target);
	OnFragmentStateAdded(Fragment, Data);
	OnFragmentStateAddedEvent.Broadcast(Fragment, Data);
}

void USigilItemInstance::OnMixinDataUpdated(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data)
{
	const USigilItemFragment* Fragment = CastChecked<USigilItemFragment>(Target);
	OnFragmentStateUpdated(Fragment, Data);
	OnFragmentStateUpdatedEvent.Broadcast(Fragment, Data);
}

void USigilItemInstance::OnMixinDataRemoved(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data)
{
	const USigilItemFragment* Fragment = CastChecked<USigilItemFragment>(Target);
	OnFragmentStateRemoved(Fragment, Data);
	OnFragmentStateRemovedEvent.Broadcast(Fragment, Data);
}

void USigilItemInstance::OnFragmentStateAdded(const USigilItemFragment* Fragment, const FInstancedStruct& Data)
{
}

void USigilItemInstance::OnFragmentStateUpdated(const USigilItemFragment* Fragment, const FInstancedStruct& Data)
{
}

void USigilItemInstance::OnFragmentStateRemoved(const USigilItemFragment* Fragment, const FInstancedStruct& Data)
{
}

void USigilItemInstance::OnTagFloatUpdate(const FGameplayTag& Tag, float OldValue, float NewValue)
{
	OnFloatAttributeChanged(Tag, OldValue, NewValue);
	OnFloatAttributeChangedEvent.Broadcast(Tag, OldValue, NewValue);
}

void USigilItemInstance::OnTagIntegerUpdate(const FGameplayTag& Tag, int32 OldValue, int32 NewValue)
{
	OnIntegerAttributeChanged(Tag, OldValue, NewValue);
	OnIntegerAttributeChangedEvent.Broadcast(Tag, OldValue, NewValue);
}

void USigilItemInstance::OnFloatAttributeChanged_Implementation(const FGameplayTag& Tag, float OldValue, float NewValue)
{
}

void USigilItemInstance::OnIntegerAttributeChanged_Implementation(const FGameplayTag& Tag, int32 OldValue, int32 NewValue)
{
}

#if UE_WITH_IRIS
void USigilItemInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;
	UObject::RegisterReplicationFragments(Context, RegistrationFlags);
	// Build descriptors and allocate PropertyReplicationFragments for this object
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif
