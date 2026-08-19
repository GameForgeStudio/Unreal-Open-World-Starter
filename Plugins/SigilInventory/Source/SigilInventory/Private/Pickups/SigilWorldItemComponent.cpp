// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "Pickups/SigilWorldItemComponent.h"
#include "UObject/Object.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySubsystem.h"
#include "SigilInventorySystemComponent.h"
#include "Items/SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilWorldItemComponent)

USigilWorldItemComponent::USigilWorldItemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	bReplicateUsingRegisteredSubObjectList = true;
}

void USigilWorldItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemInfo, Parameters)
}

USigilWorldItemComponent* USigilWorldItemComponent::GetWorldItemComponent(const AActor* Actor)
{
	if (IsValid(Actor))
	{
		return Actor->FindComponentByClass<USigilWorldItemComponent>();
	}
	return nullptr;
}

void USigilWorldItemComponent::CreateItemFromDefinition(FSigilItemDefinitionAmount ItemDefinition)
{
	if (!ItemDefinition.Definition.IsNull() && ItemDefinition.Amount >= 1)
	{
		if (!ItemInfo.IsValid())
		{
			USigilItemInstance* NewItemInstance = USigilInventorySubsystem::Get(GetWorld())->CreateItem(GetOwner(), ItemDefinition.Definition.LoadSynchronous());
			if (NewItemInstance == nullptr)
			{
				SIGIL_INVENTORY_CLOG(Error, "failed to create item instance from definition!");
			}
			else
			{
				SetItemInfo(NewItemInstance, ItemDefinition.Amount);
			}
		}
		else
		{
			SIGIL_INVENTORY_CLOG(Warning, "Already have valid item info, skip creation.")
		}
	}
	else
	{
		SIGIL_INVENTORY_CLOG(Error, "passed invalid definition setup,skip item instance creating!");
	}
}

bool USigilWorldItemComponent::HasValidDefinition() const
{
	return !Definition.Definition.IsNull() && Definition.Amount >= 1;
}


void USigilWorldItemComponent::SetItemInfo(USigilItemInstance* InItem, int32 InAmount)
{
	if (InItem == nullptr || InAmount <= 0)
	{
		return;
	}

	if (ItemInfo.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Warning, "Already have valid item info.")
		return;
	}

	ItemInfo = FSigilItemInfo(InItem, InAmount);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemInfo, this)

	// add to ReplicatedSubObject list only if it was created by this component. 
	if (bReplicateUsingRegisteredSubObjectList && InItem->GetOuter() == GetOwner())
	{
		AddReplicatedSubObject(ItemInfo.Item);
	}
}

void USigilWorldItemComponent::ResetItemInfo()
{
	if (ItemInfo.IsValid())
	{
		// remove from replicated sub object list only if it was created by this component. 
		if (bReplicateUsingRegisteredSubObjectList && ItemInfo.Item->GetOuter() == GetOwner())
		{
			RemoveReplicatedSubObject(ItemInfo.Item);
		}
	}
}

USigilItemInstance* USigilWorldItemComponent::GetItemInstance()
{
	return ItemInfo.Item;
}

USigilItemInstance* USigilWorldItemComponent::GetDuplicatedItemInstance(AActor* NewOwner)
{
	if (ItemInfo.IsValid())
	{
		return USigilInventorySubsystem::Get(GetWorld())->DuplicateItem(NewOwner, ItemInfo.Item);
	}
	return nullptr;
}

FSigilItemInfo USigilWorldItemComponent::GetItemInfo() const
{
	return ItemInfo;
}

int32 USigilWorldItemComponent::GetItemAmount() const
{
	return ItemInfo.Amount;
}

void USigilWorldItemComponent::BeginPlay()
{
	if (HasValidDefinition() && GetOwner()->HasAuthority())
	{
		CreateItemFromDefinition(Definition);
	}
	Super::BeginPlay();
}

void USigilWorldItemComponent::OnRep_ItemInfo()
{
	if (ItemInfo.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Verbose, "item:%s replicated!", *ItemInfo.Item->GetDefinition()->GetName());
		ItemInfoSetEvent.Broadcast(ItemInfo);
	}
}
