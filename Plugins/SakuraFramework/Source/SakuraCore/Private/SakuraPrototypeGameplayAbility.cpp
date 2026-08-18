#include "SakuraPrototypeGameplayAbility.h"

#include "SakuraPrototypeGameplayEffects.h"

USakuraPrototypeGameplayAbility::USakuraPrototypeGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void USakuraPrototypeGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UGameplayEffect* PulseEffect = GetDefault<USakuraPrototypePulseEffect>();
	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, PulseEffect, GetAbilityLevel(Handle, ActorInfo));
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
