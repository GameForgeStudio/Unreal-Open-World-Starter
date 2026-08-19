// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "OWSChaosVehicleInputRouterComponent.generated.h"

class UModularVehicleBaseComponent;

/**
 * Holds the already-shaped throttle action value and republishes it to Chaos Modular
 * Vehicle every pre-physics game-thread tick. Chaos remains the only live solver and
 * the only owner of vehicle input history and replication. Bind the action's active
 * event to SetShapedThrottleInput and its completed/canceled event to ClearThrottleInput.
 */
UCLASS(
	ClassGroup = (OWS, Vehicle),
	meta = (BlueprintSpawnableComponent, DisplayName = "OWS Chaos Vehicle Input Router"))
class OWSVEHICLE_API UOWSChaosVehicleInputRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWSChaosVehicleInputRouterComponent();

	/** Optional explicit target. If unset, the component finds one on its owner. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "OWS|Vehicle|Input")
	TObjectPtr<UModularVehicleBaseComponent> VehicleComponent = nullptr;

	/** Chaos input name configured by the engine's wheel/engine modules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Input")
	FName ThrottleInputName = TEXT("Throttle");

	/** Prevent remote proxies from overwriting network-predicted input with local cache data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Input")
	bool bRequireLocalControl = true;

	/** Cache the post-modifier Axis1D value received from the throttle Input Action. */
	UFUNCTION(BlueprintCallable, Category = "OWS|Vehicle|Input")
	void SetShapedThrottleInput(float ShapedThrottle);

	/** Clear the held sample when the Enhanced Input action completes or is canceled. */
	UFUNCTION(BlueprintCallable, Category = "OWS|Vehicle|Input")
	void ClearThrottleInput();

	/** Replace the target Chaos Modular Vehicle component. */
	UFUNCTION(BlueprintCallable, Category = "OWS|Vehicle|Input")
	void SetVehicleComponent(UModularVehicleBaseComponent* InVehicleComponent);

	UFUNCTION(BlueprintPure, Category = "OWS|Vehicle|Input")
	float GetShapedThrottleInput() const { return ShapedThrottleInput; }

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool ResolveVehicleComponent();

	float ShapedThrottleInput = 0.0f;
};
