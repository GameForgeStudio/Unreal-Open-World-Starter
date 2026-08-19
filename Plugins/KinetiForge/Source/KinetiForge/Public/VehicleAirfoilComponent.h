// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "VehicleAeroBaseComponent.h"
#include "VehicleUtilities.h"
#include "VehicleAirfoilComponent.generated.h"

USTRUCT(BlueprintType)
struct KINETIFORGE_API FVehicleAirfoilConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil", meta = (ClampMin = "0.01"))
	float WingArea = 1.0f;

	// Wingspan (span), used to calculate the aspect ratio and induced drag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil", meta = (ClampMin = "0.01"))
	float Span = 1.5f;

	// Oswald efficiency factor; for a standard tailplane, enter approximately 0.8
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float OswaldEfficiency = 0.8f;

	// X-axis: Actual angle of attack (degrees, -180 to 180), Y-axis: Lift coefficient CL
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil")
	FRuntimeFloatCurve LiftCurve;

	// X-axis: Actual angle of attack (degrees, -180 to 180), Y-axis: Basic parasitic drag coefficient CD_parasitic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil")
	FRuntimeFloatCurve DragCurve;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class KINETIFORGE_API UVehicleAirfoilComponent : public UVehicleAeroBaseComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVehicleAirfoilComponent();

	const FVehicleAirfoilConfig& GetConfig() { return Config; }
	void SetConfig(const FVehicleAirfoilConfig& NewConfig);

protected:
	virtual void CalculateCustomAeroForces(
		const FVector& ApparentWindWorld,
		const FVector& ChassisAngularVelWorld,
		const FTransform& CoPWorldTransform,
		FVector& OutAeroForce,
		FVector& OutAeroTorque
	) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	FVehicleAirfoilConfig Config;

	// TODO: Bake LUT
	FVehicleLUT<64> LiftLUT;
	FVehicleLUT<64> DragLUT;
};
