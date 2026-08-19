// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)

#pragma once

#include "CoreMinimal.h"
#include "AsyncTickActorComponent.h"
#include "VehicleAsyncTickComponent.generated.h"

class UVehicleDriveAssemblyComponent;
class UVehicleWheelCoordinatorComponent;
class UVehicleAsyncSpringArmComponent;
class FAsyncTickManager;

/**
 * This component recieves callback from physics thread and calls other components to update physics.
 * This component will be automatically generated if needed.
 */
UCLASS()
class KINETIFORGE_API UVehicleAsyncTickComponent : public UAsyncTickActorComponent
{
	GENERATED_BODY()

public:
	bool bUpdatePhysicsOnGameThread = false;

	static UVehicleAsyncTickComponent* FindVehicleAsyncTickComponent(AActor* VehicleActor);

	void RegisterDriveAssembly(UVehicleDriveAssemblyComponent* newDriveAssembly);
	void UnRegisterDriveAssembly(UVehicleDriveAssemblyComponent* targetDriveAssembly);

	void RegisterWheelCoordinator(UVehicleWheelCoordinatorComponent* newWheelCoordinator);
	void UnRegisterWheelCoordinator(UVehicleWheelCoordinatorComponent* targetWheelCoordinator);

	void RegisterAsyncSpringArm(UVehicleAsyncSpringArmComponent* newAsyncSpringArm);
	void UnRegisterAsyncSpringArm(UVehicleAsyncSpringArmComponent* targetAsyncSpringArm);

protected:
	TArray<TWeakObjectPtr<UVehicleDriveAssemblyComponent>> DriveAssemblies;
	TArray<TWeakObjectPtr<UVehicleWheelCoordinatorComponent>> WheelCoordinators;
	TArray<TWeakObjectPtr<UVehicleAsyncSpringArmComponent>> AsyncSpringArms;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

	/**
	* This function runs on worker thread (not safe but much better performance)
	*/
	virtual void NativeAsyncTick(float DeltaTime) override;

	/**
	* This function runs on frozen game thread, it is thread-safe but the performance is bad
	*/
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

private:
	void UpdateAllPhysics(float DeltaTime);
	FAsyncTickManager* CachedAsyncTickManager = nullptr;
};
