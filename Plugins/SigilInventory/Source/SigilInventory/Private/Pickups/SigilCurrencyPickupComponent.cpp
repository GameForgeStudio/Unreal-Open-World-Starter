// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Pickups/SigilCurrencyPickupComponent.h"
#include "SigilCurrencySystemComponent.h"
#include "GameFramework/Actor.h"
#include "SigilInventorySystemComponent.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencyPickupComponent)

void USigilCurrencyPickupComponent::BeginPlay()
{
	OwningCurrencySystem = USigilCurrencySystemComponent::GetCurrencySystemComponent(GetOwner());
	if (OwningCurrencySystem == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "Mising CurrencySystemComponent!");
	}
	Super::BeginPlay();
}

bool USigilCurrencyPickupComponent::Pickup(USigilInventorySystemComponent* Picker)
{
	if (!GetOwner()->HasAuthority())
	{
		SIGIL_INVENTORY_CLOG(Warning, "has no authority!");
		return false;
	}
	if (OwningCurrencySystem == nullptr || !IsValid(OwningCurrencySystem))
	{
		SIGIL_INVENTORY_CLOG(Warning, "mising CurrencySystemComponent!");
		return false;
	}
	if (Picker == nullptr || !IsValid(Picker))
	{
		SIGIL_INVENTORY_CLOG(Warning, "passed-in invalid picker.");
		return false;
	}

	USigilCurrencySystemComponent* PickerCurrencySystem = Picker->GetCurrencySystem();
	if (PickerCurrencySystem == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "Picker:%s has no CurrencySystem!", *Picker->GetOwner()->GetName());
		return false;
	}

	if (PickerCurrencySystem->AddCurrencies(OwningCurrencySystem->GetAllCurrencies()))
	{
		OwningCurrencySystem->EmptyCurrencies();
		return true;
	}
	return false;
}
