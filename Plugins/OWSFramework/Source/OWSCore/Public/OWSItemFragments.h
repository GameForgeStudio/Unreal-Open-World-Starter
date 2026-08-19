#pragma once

#include "Core/Fragments/SigilItemFragment.h"
#include "Core/Fragments/SigilItemFragment_Equippable.h"
#include "OWSItemFragments.generated.h"

class UGameplayEffect;

UCLASS(DisplayName="OWS Equipped Gameplay Effect", DefaultToInstanced, EditInlineNew)
class OWSCORE_API UOWSItemFragment_EquippedEffect final : public USigilItemFragment
{
	GENERATED_BODY()

public:
	UOWSItemFragment_EquippedEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|GAS")
	TSubclassOf<UGameplayEffect> EquippedEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|GAS", meta=(ClampMin="0.01"))
	float EffectLevel = 1.0f;
};

UCLASS(DisplayName="OWS Use Gameplay Effect", DefaultToInstanced, EditInlineNew)
class OWSCORE_API UOWSItemFragment_UseEffect final : public USigilItemFragment
{
	GENERATED_BODY()

public:
	UOWSItemFragment_UseEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|GAS")
	TSubclassOf<UGameplayEffect> UseEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|GAS", meta=(ClampMin="0.01"))
	float EffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Inventory", meta=(ClampMin="1"))
	int32 ConsumeCount = 1;

	// Prototype healing items are not consumed when they cannot change Health.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Inventory")
	bool bRequiresMissingHealth = true;
};

UCLASS(DisplayName="OWS Prototype Equippable", DefaultToInstanced, EditInlineNew)
class OWSCORE_API UOWSItemFragment_PrototypeEquippable final : public USigilItemFragment_Equippable
{
	GENERATED_BODY()

public:
	UOWSItemFragment_PrototypeEquippable();
};
