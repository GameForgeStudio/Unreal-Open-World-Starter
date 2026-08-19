// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Pickups/SigilInventoryPickupComponent.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySystemComponent.h"
#include "SigilInventoryTags.h"
#include "SigilItemCollection.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventoryPickupComponent)

void USigilInventoryPickupComponent::BeginPlay()
{
	if (!CollectionTag.IsValid())
	{
		CollectionTag = SigilCollectionTags::Main;
	}

	Inventory = USigilInventorySystemComponent::FindInventorySystemComponent(GetOwner());
	if (!Inventory)
	{
		SIGIL_INVENTORY_CLOG(Warning, "InventoryPickup requries an inventory system component on the same actor!")
	}

	Super::BeginPlay();
}

bool USigilInventoryPickupComponent::Pickup(USigilInventorySystemComponent* Picker)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "has no authority!");
		return false;
	}
	if (Inventory == nullptr || !IsValid(Inventory))
	{
		SIGIL_INVENTORY_CLOG(Warning, "doesn't have an inventory system component to function.")
		return false;
	}
	if (!CollectionTag.IsValid() || !IsValid(Picker))
	{
		SIGIL_INVENTORY_CLOG(Warning, "doesn't have valid picker to function.")
		return false;
	}

	USigilItemCollection* DestCollection = Picker->GetCollectionByTag(CollectionTag);
	if (DestCollection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "picker(%s) doesn't have valid collection named:%s", *Picker->GetOwner()->GetName(), *CollectionTag.ToString());
		return false;
	}

	return AddPickupToCollection(DestCollection);
}

USigilInventorySystemComponent* USigilInventoryPickupComponent::GetOwningInventory() const
{
	return Inventory;
}

bool USigilInventoryPickupComponent::AddPickupToCollection(USigilItemCollection* DestCollection)
{
	TArray<FSigilItemInfo> PickupItems = Inventory->GetDefaultCollection()->GetAllItemInfos();
	bool bAtLeastOneCanBeAdded = false;
	for (int32 i = 0; i < PickupItems.Num(); i++)
	{
		FSigilItemInfo ItemInfo = PickupItems[i];
		FSigilItemInfo CanAddedItemInfo;
		if (DestCollection->CanAddItem(ItemInfo, CanAddedItemInfo))
		{
			if (CanAddedItemInfo.Amount != 0)
			{
				bAtLeastOneCanBeAdded = true;
			}
		}
	}
	if (bAtLeastOneCanBeAdded == false)
	{
		NotifyPickupFailed();
		return false;
	}

	for (int32 i = 0; i < PickupItems.Num(); i++)
	{
		DestCollection->AddItem(PickupItems[i]);
	}
	Inventory->GetDefaultCollection()->RemoveAll();
	NotifyPickupSuccess();
	return true;
}
