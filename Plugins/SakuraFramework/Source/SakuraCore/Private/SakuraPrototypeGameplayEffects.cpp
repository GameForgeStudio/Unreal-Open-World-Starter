#include "SakuraPrototypeGameplayEffects.h"

#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "SakuraPrototypeAttributeSet.h"
#include "SakuraPrototypeGameplayAbility.h"

namespace
{
	void AddAttributeModifier(
		UGameplayEffect& Effect,
		const FGameplayAttribute& Attribute,
		const float Magnitude)
	{
		FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
	}
}

USakuraPrototypeEquippedEffect::USakuraPrototypeEquippedEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddAttributeModifier(*this, USakuraPrototypeAttributeSet::GetAttackPowerAttribute(), 10.0f);

	FGameplayAbilitySpecConfig AbilityConfig;
	AbilityConfig.Ability = USakuraPrototypeGameplayAbility::StaticClass();
	AbilityConfig.LevelScalableFloat = FScalableFloat(1.0f);
	AbilityConfig.RemovalPolicy = EGameplayEffectGrantedAbilityRemovePolicy::CancelAbilityImmediately;
	UAbilitiesGameplayEffectComponent* AbilityGrants =
		CreateDefaultSubobject<UAbilitiesGameplayEffectComponent>(TEXT("SakuraGrantedAbilities"));
	AbilityGrants->AddGrantedAbilityConfig(AbilityConfig);
	GEComponents.Add(AbilityGrants);
}

USakuraPrototypeHealEffect::USakuraPrototypeHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddAttributeModifier(*this, USakuraPrototypeAttributeSet::GetHealthAttribute(), 25.0f);
}

USakuraPrototypePulseEffect::USakuraPrototypePulseEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(2.0f);
	AddAttributeModifier(*this, USakuraPrototypeAttributeSet::GetAttackPowerAttribute(), 5.0f);
}
