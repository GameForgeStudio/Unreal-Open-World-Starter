#include "OWSGASEquipmentInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "OWSCore.h"
#include "OWSItemFragments.h"
#include "Core/Items/SigilItemInstance.h"

void UOWSGASEquipmentInstance::OnEquipmentBeginPlay_Implementation()
{
	Super::OnEquipmentBeginPlay_Implementation();

	if (!IsValid(OwningPawn) || !OwningPawn->HasAuthority() || !IsValid(SourceItem) || EquippedEffectHandle.IsValid())
	{
		return;
	}

	const UOWSItemFragment_EquippedEffect* Fragment =
		SourceItem->FindFragmentByClass<UOWSItemFragment_EquippedEffect>();
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn);
	if (!Fragment || !Fragment->EquippedEffect || !AbilitySystem)
	{
		UE_LOG(LogOWSCore, Warning, TEXT("Equipped item %s is missing a valid OWS GAS effect or ASC."),
			*GetNameSafe(SourceItem));
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(SourceItem);
	const FGameplayEffectSpecHandle Spec =
		AbilitySystem->MakeOutgoingSpec(Fragment->EquippedEffect, Fragment->EffectLevel, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(LogOWSCore, Warning, TEXT("Could not create equipped effect spec for %s."), *GetNameSafe(SourceItem));
		return;
	}

	EquippedEffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (!EquippedEffectHandle.WasSuccessfullyApplied())
	{
		EquippedEffectHandle.Invalidate();
	}
}

void UOWSGASEquipmentInstance::OnEquipmentEndPlay_Implementation()
{
	if (IsValid(OwningPawn) && OwningPawn->HasAuthority() && EquippedEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* AbilitySystem =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn))
		{
			AbilitySystem->RemoveActiveGameplayEffect(EquippedEffectHandle);
		}
		EquippedEffectHandle.Invalidate();
	}

	Super::OnEquipmentEndPlay_Implementation();
}
