#pragma once

#include "GameplayEffect.h"
#include "SakuraPrototypeGameplayEffects.generated.h"

UCLASS()
class SAKURACORE_API USakuraPrototypeEquippedEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USakuraPrototypeEquippedEffect();
};

UCLASS()
class SAKURACORE_API USakuraPrototypeHealEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USakuraPrototypeHealEffect();
};

UCLASS()
class SAKURACORE_API USakuraPrototypePulseEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USakuraPrototypePulseEffect();
};
