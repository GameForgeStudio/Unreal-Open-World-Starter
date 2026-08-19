#include "OWSItemFragments.h"

#include "OWSGASEquipmentInstance.h"
#include "OWSPrototypeGameplayEffects.h"

UOWSItemFragment_EquippedEffect::UOWSItemFragment_EquippedEffect()
{
	EquippedEffect = UOWSPrototypeEquippedEffect::StaticClass();
}

UOWSItemFragment_UseEffect::UOWSItemFragment_UseEffect()
{
	UseEffect = UOWSPrototypeHealEffect::StaticClass();
}

UOWSItemFragment_PrototypeEquippable::UOWSItemFragment_PrototypeEquippable()
{
	InstanceType = UOWSGASEquipmentInstance::StaticClass();
	bAutoActivate = true;
}
