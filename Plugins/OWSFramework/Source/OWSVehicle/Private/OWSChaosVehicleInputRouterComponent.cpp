// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "OWSChaosVehicleInputRouterComponent.h"

#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSChaosVehicleInputRouterComponent)

UOWSChaosVehicleInputRouterComponent::UOWSChaosVehicleInputRouterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(false);
}

void UOWSChaosVehicleInputRouterComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveVehicleComponent();
}

void UOWSChaosVehicleInputRouterComponent::TickComponent(
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

void UOWSChaosVehicleInputRouterComponent::SetShapedThrottleInput(const float ShapedThrottle)
{
	ShapedThrottleInput = FMath::Clamp(ShapedThrottle, 0.0f, 1.0f);
}

void UOWSChaosVehicleInputRouterComponent::ClearThrottleInput()
{
	ShapedThrottleInput = 0.0f;
}

void UOWSChaosVehicleInputRouterComponent::SetVehicleComponent(
	UModularVehicleBaseComponent* InVehicleComponent)
{
	VehicleComponent = InVehicleComponent;
}

bool UOWSChaosVehicleInputRouterComponent::ResolveVehicleComponent()
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
