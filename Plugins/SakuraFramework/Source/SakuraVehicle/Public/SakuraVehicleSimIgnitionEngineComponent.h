// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "ChaosModularVehicle/VehicleSimEngineComponent.h"
#include "SakuraVehicleSimIgnitionEngineComponent.generated.h"

/**
 * Stock Chaos engine settings with a network-history Ignition control.
 *
 * UE 5.8's stock FEngineSimModule starts running in its constructor and has no
 * public ignition API. This wrapper creates a compatible FEngineSimModule
 * subclass that keeps Epic's existing net data, output data, and factory type,
 * while allowing an unoccupied vehicle to have a genuinely stopped engine.
 */
UCLASS(
	ClassGroup = (ModularVehicle),
	meta = (BlueprintSpawnableComponent, DisplayName = "Sakura Chaos Ignition Engine"))
class SAKURAVEHICLE_API USakuraVehicleSimIgnitionEngineComponent
	: public UVehicleSimEngineComponent
{
	GENERATED_BODY()

public:
	/** Local equivalent of the empty UE 5.8 callback, needed across MinimalAPI. */
	virtual void OnOutputReady(const Chaos::FSimOutputData* OutputData) override;

	/** Creates a stock-compatible Chaos engine module with ignition support. */
	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
