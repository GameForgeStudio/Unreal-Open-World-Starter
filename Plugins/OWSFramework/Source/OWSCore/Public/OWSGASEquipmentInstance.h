#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Equipping/SigilEquipmentInstance.h"
#include "OWSGASEquipmentInstance.generated.h"

UCLASS(BlueprintType, Blueprintable)
class OWSCORE_API UOWSGASEquipmentInstance final : public USigilEquipmentInstance
{
	GENERATED_BODY()

public:
	virtual void OnEquipmentBeginPlay_Implementation() override;
	virtual void OnEquipmentEndPlay_Implementation() override;

private:
	UPROPERTY(Transient)
	FActiveGameplayEffectHandle EquippedEffectHandle;
};
