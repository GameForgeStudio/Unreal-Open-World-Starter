// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Drops/SigilItemDropperComponent.h"
#include "SigilInventoryTags.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "SigilInventoryLogChannels.h"
#include "Pickups/SigilInventoryPickupComponent.h"
#include "Pickups/SigilItemPickupComponent.h"
#include "Pickups/SigilWorldItemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemDropperComponent)

void USigilItemDropperComponent::Drop()
{
	TArray<FSigilItemInfo> ItemsToDrop = GetItemsToDrop();
	
	DropItemsInternal(ItemsToDrop);
}

void USigilItemDropperComponent::BeginPlay()
{
	if (!CollectionTag.IsValid())
	{
		CollectionTag = SigilCollectionTags::Main;
	}

	Super::BeginPlay();
}

TArray<FSigilItemInfo> USigilItemDropperComponent::GetItemsToDrop() const
{
	return GetItemsToDropInternal();
}

TArray<FSigilItemInfo> USigilItemDropperComponent::GetItemsToDropInternal() const
{
	TArray<FSigilItemInfo> Items;

	USigilInventorySystemComponent* Inventory = USigilInventorySystemComponent::FindInventorySystemComponent(GetOwner());
	if (Inventory == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "requires inventory system component to drop items.")
		return Items;
	}
	USigilItemCollection* Collection = Inventory->GetCollectionByTag(CollectionTag);
	if (Collection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, " inventory missing collection with tag:%s'", *CollectionTag.ToString())
		return Items;
	}

	Items = Collection->GetAllItemInfos();
	return Items;
}

void USigilItemDropperComponent::DropItemsInternal(const TArray<FSigilItemInfo>& ItemInfos)
{
	if (bDropAsInventory)
	{
		DropInventoryPickup(ItemInfos);
	}
	else
	{
		for (int32 i = 0; i < ItemInfos.Num(); i++)
		{
			DropItemPickup(ItemInfos[i]);
		}
	}
}

void USigilItemDropperComponent::DropInventoryPickup(const TArray<FSigilItemInfo>& ItemInfos)
{
	if (AActor* PickupActor = CreatePickupActorInstance())
	{
		USigilInventorySystemComponent* Inventory = PickupActor->FindComponentByClass<USigilInventorySystemComponent>();
		USigilInventoryPickupComponent* Pickup = PickupActor->FindComponentByClass<USigilInventoryPickupComponent>();
		if (Inventory == nullptr || Pickup == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Error, "Spawned pickup(%s) missing either inventory component or inventory pickup component.", *PickupActor->GetName());
			return;
		}

		USigilItemCollection* Collection = Inventory->GetDefaultCollection();
		if (Collection == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Error, "Spawned pickup(%s)'s inventory doesn't have default collection.", *PickupActor->GetName());
			return;
		}
		Collection->RemoveAll();
		Collection->AddItems(ItemInfos);
	}
}

void USigilItemDropperComponent::DropItemPickup(const FSigilItemInfo& ItemInfo)
{
	if (AActor* Pickup = CreatePickupActorInstance())
	{
		USigilItemPickupComponent* ItemPickup = Pickup->FindComponentByClass<USigilItemPickupComponent>();
		USigilWorldItemComponent* WorldItem = Pickup->FindComponentByClass<USigilWorldItemComponent>();
		if (ItemPickup == nullptr || WorldItem == nullptr)
		{
			SIGIL_INVENTORY_CLOG(Error, "Spawned pickup(%s) missing either ItemPickup component or WorldItem component.", *Pickup->GetName());
			return;
		}
		WorldItem->SetItemInfo(ItemInfo.Item, ItemInfo.Amount);
	}
}
