#pragma once

#include "Abilities/GameplayAbility.h"
#include "SakuraPrototypeGameplayAbility.generated.h"

UCLASS()
class SAKURACORE_API USakuraPrototypeGameplayAbility final : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USakuraPrototypeGameplayAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
