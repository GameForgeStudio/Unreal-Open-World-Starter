#pragma once

#include "Abilities/GameplayAbility.h"
#include "OWSPrototypeGameplayAbility.generated.h"

UCLASS()
class OWSCORE_API UOWSPrototypeGameplayAbility final : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UOWSPrototypeGameplayAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
