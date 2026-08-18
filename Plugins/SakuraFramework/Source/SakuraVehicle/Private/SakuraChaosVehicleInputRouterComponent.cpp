// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraChaosVehicleInputRouterComponent.h"

#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SakuraChaosVehicleInputRouterComponent)

USakuraChaosVehicleInputRouterComponent::USakuraChaosVehicleInputRouterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(false);
}

void USakuraChaosVehicleInputRouterComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveVehicleComponent();
}

void USakuraChaosVehicleInputRouterComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ResolveVehicleComponent() || ThrottleInputName.IsNone())
	{
		return;
	}

	if (bRequireLocalControl && !VehicleComponent->IsLocallyControlled())
	{
		return;
	}

	// The default Chaos producer clears consumed input, so publish the held sample every frame.
	VehicleComponent->SetInputAxis1D(
		ThrottleInputName,
		static_cast<double>(ShapedThrottleInput));
}

void USakuraChaosVehicleInputRouterComponent::SetShapedThrottleInput(const float ShapedThrottle)
{
	ShapedThrottleInput = FMath::Clamp(ShapedThrottle, 0.0f, 1.0f);
}

void USakuraChaosVehicleInputRouterComponent::ClearThrottleInput()
{
	ShapedThrottleInput = 0.0f;
}

void USakuraChaosVehicleInputRouterComponent::SetVehicleComponent(
	UModularVehicleBaseComponent* InVehicleComponent)
{
	VehicleComponent = InVehicleComponent;
}

bool USakuraChaosVehicleInputRouterComponent::ResolveVehicleComponent()
{
	if (IsValid(VehicleComponent))
	{
		return true;
	}

	if (AActor* Owner = GetOwner())
	{
		VehicleComponent = Owner->FindComponentByClass<UModularVehicleBaseComponent>();
	}

	return IsValid(VehicleComponent);
}
