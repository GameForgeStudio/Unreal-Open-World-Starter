#include "OWSInventoryBridgeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/Collections/SigilItemCollection.h"
#include "Core/Fragments/SigilItemFragment_Equippable.h"
#include "Core/Items/SigilItemInfo.h"
#include "Core/Items/SigilItemInstance.h"
#include "GameplayEffect.h"
#include "OWSCore.h"
#include "OWSItemFragments.h"
#include "OWSPrototypeAttributeSet.h"
#include "SigilInventorySystemComponent.h"
#include "SigilInventoryTags.h"

UOWSInventoryBridgeComponent::UOWSInventoryBridgeComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UOWSInventoryBridgeComponent::RequestEquipItem(USigilItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		EquipItemAuthority(Item);
	}
	else
	{
		ServerEquipItem(Item);
	}
}

void UOWSInventoryBridgeComponent::RequestUnequipItem(USigilItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UnequipItemAuthority(Item);
	}
	else
	{
		ServerUnequipItem(Item);
	}
}

void UOWSInventoryBridgeComponent::RequestUseItem(USigilItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UseItemAuthority(Item);
	}
	else
	{
		ServerUseItem(Item);
	}
}

void UOWSInventoryBridgeComponent::ServerEquipItem_Implementation(USigilItemInstance* Item)
{
	EquipItemAuthority(Item);
}

void UOWSInventoryBridgeComponent::ServerUnequipItem_Implementation(USigilItemInstance* Item)
{
	UnequipItemAuthority(Item);
}

void UOWSInventoryBridgeComponent::ServerUseItem_Implementation(USigilItemInstance* Item)
{
	UseItemAuthority(Item);
}

bool UOWSInventoryBridgeComponent::IsOwnedInventoryItem(const USigilItemInstance* Item) const
{
	const USigilInventorySystemComponent* Inventory = GetInventory();
	return IsValid(Item) && Inventory && Item->GetOwningInventory() == Inventory;
}

bool UOWSInventoryBridgeComponent::EquipItemAuthority(USigilItemInstance* Item) const
{
	USigilInventorySystemComponent* Inventory = GetInventory();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory || !IsOwnedInventoryItem(Item) ||
		!Item->FindFragmentByClass<USigilItemFragment_Equippable>())
	{
		return false;
	}

	FSigilItemInfo ItemInfo;
	if (!Inventory->GetItemInfo(Item, ItemInfo) || !ItemInfo.IsValid() ||
		!IsValid(Item->GetOwningCollection()) ||
		Item->GetOwningCollection()->GetCollectionTag() == SigilCollectionTags::Equipped)
	{
		return false;
	}

	ItemInfo.Amount = 1;
	ItemInfo.CollectionId.Invalidate();
	ItemInfo.StackId.Invalidate();
	ItemInfo.CollectionTag = SigilCollectionTags::Equipped;
	ItemInfo.Index = INDEX_NONE;
	if (!Inventory->CanMoveItem(ItemInfo))
	{
		return false;
	}

	Inventory->MoveItem(ItemInfo);
	return IsValid(Item->GetOwningCollection()) &&
		Item->GetOwningCollection()->GetCollectionTag() == SigilCollectionTags::Equipped;
}

bool UOWSInventoryBridgeComponent::UnequipItemAuthority(USigilItemInstance* Item) const
{
	USigilInventorySystemComponent* Inventory = GetInventory();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory || !IsOwnedInventoryItem(Item) ||
		!IsValid(Item->GetOwningCollection()) ||
		Item->GetOwningCollection()->GetCollectionTag() != SigilCollectionTags::Equipped)
	{
		return false;
	}

	FSigilItemInfo ItemInfo;
	if (!Inventory->GetItemInfo(Item, ItemInfo) || !ItemInfo.IsValid())
	{
		return false;
	}

	ItemInfo.Amount = 1;
	ItemInfo.CollectionId.Invalidate();
	ItemInfo.StackId.Invalidate();
	ItemInfo.CollectionTag = SigilCollectionTags::Main;
	ItemInfo.Index = INDEX_NONE;
	if (!Inventory->CanMoveItem(ItemInfo))
	{
		return false;
	}

	Inventory->MoveItem(ItemInfo);
	return IsValid(Item->GetOwningCollection()) &&
		Item->GetOwningCollection()->GetCollectionTag() == SigilCollectionTags::Main;
}

bool UOWSInventoryBridgeComponent::UseItemAuthority(USigilItemInstance* Item) const
{
	USigilInventorySystemComponent* Inventory = GetInventory();
	const UOWSItemFragment_UseEffect* Fragment = IsValid(Item)
		? Item->FindFragmentByClass<UOWSItemFragment_UseEffect>()
		: nullptr;
	UAbilitySystemComponent* AbilitySystem = GetOwner()
		? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner())
		: nullptr;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory || !IsOwnedInventoryItem(Item) ||
		!Fragment || !Fragment->UseEffect || !AbilitySystem || Fragment->ConsumeCount < 1)
	{
		return false;
	}

	if (Fragment->bRequiresMissingHealth &&
		AbilitySystem->GetNumericAttribute(UOWSPrototypeAttributeSet::GetHealthAttribute()) >=
		AbilitySystem->GetNumericAttribute(UOWSPrototypeAttributeSet::GetMaxHealthAttribute()))
	{
		return false;
	}

	FSigilItemInfo ItemInfo;
	if (!Inventory->GetItemInfo(Item, ItemInfo) || ItemInfo.Amount < Fragment->ConsumeCount)
	{
		return false;
	}
	ItemInfo.Amount = Fragment->ConsumeCount;
	if (!Inventory->CanRemoveItem(ItemInfo))
	{
		return false;
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(Item);
	const FGameplayEffectSpecHandle Spec =
		AbilitySystem->MakeOutgoingSpec(Fragment->UseEffect, Fragment->EffectLevel, Context);
	if (!Spec.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffectHandle Applied = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (!Applied.WasSuccessfullyApplied())
	{
		return false;
	}

	const FSigilItemInfo Removed = Inventory->RemoveItem(ItemInfo);
	if (Removed.Amount != Fragment->ConsumeCount)
	{
		UE_LOG(LogOWSCore, Error,
			TEXT("Use effect applied but Sigil removed %d of %d requested items."),
			Removed.Amount, Fragment->ConsumeCount);
		return false;
	}
	return true;
}

USigilInventorySystemComponent* UOWSInventoryBridgeComponent::GetInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<USigilInventorySystemComponent>() : nullptr;
}
