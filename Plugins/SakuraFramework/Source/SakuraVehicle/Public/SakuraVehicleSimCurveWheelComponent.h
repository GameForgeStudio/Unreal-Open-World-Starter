// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Curve sampling behavior is adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "ChaosModularVehicle/VehicleSimWheelComponent.h"
#include "Curves/CurveFloat.h"
#include "SakuraVehicleSimCurveWheelComponent.generated.h"

/**
 * A Chaos Modular Vehicle wheel component that still creates Epic's stock
 * Chaos::FWheelSimModule, with a GT-baked lateral-slip graph added to its settings.
 * It does not create or run a second vehicle solver.
 */
UCLASS(
	ClassGroup = (ModularVehicle),
	meta = (BlueprintSpawnableComponent, DisplayName = "Sakura Chaos Curve Wheel"))
class SAKURAVEHICLE_API USakuraVehicleSimCurveWheelComponent : public UVehicleSimWheelComponent
{
	GENERATED_BODY()

public:
	USakuraVehicleSimCurveWheelComponent();

	/** Leave disabled for an exact stock FWheelSettings baseline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Lateral Slip")
	bool bEnableLateralSlipCurve = true;

	/**
	 * Inline curve input 0 is zero slip; input 1 is NormalizedSlipDegrees.
	 * An external UCurveFloat can be selected inside the runtime curve if desired.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Lateral Slip")
	FRuntimeFloatCurve NormalizedLateralSlipCurve;

	/** Positive slip angle in degrees represented by normalized curve input 1. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Sakura|Vehicle|Lateral Slip",
		meta = (ClampMin = "0.001", UIMin = "0.1"))
	float NormalizedSlipDegrees = 8.0f;

	/** Scales the stock linear cornering force represented at NormalizedSlipDegrees. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Sakura|Vehicle|Lateral Slip",
		meta = (ClampMin = "0.0"))
	float LateralForceScale = 1.0f;

	/** Assigned to stock FWheelSettings::LateralSlipGraphMultiplier after the graph is baked. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Sakura|Vehicle|Lateral Slip",
		meta = (ClampMin = "0.0"))
	float LateralSlipGraphMultiplier = 1.0f;

	/** Associate this module with a skeletal physics body and animation bone. */
	void SetAssociatedBoneName(FName InBoneName) { BoneName = InBoneName; }

	/**
	 * Local equivalent of Epic's wheel output callback. UE 5.8's implementation is
	 * not exported from the MinimalAPI engine class, so overriding the vtable slot
	 * here keeps cross-module linking valid while preserving Epic's native event.
	 */
	virtual void OnOutputReady(const Chaos::FSimOutputData* OutputData) override;

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
