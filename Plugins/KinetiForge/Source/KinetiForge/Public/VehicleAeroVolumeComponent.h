// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)

#pragma once

#include "CoreMinimal.h"
#include "VehicleAeroBaseComponent.h"
#include "VehicleAeroVolumeComponent.generated.h"

USTRUCT(BlueprintType)
struct KINETIFORGE_API FVehicleAeroVolumeConfig
{
	GENERATED_BODY()

	// Three-axis frontal area (square meters, m^2)
	// X: Front and back surface area, Y: Side surface area, Z: Top/bottom surface area
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AeroVolume")
	FVector FrontalAreas = FVector(2.0f, 4.0f, 8.0f);

	// Three-axis translational drag coefficient (dimensionless)
	// For modern passenger cars, the X-axis coefficient typically ranges from 0.25 to 0.35
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AeroVolume")
	FVector DragCoefficients = FVector(0.3f, 0.8f, 1.0f);

	// X: Track width (e.g., 1.8 m), used for Roll
	// Y: Vehicle length/wheelbase (e.g., 4.5 m), used for Pitch
	// Z: Vehicle length/wheelbase (e.g., 4.5 m), used for Yaw
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AeroVolume")
	FVector ReferenceLengths = FVector(1.8f, 4.5f, 4.5f);

	// Three-axis rotation resistance coefficients (used to simulate the damping caused by displaced air during volumetric rotation, greatly enhancing physical stability)
	// X: Roll damping, Y: Pitch damping, Z: Yaw damping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AeroVolume")
	FVector AngularDragCoefficients = FVector(1.0f, 2.0f, 2.0f);
};

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class KINETIFORGE_API UVehicleAeroVolumeComponent : public UVehicleAeroBaseComponent
{
	GENERATED_BODY()

public:

	const FVehicleAeroVolumeConfig& GetConfig() { return Config; }
	void SetConfig(const FVehicleAeroVolumeConfig& NewConfig);

protected:
	virtual void CalculateCustomAeroForces(
		const FVector& ApparentWindWorld,
		const FVector& ChassisAngularVelWorld,
		const FTransform& CoPWorldTransform,
		FVector& OutAeroForce,
		FVector& OutAeroTorque
	) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	FVehicleAeroVolumeConfig Config;
};
