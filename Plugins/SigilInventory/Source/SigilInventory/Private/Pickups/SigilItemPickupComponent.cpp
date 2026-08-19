// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Pickups/SigilItemPickupComponent.h"
#include "GameFramework/Actor.h"
#include "SigilInventoryTags.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"
#include "Pickups/SigilWorldItemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemPickupComponent)

bool USigilItemPickupComponent::Pickup(USigilInventorySystemComponent* Picker)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "has no authority!");
		return false;
	}
	if (!CollectionTag.IsValid() || !IsValid(Picker))
	{
		SIGIL_INVENTORY_CLOG(Warning, "passed-in invalid picker.");
		return false;
	}
	if (WorldItemComponent == nullptr && WorldItemComponent->GetItemInstance()->IsItemValid())
	{
		SIGIL_INVENTORY_CLOG(Warning, "doesn't have valid WordItem component attached or it has invalid item instance reference.");
		return false;
	}

	return TryAddToCollection(Picker);
}

USigilWorldItemComponent* USigilItemPickupComponent::GetWorldItem() const
{
	return WorldItemComponent;
}

// Called when the game starts
void USigilItemPickupComponent::BeginPlay()
{
	if (!CollectionTag.IsValid())
	{
		CollectionTag = SigilCollectionTags::Main;
	}

	Super::BeginPlay();
	WorldItemComponent = GetOwner()->FindComponentByClass<USigilWorldItemComponent>();

	if (WorldItemComponent == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "requires SigilWorldItemComponent to function!")
	}
}

bool USigilItemPickupComponent::TryAddToCollection(USigilInventorySystemComponent* Picker)
{
	USigilItemInstance* NewItemInstance = WorldItemComponent->GetDuplicatedItemInstance(Picker->GetOwner());

	if (NewItemInstance == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "referenced invalid item! Pickup failed!");
		NotifyPickupFailed();
		return false;
	}

	FSigilItemInfo NewItemInfo;
	NewItemInfo.Item = NewItemInstance;
	NewItemInfo.Amount = WorldItemComponent->GetItemAmount();
	const FGameplayTag TargetCollection = CollectionTag.IsValid() ? CollectionTag : SigilCollectionTags::Main;
	NewItemInfo.CollectionTag = TargetCollection;

	FSigilItemInfo CanAddedItemInfo;
	const bool bResult = Picker->CanAddItem(NewItemInfo, CanAddedItemInfo);

	if (!bResult || CanAddedItemInfo.Amount == 0 || (bFailIfFullAmountNotFit && CanAddedItemInfo.Amount != NewItemInfo.Amount))
	{
		NotifyPickupFailed();
		return false;
	}

	Picker->AddItem(NewItemInfo);
	NotifyPickupSuccess();
	return true;
}
