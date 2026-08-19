// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "OWSChaosVehicleDemoPawn.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "ChaosModularVehicle/VehicleSimChassisComponent.h"
#include "ChaosModularVehicle/VehicleSimClutchComponent.h"
#include "ChaosModularVehicle/VehicleSimTransmissionComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifier_OWSKinetiThrottle.h"
#include "Net/UnrealNetwork.h"
#include "OWSChaosVehicleInputRouterComponent.h"
#include "OWSVehicleSimCurveWheelComponent.h"
#include "OWSVehicleSimIgnitionEngineComponent.h"
#include "OWSVehicleSimSuspensionComponent.h"
#include "SimModule/ModuleInput.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSChaosVehicleDemoPawn)

namespace OWS::Vehicle::Demo
{
	const FName ThrottleInputName(TEXT("Throttle"));
	const FName BrakeInputName(TEXT("Brake"));
	const FName SteeringInputName(TEXT("Steering"));
	const FName HandbrakeInputName(TEXT("Handbrake"));
	const FName ReverseInputName(TEXT("Reverse"));
	const FName IgnitionInputName(TEXT("Ignition"));

	const FName FrontLeftBone(TEXT("Phys_Wheel_FL"));
	const FName FrontRightBone(TEXT("Phys_Wheel_FR"));
	const FName BackLeftBone(TEXT("Phys_Wheel_BL"));
	const FName BackRightBone(TEXT("Phys_Wheel_BR"));
}

AOWSChaosVehicleDemoPawn::AOWSChaosVehicleDemoPawn(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace OWS::Vehicle::Demo;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bReplicates = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	FOWSVehicleSeatDefinition DriverSeat;
	DriverSeat.SeatId = TEXT("FrontLeft");
	DriverSeat.SeatType = TEXT("Driver");
	DriverSeat.bControlsVehicle = true;
	DriverSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(20.0f, -45.0f, 55.0f));
	SeatDefinitions.Add(DriverSeat);

	FOWSVehicleSeatDefinition PassengerSeat;
	PassengerSeat.SeatId = TEXT("FrontRight");
	PassengerSeat.SeatType = TEXT("Passenger");
	PassengerSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(20.0f, 45.0f, 55.0f));
	SeatDefinitions.Add(PassengerSeat);

	FOWSVehicleDoorDefinition LeftDoor;
	LeftDoor.DoorId = TEXT("LeftDoor");
	LeftDoor.RelativeTransform = FTransform(
		FRotator(0.0f, -90.0f, 0.0f),
		FVector(15.0f, -125.0f, 55.0f));
	LeftDoor.OrderedSeatIds = {DriverSeat.SeatId, PassengerSeat.SeatId};
	DoorDefinitions.Add(LeftDoor);

	FOWSVehicleDoorDefinition RightDoor;
	RightDoor.DoorId = TEXT("RightDoor");
	RightDoor.RelativeTransform = FTransform(
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(15.0f, 125.0f, 55.0f));
	RightDoor.OrderedSeatIds = {PassengerSeat.SeatId, DriverSeat.SeatId};
	DoorDefinitions.Add(RightDoor);

	ExampleSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Models/SportsCar/SKM_SportsCar.SKM_SportsCar")));
	ExampleAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Vehicles/ModularVehicle/SportsCar/"
			"SportsCar_ModularAnimBP.SportsCar_ModularAnimBP_C")));
	ExampleInputMapping = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/"
			"VehicleInputMappingContext.VehicleInputMappingContext")));
	ThrottleActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Throttle.Throttle")));
	BrakeActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Brake.Brake")));
	SteeringActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Steering.Steering")));
	HandbrakeActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Handbrake.Handbrake")));
	ReverseActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Reverse.Reverse")));
	LookActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(
		TEXT("/ChaosModularVehicleExamples/Input/Look.Look")));

	USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	UModularVehicleBaseComponent* VehicleComponent = GetVehicleSimulationComponent();
	check(VehicleMesh != nullptr);
	check(VehicleComponent != nullptr);

	VehicleMesh->SetCollisionProfileName(TEXT("OWSVehicle"));
	VehicleMesh->SetEnableGravity(true);
	VehicleMesh->SetSimulatePhysics(true);
	VehicleMesh->SetLinearDamping(0.08f);
	VehicleMesh->SetAngularDamping(0.65f);
	VehicleComponent->SetUpdatedComponent(VehicleMesh);
	VehicleComponent->LinearDamping = 0.08f;
	VehicleComponent->AngularDamping = 0.65f;
	VehicleComponent->bKeepVehicleAwake = false;
	VehicleComponent->InputConfig.Reset();
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		SteeringInputName,
		EModuleInputValueType::MAxis1D));
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		ThrottleInputName,
		EModuleInputValueType::MAxis1D));
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		BrakeInputName,
		EModuleInputValueType::MAxis1D));
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		HandbrakeInputName,
		EModuleInputValueType::MBoolean));
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		ReverseInputName,
		EModuleInputValueType::MBoolean));
	VehicleComponent->InputConfig.Add(FModuleInputSetup(
		IgnitionInputName,
		EModuleInputValueType::MBoolean));

	InputRouter = CreateDefaultSubobject<UOWSChaosVehicleInputRouterComponent>(
		TEXT("OWSInputRouter"));
	InputRouter->SetVehicleComponent(VehicleComponent);
	InputRouter->AddTickPrerequisiteActor(this);

	ThrottleShaper = CreateDefaultSubobject<UInputModifier_OWSKinetiThrottle>(
		TEXT("OWSThrottleShaper"));

	Chassis = CreateDefaultSubobject<UVehicleSimChassisComponent>(
		TEXT("VehicleSimChassis"));
	Chassis->SetupAttachment(VehicleMesh);

	Engine = CreateDefaultSubobject<UOWSVehicleSimIgnitionEngineComponent>(
		TEXT("VehicleSimEngine"));
	Engine->SetupAttachment(Chassis);
	Engine->MaxTorque = 500.0f;
	Engine->MaxRPM = 6500;
	Engine->EngineIdleRPM = 1200;
	Engine->EngineBrakeEffect = 150.0f;
	Engine->EngineInertia = 1000.0f;

	Clutch = CreateDefaultSubobject<UVehicleSimClutchComponent>(TEXT("VehicleSimClutch"));
	Clutch->SetupAttachment(Engine);
	Clutch->ClutchStrength = 1.0f;

	Transmission = CreateDefaultSubobject<UVehicleSimTransmissionComponent>(
		TEXT("VehicleSimTransmission"));
	Transmission->SetupAttachment(Clutch);
	Transmission->ForwardRatios = {3.09f, 1.89f, 1.32f, 1.0f, 0.75f};
	Transmission->ReverseRatios = {2.86f};
	Transmission->FinalDriveRatio = 4.0f;
	Transmission->ChangeUpRPM = 4000;
	Transmission->ChangeDownRPM = 1600;
	Transmission->GearChangeTime = 0.25f;
	Transmission->GearHysteresisTime = 2.0f;
	Transmission->TransmissionEfficiency = 0.9f;
	Transmission->TransmissionType = EModuleTransType::Automatic;
	Transmission->AutoReverse = true;

	FrontLeftSuspension =
		CreateDefaultSubobject<UOWSVehicleSimSuspensionComponent>(
			TEXT("FL_VehicleSimSuspension"));
	FrontLeftSuspension->SetupAttachment(VehicleMesh);
	ConfigureSuspension(
		FrontLeftSuspension,
		FrontLeftBone,
		FVector(130.0, -90.0, 20.0));

	FrontRightSuspension =
		CreateDefaultSubobject<UOWSVehicleSimSuspensionComponent>(
			TEXT("FR_VehicleSimSuspension"));
	FrontRightSuspension->SetupAttachment(VehicleMesh);
	ConfigureSuspension(
		FrontRightSuspension,
		FrontRightBone,
		FVector(130.0, 90.0, 20.0));

	BackLeftSuspension =
		CreateDefaultSubobject<UOWSVehicleSimSuspensionComponent>(
			TEXT("BL_VehicleSimSuspension"));
	BackLeftSuspension->SetupAttachment(VehicleMesh);
	ConfigureSuspension(
		BackLeftSuspension,
		BackLeftBone,
		FVector(-130.0, -90.0, 20.0));

	BackRightSuspension =
		CreateDefaultSubobject<UOWSVehicleSimSuspensionComponent>(
			TEXT("BR_VehicleSimSuspension"));
	BackRightSuspension->SetupAttachment(VehicleMesh);
	ConfigureSuspension(
		BackRightSuspension,
		BackRightBone,
		FVector(-130.0, 90.0, 20.0));

	FrontLeftWheel = CreateDefaultSubobject<UOWSVehicleSimCurveWheelComponent>(
		TEXT("VehicleSimWheel_FL"));
	FrontLeftWheel->SetupAttachment(FrontLeftSuspension);
	ConfigureWheel(FrontLeftWheel, FrontLeftBone, true);

	FrontRightWheel = CreateDefaultSubobject<UOWSVehicleSimCurveWheelComponent>(
		TEXT("VehicleSimWheel_FR"));
	FrontRightWheel->SetupAttachment(FrontRightSuspension);
	ConfigureWheel(FrontRightWheel, FrontRightBone, true);

	BackLeftWheel = CreateDefaultSubobject<UOWSVehicleSimCurveWheelComponent>(
		TEXT("VehicleSimWheel_BL"));
	BackLeftWheel->SetupAttachment(BackLeftSuspension);
	ConfigureWheel(BackLeftWheel, BackLeftBone, false);

	BackRightWheel = CreateDefaultSubobject<UOWSVehicleSimCurveWheelComponent>(
		TEXT("VehicleSimWheel_BR"));
	BackRightWheel->SetupAttachment(BackRightSuspension);
	ConfigureWheel(BackRightWheel, BackRightBone, false);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(VehicleMesh);
	CameraBoom->SetRelativeLocation(FVector(0.0, 0.0, 150.0));
	CameraBoom->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
	CameraBoom->TargetArmLength = 650.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	ApplyExampleAssets();
	ApplyComparisonMode();
}

AOWSChaosVehicleStockDemoPawn::AOWSChaosVehicleStockDemoPawn(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ComparisonMode = EOWSChaosVehicleDemoMode::StockBaseline;
	ApplyComparisonMode();
}

void AOWSChaosVehicleDemoPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bParked)
	{
		UpdateParkedState(DeltaSeconds);
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	const USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	const float ForwardSpeed = VehicleMesh != nullptr
		? FVector::DotProduct(
			VehicleMesh->GetPhysicsLinearVelocity(),
			GetActorForwardVector())
		: 0.0f;
	constexpr float DirectionChangeSpeed = 50.0f;
	constexpr float MeaningfulInput = 0.05f;

	float DriveInput = FMath::Clamp(RawThrottleInput, 0.0f, 1.0f);
	float EffectiveBrake = FMath::Clamp(BrakeInput, 0.0f, 1.0f);
	if (DriveInput > MeaningfulInput)
	{
		bIgnitionRequested = true;
		bDriveReleased = true;
		if (ForwardSpeed < -DirectionChangeSpeed)
		{
			// Like Horizon's automatic layout, the forward accelerator first
			// brakes an already-reversing car before selecting a forward gear.
			EffectiveBrake = FMath::Max(EffectiveBrake, DriveInput);
			DriveInput = 0.0f;
		}
		else
		{
			bReverseInput = false;
		}
	}
	else if (EffectiveBrake > MeaningfulInput)
	{
		if (ForwardSpeed > DirectionChangeSpeed)
		{
			// LT remains the service brake while the car is moving forward.
			bReverseInput = false;
		}
		else
		{
			// At rest (or already moving backward), LT becomes proportional
			// reverse acceleration and also starts an engine that is still off.
			bReverseInput = true;
			bIgnitionRequested = true;
			bDriveReleased = true;
			DriveInput = EffectiveBrake;
			EffectiveBrake = 0.0f;
		}
	}

	// Possession selects drive mode but must not release a parked, engine-off
	// vehicle on a slope. X starts the engine in place; the first actual forward
	// or reverse accelerator releases this hold and begins motion.
	if (!bDriveReleased)
	{
		if (InputRouter != nullptr)
		{
			InputRouter->SetShapedThrottleInput(0.0f);
		}
		if (!bIgnitionRequested)
		{
			USkeletalMeshComponent* MutableVehicleMesh = GetSkeletalMeshComponent();
			if (MutableVehicleMesh != nullptr &&
				!bParkingBrakeActive && MutableVehicleMesh->IsAnyRigidBodyAwake())
			{
				// A real impact is allowed to move the held car. Re-engage the brakes
				// and let Chaos resolve that momentum instead of deleting it.
				bParkingBrakeActive = true;
				ParkingStableTime = 0.0f;
			}

			if (!bParkingBrakeActive || MutableVehicleMesh == nullptr)
			{
				PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, false);
				return;
			}

			PublishVehicleControls(0.0f, 1.0f, 0.0f, true, false, false);
			const float LinearSpeed = MutableVehicleMesh->GetPhysicsLinearVelocity().Size();
			const float AngularSpeed =
				MutableVehicleMesh->GetPhysicsAngularVelocityInDegrees().Size();
			if (LinearSpeed <= ParkingSleepLinearSpeed &&
				AngularSpeed <= ParkingSleepAngularSpeed)
			{
				ParkingStableTime += DeltaSeconds;
			}
			else
			{
				ParkingStableTime = 0.0f;
			}

			if (ParkingStableTime >= ParkingSettleSeconds)
			{
				PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, false);
				MutableVehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
				MutableVehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				MutableVehicleMesh->PutAllRigidBodiesToSleep();
				bParkingBrakeActive = false;
				ParkingStableTime = 0.0f;
			}
			return;
		}
		PublishVehicleControls(
			0.0f,
			1.0f,
			0.0f,
			true,
			false,
			bIgnitionRequested);
		return;
	}
	if (UModularVehicleBaseComponent* VehicleComponent =
		GetVehicleSimulationComponent())
	{
		VehicleComponent->bKeepVehicleAwake = true;
	}

	float RoutedThrottle = DriveInput;
	if (ComparisonMode == EOWSChaosVehicleDemoMode::OWSHybrid &&
		ThrottleShaper != nullptr)
	{
		RoutedThrottle = ThrottleShaper->ModifyRaw(
			nullptr,
			FInputActionValue(RoutedThrottle),
			DeltaSeconds).Get<float>();
	}

	if (InputRouter != nullptr)
	{
		InputRouter->SetShapedThrottleInput(RoutedThrottle);
	}

	if (UModularVehicleBaseComponent* VehicleComponent =
		GetVehicleSimulationComponent())
	{
		PublishVehicleControls(
			RoutedThrottle,
			EffectiveBrake,
			SteeringInput,
			bHandbrakeInput,
			bReverseInput,
			bIgnitionRequested);
	}
}

void AOWSChaosVehicleDemoPawn::RequestIgnitionStart()
{
	if (!bParked && GetController() != nullptr)
	{
		bIgnitionRequested = true;
		if (UModularVehicleBaseComponent* VehicleComponent =
			GetVehicleSimulationComponent())
		{
			VehicleComponent->bKeepVehicleAwake = true;
		}
		if (USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent())
		{
			VehicleMesh->WakeAllRigidBodies();
		}
	}
}

bool AOWSChaosVehicleDemoPawn::FindFacedDoor(
	const FVector& ActivatorLocation,
	const FVector& ActivatorForward,
	const float MaximumDistance,
	FName& OutDoorId,
	float& OutDistance) const
{
	OutDoorId = NAME_None;
	OutDistance = -1.0f;
	FVector Facing = ActivatorForward;
	Facing.Z = 0.0f;
	if (!Facing.Normalize())
	{
		return false;
	}

	float BestFacing = MinimumDoorFacingDot;
	float BestDistance = MaximumDistance;
	for (const FOWSVehicleDoorDefinition& Door : DoorDefinitions)
	{
		if (Door.DoorId.IsNone())
		{
			continue;
		}

		const FTransform DoorWorld = Door.RelativeTransform * GetActorTransform();
		FVector ToDoor = DoorWorld.GetLocation() - ActivatorLocation;
		ToDoor.Z = 0.0f;
		const float Distance = ToDoor.Size();
		if (Distance > MaximumDistance || Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		ToDoor /= Distance;
		const float FacingDot = FVector::DotProduct(Facing, ToDoor);
		if (FacingDot < MinimumDoorFacingDot)
		{
			continue;
		}

		if (FacingDot > BestFacing + KINDA_SMALL_NUMBER ||
			(FMath::IsNearlyEqual(FacingDot, BestFacing) && Distance < BestDistance))
		{
			BestFacing = FacingDot;
			BestDistance = Distance;
			OutDoorId = Door.DoorId;
		}
	}

	if (OutDoorId.IsNone())
	{
		return false;
	}
	OutDistance = BestDistance;
	return true;
}

bool AOWSChaosVehicleDemoPawn::SelectAvailableSeat(
	const FName DoorId,
	const bool bAutoPlacementEnabled,
	const AActor* EnteringActor,
	FName& OutSeatId) const
{
	OutSeatId = NAME_None;
	const FOWSVehicleDoorDefinition* Door = FindDoorDefinition(DoorId);
	if (Door == nullptr || Door->OrderedSeatIds.IsEmpty())
	{
		return false;
	}

	if (bAutoPlacementEnabled)
	{
		// Auto placement only overrides the faced door while the vehicle has no
		// driver/control-seat occupant. Once somebody is driving, selection stays
		// local to the door the activating actor actually approached.
		const bool bControlSeatReservedByController = GetController() != nullptr;
		if (!HasControlSeatOccupant() && !bControlSeatReservedByController)
		{
			for (const FOWSVehicleSeatDefinition& Seat : SeatDefinitions)
			{
				if (Seat.bControlsVehicle && IsSeatAvailable(Seat.SeatId, EnteringActor))
				{
					OutSeatId = Seat.SeatId;
					return true;
				}
			}
		}

		for (const FName CandidateSeatId : Door->OrderedSeatIds)
		{
			// A controller assigned outside OWS's occupancy registry still owns
			// vehicle authority. Treat every control seat as unavailable instead of
			// selecting one and rejecting the whole activation later.
			if (bControlSeatReservedByController && IsControlSeat(CandidateSeatId))
			{
				continue;
			}
			if (IsSeatAvailable(CandidateSeatId, EnteringActor))
			{
				OutSeatId = CandidateSeatId;
				return true;
			}
		}
		return false;
	}

	// Manual placement never silently repositions the player. The primary seat
	// assigned to the faced door either accepts this actor or entry is rejected.
	const FName RequestedSeatId = Door->OrderedSeatIds[0];
	if ((GetController() != nullptr && IsControlSeat(RequestedSeatId)) ||
		!IsSeatAvailable(RequestedSeatId, EnteringActor))
	{
		return false;
	}
	OutSeatId = RequestedSeatId;
	return true;
}

bool AOWSChaosVehicleDemoPawn::OccupySeat(
	const FName SeatId,
	AActor* OccupantActor)
{
	if (!HasAuthority() || !IsValid(OccupantActor) ||
		GetSeatForOccupant(OccupantActor) != NAME_None ||
		!IsSeatAvailable(SeatId, OccupantActor))
	{
		return false;
	}

	for (FOWSVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.SeatId == SeatId)
		{
			State.Occupant = OccupantActor;
			ForceNetUpdate();
			return true;
		}
	}

	return false;
}

bool AOWSChaosVehicleDemoPawn::ReleaseSeat(AActor* OccupantActor)
{
	if (!HasAuthority() || !IsValid(OccupantActor))
	{
		return false;
	}

	for (FOWSVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.Occupant == OccupantActor)
		{
			State.Occupant = nullptr;
			ForceNetUpdate();
			return true;
		}
	}

	// Idempotent release lets an authority-side exit finish safely if another
	// authoritative lifecycle event already released this actor's reservation.
	return true;
}

bool AOWSChaosVehicleDemoPawn::GetSeatWorldTransform(
	const FName SeatId,
	FTransform& OutTransform) const
{
	if (const FOWSVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId))
	{
		OutTransform = Seat->RelativeTransform * GetActorTransform();
		return true;
	}
	return false;
}

bool AOWSChaosVehicleDemoPawn::GetDoorWorldTransform(
	const FName DoorId,
	FTransform& OutTransform) const
{
	if (const FOWSVehicleDoorDefinition* Door = FindDoorDefinition(DoorId))
	{
		OutTransform = Door->RelativeTransform * GetActorTransform();
		return true;
	}
	return false;
}

bool AOWSChaosVehicleDemoPawn::IsControlSeat(const FName SeatId) const
{
	const FOWSVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	return Seat != nullptr && Seat->bControlsVehicle;
}

FName AOWSChaosVehicleDemoPawn::GetSeatForOccupant(
	const AActor* OccupantActor) const
{
	if (OccupantActor == nullptr)
	{
		return NAME_None;
	}
	for (const FOWSVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.Occupant == OccupantActor)
		{
			return State.SeatId;
		}
	}
	return NAME_None;
}

bool AOWSChaosVehicleDemoPawn::HasControlSeatOccupant() const
{
	for (const FOWSVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FOWSVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return true;
			}
		}
	}
	return false;
}

bool AOWSChaosVehicleDemoPawn::IsRecoveryOwner(
	const AActor* OccupantActor) const
{
	if (!IsValid(OccupantActor))
	{
		return false;
	}

	// The controlling occupant is always the sole recovery owner.
	for (const FOWSVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FOWSVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return State.Occupant == OccupantActor;
			}
		}
	}
	if (GetController() != nullptr)
	{
		// An external controller owns the rigid body even if it did not register
		// an actor in OWS's control seat. Passengers cannot recover its vehicle.
		return false;
	}

	// A driverless occupied vehicle still needs exactly one recovery authority.
	// SeatDefinitions supplies deterministic ordering for arbitrary layouts.
	for (const FOWSVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		for (const FOWSVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return State.Occupant == OccupantActor;
			}
		}
	}

	return false;
}

void AOWSChaosVehicleDemoPawn::BeginPlay()
{
	Super::BeginPlay();
	InitializeSeatOccupancy();

	if (GetController() == nullptr)
	{
		SetParkedState(true);
		if (HasAuthority())
		{
			AlignParkedVehicleToGround();
		}
	}
	else
	{
		SetParkedState(false);
	}
}

void AOWSChaosVehicleDemoPawn::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOWSChaosVehicleDemoPawn, SeatOccupancy);
}

void AOWSChaosVehicleDemoPawn::InitializeSeatOccupancy()
{
	if (!HasAuthority())
	{
		return;
	}

	SeatOccupancy.Reset(SeatDefinitions.Num());
	for (const FOWSVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.SeatId.IsNone())
		{
			FOWSVehicleSeatOccupancy& State = SeatOccupancy.AddDefaulted_GetRef();
			State.SeatId = Seat.SeatId;
		}
	}
	ForceNetUpdate();
}

const FOWSVehicleSeatDefinition*
AOWSChaosVehicleDemoPawn::FindSeatDefinition(const FName SeatId) const
{
	return SeatDefinitions.FindByPredicate(
		[SeatId](const FOWSVehicleSeatDefinition& Seat)
		{
			return Seat.SeatId == SeatId;
		});
}

const FOWSVehicleDoorDefinition*
AOWSChaosVehicleDemoPawn::FindDoorDefinition(const FName DoorId) const
{
	return DoorDefinitions.FindByPredicate(
		[DoorId](const FOWSVehicleDoorDefinition& Door)
		{
			return Door.DoorId == DoorId;
		});
}

bool AOWSChaosVehicleDemoPawn::IsSeatAvailable(
	const FName SeatId,
	const AActor* EnteringActor) const
{
	const FOWSVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	if (Seat == nullptr)
	{
		return false;
	}

	const FOWSVehicleSeatOccupancy* State = SeatOccupancy.FindByPredicate(
		[SeatId](const FOWSVehicleSeatOccupancy& Candidate)
		{
			return Candidate.SeatId == SeatId;
		});
	if (State == nullptr || IsValid(State->Occupant))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	// A valid seat must be physically clear regardless of what kind of actor is
	// occupying it: player, NPC, animal, rigid cargo, or a static obstruction.
	// The vehicle and entering actor are explicitly ignored below.
	const FCollisionObjectQueryParams ObjectQuery(
		FCollisionObjectQueryParams::InitType::AllObjects);
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(OWSVehicleSeatClearance),
		false,
		this);
	QueryParams.AddIgnoredActor(this);
	if (EnteringActor != nullptr)
	{
		QueryParams.AddIgnoredActor(EnteringActor);
	}

	const FTransform SeatWorld = Seat->RelativeTransform * GetActorTransform();
	return !World->OverlapAnyTestByObjectType(
		SeatWorld.GetLocation(),
		SeatWorld.GetRotation(),
		ObjectQuery,
		FCollisionShape::MakeBox(Seat->ClearanceExtent),
		QueryParams);
}

void AOWSChaosVehicleDemoPawn::OnRep_SeatOccupancy()
{
	// Future seat UI and enter/exit animation presentation can subscribe here
	// without changing the server-authoritative selection contract.
}

void AOWSChaosVehicleDemoPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyExampleAssets();
	ApplyComparisonMode();
}

void AOWSChaosVehicleDemoPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	SetParkedState(false);
	InstallInputMapping();
}

void AOWSChaosVehicleDemoPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Remote possession changes do not invoke UnPossessed on the owning client.
	// Remove this higher-priority modal mapping as soon as controller replication
	// says the vehicle is no longer locally owned, or it can suppress the
	// character's WASD/Space mappings after exit.
	if (GetController() == nullptr)
	{
		ClearHeldInputs();
		SetParkedState(true);
		RemoveInputMapping();
	}
	else
	{
		SetParkedState(false);
	}
}

void AOWSChaosVehicleDemoPawn::UnPossessed()
{
	ClearHeldInputs();
	SetParkedState(true);
	RemoveInputMapping();
	Super::UnPossessed();
}

void AOWSChaosVehicleDemoPawn::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	LoadInputAssets();

	UEnhancedInputComponent* EnhancedInput =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInput == nullptr)
	{
		return;
	}

	if (LoadedThrottleAction != nullptr)
	{
		EnhancedInput->BindAction(
			LoadedThrottleAction,
			ETriggerEvent::Triggered,
			this,
			&AOWSChaosVehicleDemoPawn::OnThrottleInput);
		EnhancedInput->BindAction(
			LoadedThrottleAction,
			ETriggerEvent::Completed,
			this,
			&AOWSChaosVehicleDemoPawn::OnThrottleReleased);
		EnhancedInput->BindAction(
			LoadedThrottleAction,
			ETriggerEvent::Canceled,
			this,
			&AOWSChaosVehicleDemoPawn::OnThrottleReleased);
	}

	if (LoadedBrakeAction != nullptr)
	{
		EnhancedInput->BindAction(
			LoadedBrakeAction,
			ETriggerEvent::Triggered,
			this,
			&AOWSChaosVehicleDemoPawn::OnBrakeInput);
		EnhancedInput->BindAction(
			LoadedBrakeAction,
			ETriggerEvent::Completed,
			this,
			&AOWSChaosVehicleDemoPawn::OnBrakeReleased);
		EnhancedInput->BindAction(
			LoadedBrakeAction,
			ETriggerEvent::Canceled,
			this,
			&AOWSChaosVehicleDemoPawn::OnBrakeReleased);
	}

	if (LoadedSteeringAction != nullptr)
	{
		EnhancedInput->BindAction(
			LoadedSteeringAction,
			ETriggerEvent::Triggered,
			this,
			&AOWSChaosVehicleDemoPawn::OnSteeringInput);
		EnhancedInput->BindAction(
			LoadedSteeringAction,
			ETriggerEvent::Completed,
			this,
			&AOWSChaosVehicleDemoPawn::OnSteeringReleased);
		EnhancedInput->BindAction(
			LoadedSteeringAction,
			ETriggerEvent::Canceled,
			this,
			&AOWSChaosVehicleDemoPawn::OnSteeringReleased);
	}

	if (LoadedHandbrakeAction != nullptr)
	{
		EnhancedInput->BindAction(
			LoadedHandbrakeAction,
			ETriggerEvent::Triggered,
			this,
			&AOWSChaosVehicleDemoPawn::OnHandbrakeInput);
		EnhancedInput->BindAction(
			LoadedHandbrakeAction,
			ETriggerEvent::Completed,
			this,
			&AOWSChaosVehicleDemoPawn::OnHandbrakeReleased);
		EnhancedInput->BindAction(
			LoadedHandbrakeAction,
			ETriggerEvent::Canceled,
			this,
			&AOWSChaosVehicleDemoPawn::OnHandbrakeReleased);
	}

	if (LoadedLookAction != nullptr)
	{
		EnhancedInput->BindAction(
			LoadedLookAction,
			ETriggerEvent::Triggered,
			this,
			&AOWSChaosVehicleDemoPawn::OnLookInput);
	}

	InstallInputMapping();
}

void AOWSChaosVehicleDemoPawn::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearHeldInputs();
	RemoveInputMapping();
	Super::EndPlay(EndPlayReason);
}

void AOWSChaosVehicleDemoPawn::ApplyComparisonMode()
{
	const bool bEnableCurve =
		ComparisonMode == EOWSChaosVehicleDemoMode::OWSHybrid;

	if (FrontLeftWheel != nullptr)
	{
		FrontLeftWheel->bEnableLateralSlipCurve = bEnableCurve;
	}
	if (FrontRightWheel != nullptr)
	{
		FrontRightWheel->bEnableLateralSlipCurve = bEnableCurve;
	}
	if (BackLeftWheel != nullptr)
	{
		BackLeftWheel->bEnableLateralSlipCurve = bEnableCurve;
	}
	if (BackRightWheel != nullptr)
	{
		BackRightWheel->bEnableLateralSlipCurve = bEnableCurve;
	}
}

void AOWSChaosVehicleDemoPawn::ApplyExampleAssets()
{
	USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	if (VehicleMesh == nullptr)
	{
		return;
	}

	if (USkeletalMesh* MeshAsset = ExampleSkeletalMesh.LoadSynchronous())
	{
		VehicleMesh->SetSkeletalMeshAsset(MeshAsset);
	}

	if (UClass* AnimClass = ExampleAnimClass.LoadSynchronous())
	{
		VehicleMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		VehicleMesh->SetAnimInstanceClass(AnimClass);
	}

	// Reapply body settings after a mesh swap so construction and spawned actors
	// enter the same independent-skeletal-mesh Chaos path.
	VehicleMesh->SetCollisionProfileName(TEXT("OWSVehicle"));
	VehicleMesh->SetEnableGravity(true);
	VehicleMesh->SetSimulatePhysics(true);
}

void AOWSChaosVehicleDemoPawn::ConfigureSuspension(
	UOWSVehicleSimSuspensionComponent* Suspension,
	const FName BoneName,
	const FVector& RelativeLocation)
{
	check(Suspension != nullptr);
	Suspension->SetAssociatedBoneName(BoneName);
	Suspension->SetRelativeLocation(RelativeLocation);
	Suspension->SuspensionAxis = FVector(0.0, 0.0, -1.0);
	Suspension->SuspensionMaxRaise = 10.0f;
	Suspension->SuspensionMaxDrop = 15.0f;
	Suspension->SpringRate = 200.0f;
	Suspension->SpringPreload = 50.0f;
	Suspension->SpringDamping = 0.9f;
	Suspension->SuspensionForceEffect = 100.0f;
}

void AOWSChaosVehicleDemoPawn::ConfigureWheel(
	UOWSVehicleSimCurveWheelComponent* Wheel,
	const FName BoneName,
	const bool bFrontWheel)
{
	check(Wheel != nullptr);
	Wheel->SetAssociatedBoneName(BoneName);
	Wheel->WheelRadius = 40.0f;
	Wheel->WheelWidth = 20.0f;
	Wheel->WheelInertia = 10.0f;
	Wheel->FrictionMultiplier = 1.0f;
	Wheel->CorneringStiffness = 80.0f;
	Wheel->SlipAngleLimit = 8.0f;
	Wheel->MaxBrakeTorque = 2000.0f;
	Wheel->bHandbrakeEnabled = !bFrontWheel;
	Wheel->HandbrakeTorque = bFrontWheel ? 500.0f : 3000.0f;
	Wheel->bSteeringEnabled = bFrontWheel;
	Wheel->MaxSteeringAngle = 35.0f;
	Wheel->bABSEnabled = false;
	Wheel->bTractionControlEnabled = true;
	Wheel->AxisType = EWheelAxisType::X;
	Wheel->ForceOffset = FVector(0.0, 0.0, -20.0);
	Wheel->ReverseDirection = false;
}

void AOWSChaosVehicleDemoPawn::LoadInputAssets()
{
	LoadedInputMapping = ExampleInputMapping.LoadSynchronous();
	LoadedThrottleAction = ThrottleActionAsset.LoadSynchronous();
	LoadedBrakeAction = BrakeActionAsset.LoadSynchronous();
	LoadedSteeringAction = SteeringActionAsset.LoadSynchronous();
	LoadedHandbrakeAction = HandbrakeActionAsset.LoadSynchronous();
	LoadedReverseAction = ReverseActionAsset.LoadSynchronous();
	LoadedLookAction = LookActionAsset.LoadSynchronous();
}

void AOWSChaosVehicleDemoPawn::InstallInputMapping()
{
	LoadInputAssets();
	if (LoadedInputMapping == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	ULocalPlayer* LocalPlayer = PlayerController != nullptr
		? PlayerController->GetLocalPlayer()
		: nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer != nullptr
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
		: nullptr;
	if (InputSubsystem == nullptr)
	{
		return;
	}

	if (bInputMappingInstalled && InstalledInputSubsystem.Get() == InputSubsystem)
	{
		return;
	}

	RemoveInputMapping();
	InputSubsystem->AddMappingContext(LoadedInputMapping, InputMappingPriority);
	InstalledInputSubsystem = InputSubsystem;
	bInputMappingInstalled = true;
}

void AOWSChaosVehicleDemoPawn::RemoveInputMapping()
{
	if (bInputMappingInstalled && InstalledInputSubsystem.IsValid() &&
		LoadedInputMapping != nullptr)
	{
		InstalledInputSubsystem->RemoveMappingContext(LoadedInputMapping);
	}

	InstalledInputSubsystem.Reset();
	bInputMappingInstalled = false;
}

void AOWSChaosVehicleDemoPawn::ClearHeldInputs()
{
	RawThrottleInput = 0.0f;
	BrakeInput = 0.0f;
	SteeringInput = 0.0f;
	bHandbrakeInput = false;
	bReverseInput = false;
	bIgnitionRequested = false;
	bDriveReleased = false;

	if (InputRouter != nullptr)
	{
		InputRouter->ClearThrottleInput();
	}
	if (ThrottleShaper != nullptr)
	{
		ThrottleShaper->ResetThrottleState();
	}

	PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, false);
}

void AOWSChaosVehicleDemoPawn::SetParkedState(const bool bNewParked)
{
	const bool bWasParked = bParked;
	bParked = bNewParked;
	ParkingStableTime = 0.0f;
	bParkingBrakeActive = bParked;
	bDriveReleased = false;

	UModularVehicleBaseComponent* VehicleComponent =
		GetVehicleSimulationComponent();
	USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	if (VehicleComponent == nullptr || VehicleMesh == nullptr)
	{
		return;
	}

	VehicleComponent->bKeepVehicleAwake = false;
	if (bParked)
	{
		bIgnitionRequested = false;
		// Brake all wheels while an arriving vehicle settles. Once it is truly
		// stationary, UpdateParkedState clears every control and lets Chaos sleep.
		PublishVehicleControls(0.0f, 1.0f, 0.0f, true, false, false);
	}
	else
	{
		if (bWasParked)
		{
			bIgnitionRequested = false;
		}
		// Entering drive mode does not start the engine. X/Activate or the
		// first meaningful forward/reverse accelerator input does that.
		PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, bIgnitionRequested);
		VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		VehicleMesh->PutAllRigidBodiesToSleep();
	}
}

void AOWSChaosVehicleDemoPawn::UpdateParkedState(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	if (VehicleMesh == nullptr)
	{
		return;
	}

	// A sleeping parked car may wake from a real collision. Re-engage its
	// service + parking brakes until it settles again instead of letting it roll.
	if (!bParkingBrakeActive && VehicleMesh->IsAnyRigidBodyAwake())
	{
		bParkingBrakeActive = true;
		ParkingStableTime = 0.0f;
	}

	if (!bParkingBrakeActive)
	{
		return;
	}

	PublishVehicleControls(0.0f, 1.0f, 0.0f, true, false, false);

	const float LinearSpeed = VehicleMesh->GetPhysicsLinearVelocity().Size();
	const float AngularSpeed = VehicleMesh->GetPhysicsAngularVelocityInDegrees().Size();
	if (LinearSpeed <= ParkingSleepLinearSpeed &&
		AngularSpeed <= ParkingSleepAngularSpeed)
	{
		ParkingStableTime += DeltaSeconds;
	}
	else
	{
		ParkingStableTime = 0.0f;
	}

	if (ParkingStableTime < ParkingSettleSeconds)
	{
		return;
	}

	// Normal brake inputs count as active controls and would wake Chaos again.
	// Zero them only after the body has stopped, then make sleeping the parked
	// hold. A later collision re-enters the braking path above.
	PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, false);
	VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	VehicleMesh->PutAllRigidBodiesToSleep();
	bParkingBrakeActive = false;
	ParkingStableTime = 0.0f;
}

void AOWSChaosVehicleDemoPawn::PublishVehicleControls(
	const float Throttle,
	const float Brake,
	const float Steering,
	const bool bHandbrake,
	const bool bReverse,
	const bool bIgnition)
{
	using namespace OWS::Vehicle::Demo;
	if (UModularVehicleBaseComponent* VehicleComponent =
		GetVehicleSimulationComponent())
	{
		VehicleComponent->SetInputAxis1D(ThrottleInputName, FMath::Clamp(Throttle, 0.0f, 1.0f));
		VehicleComponent->SetInputAxis1D(BrakeInputName, FMath::Clamp(Brake, 0.0f, 1.0f));
		VehicleComponent->SetInputAxis1D(SteeringInputName, FMath::Clamp(Steering, -1.0f, 1.0f));
		VehicleComponent->SetInputBool(HandbrakeInputName, bHandbrake);
		VehicleComponent->SetInputBool(ReverseInputName, bReverse);
		VehicleComponent->SetInputBool(IgnitionInputName, bIgnition);
	}
}

void AOWSChaosVehicleDemoPawn::AlignParkedVehicleToGround()
{
	USkeletalMeshComponent* VehicleMesh = GetSkeletalMeshComponent();
	UWorld* World = GetWorld();
	if (VehicleMesh == nullptr || World == nullptr)
	{
		return;
	}

	using namespace OWS::Vehicle::Demo;
	const FName WheelBones[] =
	{
		FrontLeftBone,
		FrontRightBone,
		BackLeftBone,
		BackRightBone
	};

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OWSVehicleGroundAlign), false, this);
	QueryParams.AddIgnoredActor(this);
	float AccumulatedOffset = 0.0f;
	int32 ValidWheelTraces = 0;
	for (const FName WheelBone : WheelBones)
	{
		if (VehicleMesh->GetBoneIndex(WheelBone) == INDEX_NONE)
		{
			continue;
		}
		const FVector WheelLocation = VehicleMesh->GetBoneLocation(WheelBone);
		FHitResult GroundHit;
		const FVector TraceStart = WheelLocation + FVector(0.0f, 0.0f, 300.0f);
		const FVector TraceEnd = WheelLocation - FVector(0.0f, 0.0f, 800.0f);
		if (World->LineTraceSingleByChannel(
			GroundHit,
			TraceStart,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams))
		{
			// MaxDrop (15 cm) minus 8 cm target compression leaves a small,
			// repeatable amount of suspension travel in both directions.
			constexpr float WheelRadius = 40.0f;
			constexpr float SuspensionMaxDrop = 15.0f;
			constexpr float TargetCompression = 8.0f;
			const float DesiredWheelZ = GroundHit.ImpactPoint.Z + WheelRadius +
				SuspensionMaxDrop - TargetCompression;
			AccumulatedOffset += DesiredWheelZ - WheelLocation.Z;
			++ValidWheelTraces;
		}
	}

	if (ValidWheelTraces == UE_ARRAY_COUNT(WheelBones))
	{
		const float VerticalOffset = AccumulatedOffset /
			static_cast<float>(ValidWheelTraces);
		AddActorWorldOffset(
			FVector(0.0f, 0.0f, VerticalOffset),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PublishVehicleControls(0.0f, 0.0f, 0.0f, false, false, false);
	VehicleMesh->PutAllRigidBodiesToSleep();
	bParkingBrakeActive = false;
	ParkingStableTime = 0.0f;
}

void AOWSChaosVehicleDemoPawn::OnThrottleInput(
	const FInputActionValue& Value)
{
	RawThrottleInput = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
}

void AOWSChaosVehicleDemoPawn::OnThrottleReleased(
	const FInputActionValue& Value)
{
	RawThrottleInput = 0.0f;
}

void AOWSChaosVehicleDemoPawn::OnBrakeInput(const FInputActionValue& Value)
{
	BrakeInput = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
}

void AOWSChaosVehicleDemoPawn::OnBrakeReleased(
	const FInputActionValue& Value)
{
	BrakeInput = 0.0f;
}

void AOWSChaosVehicleDemoPawn::OnSteeringInput(
	const FInputActionValue& Value)
{
	SteeringInput = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AOWSChaosVehicleDemoPawn::OnSteeringReleased(
	const FInputActionValue& Value)
{
	SteeringInput = 0.0f;
}

void AOWSChaosVehicleDemoPawn::OnHandbrakeInput(
	const FInputActionValue& Value)
{
	bHandbrakeInput = Value.Get<bool>();
}

void AOWSChaosVehicleDemoPawn::OnHandbrakeReleased(
	const FInputActionValue& Value)
{
	bHandbrakeInput = false;
}

void AOWSChaosVehicleDemoPawn::OnReverseInput(
	const FInputActionValue& Value)
{
	bReverseInput = Value.Get<bool>();
}

void AOWSChaosVehicleDemoPawn::OnReverseReleased(
	const FInputActionValue& Value)
{
	bReverseInput = false;
}

void AOWSChaosVehicleDemoPawn::OnLookInput(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}
