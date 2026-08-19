#pragma once

#include "GameplayEffect.h"
#include "OWSPrototypeGameplayEffects.generated.h"

UCLASS()
class OWSCORE_API UOWSPrototypeEquippedEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOWSPrototypeEquippedEffect();
};

UCLASS()
class OWSCORE_API UOWSPrototypeHealEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOWSPrototypeHealEffect();
};

UCLASS()
class OWSCORE_API UOWSPrototypePulseEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOWSPrototypePulseEffect();
};
