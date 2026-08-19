// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VehicleAeroBaseComponent.generated.h"

class UVehicleWheelCoordinatorComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class KINETIFORGE_API UVehicleAeroBaseComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVehicleAeroBaseComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    virtual void CalculateCustomAeroForces(
        const FVector& ApparentWindWorld, // unit£ºcm/s
        const FVector& ChassisAngularVelWorld,
        const FTransform& CoPWorldTransform,
        FVector& OutAeroForce,
        FVector& OutAeroTorque
    ) { /* Please override */ }

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void CalculateAerodynamics(
        const float PhysicsDeltaTime,
        const FVector& ParentCOMWorldLocation,
        const FVector& ParentWorldLinearVelocity,
        const FVector& ParentWorldAngularVelocity,
        const FTransform& ParentWorldTransform,
        const FVector& WindWorldVelocity,
        FVector& OutLinearForce,
        FVector& OutAngularTorque
    );

    UFUNCTION(BlueprintCallable, Category = "VehicleAeroBase")
    const FTransform& GetVirtualPartLocalTransform() { return VirtualPartLocalTransform; }

    UFUNCTION(BlueprintCallable, Category = "VehicleAeroBase")
    void SetVirtualPartLocalTransform(const FTransform& NewTransform, const bool bTeleport = false);

    /**
    * Offset relative to virtual aero part. (see VirtualPartLocalTransform)
    */
    UPROPERTY(EditAnywhere, Category = "Aerodynamics")
    FVector CenterOfPressureOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Aerodynamics")
    float AirDensity = 1.225f;

protected:

    UPROPERTY()
    TWeakObjectPtr<UPrimitiveComponent> Chassis;

    UPROPERTY()
	TWeakObjectPtr<UVehicleWheelCoordinatorComponent> WheelCoordinator;


    /**
    * There's a virtual aero part on this component. The virtual part simulates aero.
    * VirtualPartLocalTransform is the transform of the virtual aero part relative to this component.
    */
    FTransform VirtualPartLocalTransform;
    FTransform LastVirtualPartLocalTransform;
    bool bTeleportRequested = false;
};
