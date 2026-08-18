#include "SakuraItemFragments.h"

#include "SakuraGASEquipmentInstance.h"
#include "SakuraPrototypeGameplayEffects.h"

USakuraItemFragment_EquippedEffect::USakuraItemFragment_EquippedEffect()
{
	EquippedEffect = USakuraPrototypeEquippedEffect::StaticClass();
}

USakuraItemFragment_UseEffect::USakuraItemFragment_UseEffect()
{
	UseEffect = USakuraPrototypeHealEffect::StaticClass();
}

USakuraItemFragment_PrototypeEquippable::USakuraItemFragment_PrototypeEquippable()
{
	InstanceType = USakuraGASEquipmentInstance::StaticClass();
	bAutoActivate = true;
}
