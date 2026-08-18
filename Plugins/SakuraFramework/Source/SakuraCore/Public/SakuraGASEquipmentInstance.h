#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Equipping/SigilEquipmentInstance.h"
#include "SakuraGASEquipmentInstance.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SAKURACORE_API USakuraGASEquipmentInstance final : public USigilEquipmentInstance
{
	GENERATED_BODY()

public:
	virtual void OnEquipmentBeginPlay_Implementation() override;
	virtual void OnEquipmentEndPlay_Implementation() override;

private:
	UPROPERTY(Transient)
	FActiveGameplayEffectHandle EquippedEffectHandle;
};
