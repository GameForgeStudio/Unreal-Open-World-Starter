#pragma once

#include "Core/Fragments/SigilItemFragment.h"
#include "Core/Fragments/SigilItemFragment_Equippable.h"
#include "SakuraItemFragments.generated.h"

class UGameplayEffect;

UCLASS(DisplayName="Sakura Equipped Gameplay Effect", DefaultToInstanced, EditInlineNew)
class SAKURACORE_API USakuraItemFragment_EquippedEffect final : public USigilItemFragment
{
	GENERATED_BODY()

public:
	USakuraItemFragment_EquippedEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|GAS")
	TSubclassOf<UGameplayEffect> EquippedEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|GAS", meta=(ClampMin="0.01"))
	float EffectLevel = 1.0f;
};

UCLASS(DisplayName="Sakura Use Gameplay Effect", DefaultToInstanced, EditInlineNew)
class SAKURACORE_API USakuraItemFragment_UseEffect final : public USigilItemFragment
{
	GENERATED_BODY()

public:
	USakuraItemFragment_UseEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|GAS")
	TSubclassOf<UGameplayEffect> UseEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|GAS", meta=(ClampMin="0.01"))
	float EffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|Inventory", meta=(ClampMin="1"))
	int32 ConsumeCount = 1;

	// Prototype healing items are not consumed when they cannot change Health.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sakura|Inventory")
	bool bRequiresMissingHealth = true;
};

UCLASS(DisplayName="Sakura Prototype Equippable", DefaultToInstanced, EditInlineNew)
class SAKURACORE_API USakuraItemFragment_PrototypeEquippable final : public USigilItemFragment_Equippable
{
	GENERATED_BODY()

public:
	USakuraItemFragment_PrototypeEquippable();
};
