// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "ChaosModularVehicle/VehicleSimSuspensionComponent.h"
#include "SakuraVehicleSimSuspensionComponent.generated.h"

/**
 * Stock Chaos suspension wrapper that exposes the protected skeletal-body
 * association to native vehicle assembly code. It does not replace the solver
 * module created by UVehicleSimSuspensionComponent.
 */
UCLASS(
	ClassGroup = (ModularVehicle),
	meta = (BlueprintSpawnableComponent, DisplayName = "Sakura Chaos Suspension"))
class SAKURAVEHICLE_API USakuraVehicleSimSuspensionComponent
	: public UVehicleSimSuspensionComponent
{
	GENERATED_BODY()

public:
	void SetAssociatedBoneName(FName InBoneName) { BoneName = InBoneName; }
};
