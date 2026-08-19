// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Drops/SigilCurrencyDropper.h"

#include "SigilCurrencySystemComponent.h"
#include "SigilInventoryLogChannels.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencyDropper)

void USigilCurrencyDropper::BeginPlay()
{
	MyCurrency = USigilCurrencySystemComponent::GetCurrencySystemComponent(GetOwner());
	if (MyCurrency == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "Mising currency system component!");
	}
	Super::BeginPlay();
}

void USigilCurrencyDropper::Drop()
{
	if (AActor* PickupActor = CreatePickupActorInstance())
	{
		if (USigilCurrencySystemComponent* CurrencySys = USigilCurrencySystemComponent::GetCurrencySystemComponent(PickupActor))
		{
			CurrencySys->SetCurrencies(MyCurrency->GetAllCurrencies());
		}
	}
}


#if WITH_EDITOR
EDataValidationResult USigilCurrencyDropper::IsDataValid(FDataValidationContext& Context) const
{
	if (PickupActorClass.IsNull())
	{
		Context.AddError(FText::FromString(FString::Format(TEXT("%s has no pickup actor class.!"), {*GetName()})));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}
#endif
