#include "SakuraGASEquipmentInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SakuraCore.h"
#include "SakuraItemFragments.h"
#include "Core/Items/SigilItemInstance.h"

void USakuraGASEquipmentInstance::OnEquipmentBeginPlay_Implementation()
{
	Super::OnEquipmentBeginPlay_Implementation();

	if (!IsValid(OwningPawn) || !OwningPawn->HasAuthority() || !IsValid(SourceItem) || EquippedEffectHandle.IsValid())
	{
		return;
	}

	const USakuraItemFragment_EquippedEffect* Fragment =
		SourceItem->FindFragmentByClass<USakuraItemFragment_EquippedEffect>();
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn);
	if (!Fragment || !Fragment->EquippedEffect || !AbilitySystem)
	{
		UE_LOG(LogSakuraCore, Warning, TEXT("Equipped item %s is missing a valid Sakura GAS effect or ASC."),
			*GetNameSafe(SourceItem));
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(SourceItem);
	const FGameplayEffectSpecHandle Spec =
		AbilitySystem->MakeOutgoingSpec(Fragment->EquippedEffect, Fragment->EffectLevel, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(LogSakuraCore, Warning, TEXT("Could not create equipped effect spec for %s."), *GetNameSafe(SourceItem));
		return;
	}

	EquippedEffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (!EquippedEffectHandle.WasSuccessfullyApplied())
	{
		EquippedEffectHandle.Invalidate();
	}
}

void USakuraGASEquipmentInstance::OnEquipmentEndPlay_Implementation()
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
