#include "OWSPrototypeGameplayEffects.h"

#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "OWSPrototypeAttributeSet.h"
#include "OWSPrototypeGameplayAbility.h"

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

UOWSPrototypeEquippedEffect::UOWSPrototypeEquippedEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddAttributeModifier(*this, UOWSPrototypeAttributeSet::GetAttackPowerAttribute(), 10.0f);

	FGameplayAbilitySpecConfig AbilityConfig;
	AbilityConfig.Ability = UOWSPrototypeGameplayAbility::StaticClass();
	AbilityConfig.LevelScalableFloat = FScalableFloat(1.0f);
	AbilityConfig.RemovalPolicy = EGameplayEffectGrantedAbilityRemovePolicy::CancelAbilityImmediately;
	UAbilitiesGameplayEffectComponent* AbilityGrants =
		CreateDefaultSubobject<UAbilitiesGameplayEffectComponent>(TEXT("OWSGrantedAbilities"));
	AbilityGrants->AddGrantedAbilityConfig(AbilityConfig);
	GEComponents.Add(AbilityGrants);
}

UOWSPrototypeHealEffect::UOWSPrototypeHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddAttributeModifier(*this, UOWSPrototypeAttributeSet::GetHealthAttribute(), 25.0f);
}

UOWSPrototypePulseEffect::UOWSPrototypePulseEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(2.0f);
	AddAttributeModifier(*this, UOWSPrototypeAttributeSet::GetAttackPowerAttribute(), 5.0f);
}
