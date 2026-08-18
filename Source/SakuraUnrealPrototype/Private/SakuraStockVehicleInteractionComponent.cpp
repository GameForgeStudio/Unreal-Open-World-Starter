#include "SakuraStockVehicleInteractionComponent.h"

#include "CollisionShape.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "VehicleDriveAssemblyComponent.h"

USakuraStockVehicleInteractionComponent::USakuraStockVehicleInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	FSakuraStockVehicleSeatDefinition DriverSeat;
	DriverSeat.SeatId = TEXT("FrontLeft");
	DriverSeat.SeatType = TEXT("Driver");
	DriverSeat.bControlsVehicle = true;
	DriverSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator, FVector(20.0f, -45.0f, 55.0f));
	SeatDefinitions.Add(DriverSeat);

	FSakuraStockVehicleSeatDefinition PassengerSeat;
	PassengerSeat.SeatId = TEXT("FrontRight");
	PassengerSeat.SeatType = TEXT("Passenger");
	PassengerSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator, FVector(20.0f, 45.0f, 55.0f));
	SeatDefinitions.Add(PassengerSeat);

	FSakuraStockVehicleDoorDefinition LeftDoor;
	LeftDoor.DoorId = TEXT("LeftDoor");
	LeftDoor.RelativeTransform = FTransform(
		FRotator(0.0f, -90.0f, 0.0f), FVector(15.0f, -125.0f, 55.0f));
	LeftDoor.OrderedSeatIds = {DriverSeat.SeatId, PassengerSeat.SeatId};
	DoorDefinitions.Add(LeftDoor);

	FSakuraStockVehicleDoorDefinition RightDoor;
	RightDoor.DoorId = TEXT("RightDoor");
	RightDoor.RelativeTransform = FTransform(
		FRotator(0.0f, 90.0f, 0.0f), FVector(15.0f, 125.0f, 55.0f));
	RightDoor.OrderedSeatIds = {PassengerSeat.SeatId, DriverSeat.SeatId};
	DoorDefinitions.Add(RightDoor);
}

void USakuraStockVehicleInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSeatOccupancy();
}

void USakuraStockVehicleInteractionComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USakuraStockVehicleInteractionComponent, SeatOccupancy);
}

APawn* USakuraStockVehicleInteractionComponent::GetVehiclePawn() const
{
	return Cast<APawn>(GetOwner());
}

bool USakuraStockVehicleInteractionComponent::FindFacedDoor(
	const FVector& ActivatorLocation,
	const FVector& ActivatorForward,
	const float MaximumDistance,
	FName& OutDoorId,
	float& OutDistance) const
{
	OutDoorId = NAME_None;
	OutDistance = -1.0f;
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	FVector Facing = ActivatorForward;
	Facing.Z = 0.0f;
	if (!Facing.Normalize())
	{
		return false;
	}

	float BestFacing = MinimumDoorFacingDot;
	float BestDistance = MaximumDistance;
	for (const FSakuraStockVehicleDoorDefinition& Door : DoorDefinitions)
	{
		if (Door.DoorId.IsNone())
		{
			continue;
		}
		const FTransform DoorWorld = Door.RelativeTransform * OwnerActor->GetActorTransform();
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

bool USakuraStockVehicleInteractionComponent::SelectAvailableSeat(
	const FName DoorId,
	const bool bAutoPlacementEnabled,
	const AActor* EnteringActor,
	FName& OutSeatId) const
{
	OutSeatId = NAME_None;
	const FSakuraStockVehicleDoorDefinition* Door = FindDoorDefinition(DoorId);
	const APawn* VehiclePawn = GetVehiclePawn();
	if (Door == nullptr || Door->OrderedSeatIds.IsEmpty() || VehiclePawn == nullptr)
	{
		return false;
	}

	const bool bControlSeatReservedByController = VehiclePawn->GetController() != nullptr;
	if (bAutoPlacementEnabled)
	{
		if (!HasControlSeatOccupant() && !bControlSeatReservedByController)
		{
			for (const FSakuraStockVehicleSeatDefinition& Seat : SeatDefinitions)
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

	const FName RequestedSeatId = Door->OrderedSeatIds[0];
	if ((bControlSeatReservedByController && IsControlSeat(RequestedSeatId)) ||
		!IsSeatAvailable(RequestedSeatId, EnteringActor))
	{
		return false;
	}
	OutSeatId = RequestedSeatId;
	return true;
}

bool USakuraStockVehicleInteractionComponent::OccupySeat(
	const FName SeatId, AActor* OccupantActor)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || !IsValid(OccupantActor) ||
		GetSeatForOccupant(OccupantActor) != NAME_None ||
		!IsSeatAvailable(SeatId, OccupantActor))
	{
		return false;
	}
	for (FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.SeatId == SeatId)
		{
			State.Occupant = OccupantActor;
			GetOwner()->ForceNetUpdate();
			return true;
		}
	}
	return false;
}

bool USakuraStockVehicleInteractionComponent::ReleaseSeat(AActor* OccupantActor)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || !IsValid(OccupantActor))
	{
		return false;
	}
	for (FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.Occupant == OccupantActor)
		{
			State.Occupant = nullptr;
			GetOwner()->ForceNetUpdate();
			return true;
		}
	}
	return true;
}

bool USakuraStockVehicleInteractionComponent::GetSeatWorldTransform(
	const FName SeatId, FTransform& OutTransform) const
{
	const AActor* OwnerActor = GetOwner();
	if (const FSakuraStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
		Seat != nullptr && OwnerActor != nullptr)
	{
		OutTransform = Seat->RelativeTransform * OwnerActor->GetActorTransform();
		return true;
	}
	return false;
}

bool USakuraStockVehicleInteractionComponent::GetDoorWorldTransform(
	const FName DoorId, FTransform& OutTransform) const
{
	const AActor* OwnerActor = GetOwner();
	if (const FSakuraStockVehicleDoorDefinition* Door = FindDoorDefinition(DoorId);
		Door != nullptr && OwnerActor != nullptr)
	{
		OutTransform = Door->RelativeTransform * OwnerActor->GetActorTransform();
		return true;
	}
	return false;
}

bool USakuraStockVehicleInteractionComponent::IsControlSeat(const FName SeatId) const
{
	const FSakuraStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	return Seat != nullptr && Seat->bControlsVehicle;
}

FName USakuraStockVehicleInteractionComponent::GetSeatForOccupant(
	const AActor* OccupantActor) const
{
	if (OccupantActor == nullptr)
	{
		return NAME_None;
	}
	for (const FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.Occupant == OccupantActor)
		{
			return State.SeatId;
		}
	}
	return NAME_None;
}

bool USakuraStockVehicleInteractionComponent::HasControlSeatOccupant() const
{
	for (const FSakuraStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return true;
			}
		}
	}
	return false;
}

bool USakuraStockVehicleInteractionComponent::IsRecoveryOwner(
	const AActor* OccupantActor) const
{
	if (!IsValid(OccupantActor))
	{
		return false;
	}
	for (const FSakuraStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return State.Occupant == OccupantActor;
			}
		}
	}
	if (const APawn* VehiclePawn = GetVehiclePawn();
		VehiclePawn != nullptr && VehiclePawn->GetController() != nullptr)
	{
		return false;
	}
	for (const FSakuraStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		for (const FSakuraStockVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return State.Occupant == OccupantActor;
			}
		}
	}
	return false;
}

UModularVehicleBaseComponent*
USakuraStockVehicleInteractionComponent::GetVehicleSimulation() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<UModularVehicleBaseComponent>()
		: nullptr;
}

UVehicleDriveAssemblyComponent*
USakuraStockVehicleInteractionComponent::GetKinetiForgeDriveAssembly() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<UVehicleDriveAssemblyComponent>()
		: nullptr;
}

UPrimitiveComponent*
USakuraStockVehicleInteractionComponent::GetVehiclePhysicsBody() const
{
	if (USkeletalMeshComponent* SkeletalBody = GetVehicleMesh())
	{
		return SkeletalBody;
	}

	return GetOwner() != nullptr
		? Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent())
		: nullptr;
}

USkeletalMeshComponent*
USakuraStockVehicleInteractionComponent::GetVehicleMesh() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
}

void USakuraStockVehicleInteractionComponent::SetDriverPresent(const bool bPresent)
{
	bDriverPresent = bPresent;
}

#if 0 // Project-side engine, parking, and input mutation intentionally disabled; stock vehicle owns these systems.
void USakuraStockVehicleInteractionComponent::RequestEngineStart()
{
	if (bDriverPresent)
	{
		bEngineStarted = true;
		bDriveReleased = true;
		SetComponentTickEnabled(false);
		const bool bHasStationaryHold = IsValid(ParkingConstraint);
		if (bHasStationaryHold)
		{
			ReleaseParkingLock();
			if (UPrimitiveComponent* VehicleBody = GetVehiclePhysicsBody())
			{
				VehicleBody->WakeAllRigidBodies();
			}
		}
		if (UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly())
		{
			KinetiDrive->StartVehicleEngine();
		}
	}
}

void USakuraStockVehicleInteractionComponent::RequestDriveRelease()
{
	if (!bDriverPresent)
	{
		return;
	}

	bEngineStarted = true;
	if (bDriveReleased)
	{
		ReleaseParkingLock();
		if (UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly())
		{
			KinetiDrive->StartVehicleEngine();
		}
		if (UPrimitiveComponent* VehicleBody = GetVehiclePhysicsBody())
		{
			VehicleBody->WakeAllRigidBodies();
		}
		return;
	}

	bDriveReleased = true;
	if (UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly())
	{
		KinetiDrive->StartVehicleEngine();
	}
	ReleaseParkingLock();
	if (UModularVehicleBaseComponent* VehicleSimulation = GetVehicleSimulation())
	{
		VehicleSimulation->bKeepVehicleAwake = false;
	}
	if (UPrimitiveComponent* VehicleBody = GetVehiclePhysicsBody())
	{
		VehicleBody->WakeAllRigidBodies();
	}
}

void USakuraStockVehicleInteractionComponent::EngageStationaryHold()
{
	if (bDriverPresent && bDriveReleased && !IsValid(ParkingConstraint))
	{
		ApplyParkedState();
	}
}

void USakuraStockVehicleInteractionComponent::ReleaseStationaryHold()
{
	if (!bDriverPresent || !bDriveReleased || !IsValid(ParkingConstraint))
	{
		return;
	}

	const bool bShouldRestartEngine = bEngineStarted;
	ReleaseParkingLock();
	if (bShouldRestartEngine)
	{
		if (UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly())
		{
			KinetiDrive->StartVehicleEngine();
		}
	}
	if (UPrimitiveComponent* VehicleBody = GetVehiclePhysicsBody())
	{
		VehicleBody->WakeAllRigidBodies();
	}
}

void USakuraStockVehicleInteractionComponent::ClearVehicleInputs() const
{
	if (UModularVehicleBaseComponent* VehicleSimulation = GetVehicleSimulation())
	{
		VehicleSimulation->SetInputAxis1D(TEXT("Steering"), 0.0);
		VehicleSimulation->SetInputAxis1D(TEXT("Throttle"), 0.0);
		VehicleSimulation->SetInputAxis1D(TEXT("Brake"), 0.0);
		VehicleSimulation->SetInputBool(TEXT("Handbrake"), false);
		VehicleSimulation->SetInputBool(TEXT("Reverse"), false);
	}
	if (UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly())
	{
		KinetiDrive->InputSteering(0.0f, true);
		KinetiDrive->InputThrottle(0.0f, true);
		KinetiDrive->InputBrake(0.0f, true);
		KinetiDrive->InputHandbrake(0.0f, true);
		KinetiDrive->InputClutch(0.0f, true);
	}
}

void USakuraStockVehicleInteractionComponent::ApplyParkedState()
{
	ClearVehicleInputs();

	UModularVehicleBaseComponent* VehicleSimulation = GetVehicleSimulation();
	UVehicleDriveAssemblyComponent* KinetiDrive = GetKinetiForgeDriveAssembly();
	UPrimitiveComponent* VehicleBody = GetVehiclePhysicsBody();
	AActor* OwnerActor = GetOwner();
	if ((VehicleSimulation == nullptr && KinetiDrive == nullptr) ||
		VehicleBody == nullptr || OwnerActor == nullptr)
	{
		return;
	}

	if (VehicleSimulation != nullptr)
	{
		VehicleSimulation->bKeepVehicleAwake = false;
	}
	// Cars use spring-return pedals. With no driver, propulsion and steering
	// return to zero while both the service brake and parking brake remain
	// applied. The constraint below holds the chassis; these brake inputs also
	// dissipate retained per-wheel angular velocity so the stock wheel animation
	// cannot continue spinning after the driver leaves.
	if (VehicleSimulation != nullptr)
	{
		VehicleSimulation->SetInputAxis1D(TEXT("Steering"), 0.0f);
		VehicleSimulation->SetInputAxis1D(TEXT("Throttle"), 0.0f);
		VehicleSimulation->SetInputAxis1D(TEXT("Brake"), 1.0f);
		VehicleSimulation->SetInputBool(TEXT("Handbrake"), true);
		VehicleSimulation->SetInputBool(TEXT("Reverse"), false);
	}
	if (KinetiDrive != nullptr)
	{
		KinetiDrive->InputSteering(0.0f, true);
		KinetiDrive->InputThrottle(0.0f, true);
		KinetiDrive->InputBrake(1.0f, true);
		KinetiDrive->InputHandbrake(1.0f, true);
		KinetiDrive->ShutVehicleEngine();
	}
	VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector, false, NAME_None);
	VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false, NAME_None);

	if (!IsValid(ParkingConstraint))
	{
		ParkingConstraint = NewObject<UPhysicsConstraintComponent>(
			OwnerActor,
			TEXT("SakuraStockParkingConstraint"));
		OwnerActor->AddInstanceComponent(ParkingConstraint);
		ParkingConstraint->SetupAttachment(VehicleBody);
		ParkingConstraint->SetWorldTransform(VehicleBody->GetComponentTransform());
		ParkingConstraint->SetDisableCollision(true);
		ParkingConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
		ParkingConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
		ParkingConstraint->SetLinearZLimit(LCM_Locked, 0.0f);
		ParkingConstraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
		ParkingConstraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
		ParkingConstraint->SetAngularTwistLimit(ACM_Locked, 0.0f);
		ParkingConstraint->RegisterComponent();
		ParkingConstraint->SetConstrainedComponents(
			VehicleBody,
			VehicleSimulation != nullptr ? ParkingBodyBoneName : NAME_None,
			nullptr,
			NAME_None);
	}

	VehicleBody->PutAllRigidBodiesToSleep();
}

void USakuraStockVehicleInteractionComponent::ReleaseParkingLock()
{
	if (IsValid(ParkingConstraint))
	{
		ParkingConstraint->BreakConstraint();
		ParkingConstraint->DestroyComponent();
		ParkingConstraint = nullptr;
	}
}
#endif

void USakuraStockVehicleInteractionComponent::InitializeSeatOccupancy()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}
	SeatOccupancy.Reset(SeatDefinitions.Num());
	for (const FSakuraStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.SeatId.IsNone())
		{
			FSakuraStockVehicleSeatOccupancy& State = SeatOccupancy.AddDefaulted_GetRef();
			State.SeatId = Seat.SeatId;
		}
	}
	GetOwner()->ForceNetUpdate();
}

const FSakuraStockVehicleSeatDefinition*
USakuraStockVehicleInteractionComponent::FindSeatDefinition(const FName SeatId) const
{
	return SeatDefinitions.FindByPredicate(
		[SeatId](const FSakuraStockVehicleSeatDefinition& Seat)
		{
			return Seat.SeatId == SeatId;
		});
}

const FSakuraStockVehicleDoorDefinition*
USakuraStockVehicleInteractionComponent::FindDoorDefinition(const FName DoorId) const
{
	return DoorDefinitions.FindByPredicate(
		[DoorId](const FSakuraStockVehicleDoorDefinition& Door)
		{
			return Door.DoorId == DoorId;
		});
}

bool USakuraStockVehicleInteractionComponent::IsSeatAvailable(
	const FName SeatId, const AActor* EnteringActor) const
{
	const FSakuraStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	const AActor* OwnerActor = GetOwner();
	if (Seat == nullptr || OwnerActor == nullptr)
	{
		return false;
	}
	const FSakuraStockVehicleSeatOccupancy* State = SeatOccupancy.FindByPredicate(
		[SeatId](const FSakuraStockVehicleSeatOccupancy& Candidate)
		{
			return Candidate.SeatId == SeatId;
		});
	if (State == nullptr || IsValid(State->Occupant) || GetWorld() == nullptr)
	{
		return false;
	}

	const FCollisionObjectQueryParams ObjectQuery(
		FCollisionObjectQueryParams::InitType::AllObjects);
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(SakuraStockVehicleSeatClearance), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);
	if (EnteringActor != nullptr)
	{
		QueryParams.AddIgnoredActor(EnteringActor);
	}
	const FTransform SeatWorld = Seat->RelativeTransform * OwnerActor->GetActorTransform();
	return !GetWorld()->OverlapAnyTestByObjectType(
		SeatWorld.GetLocation(),
		SeatWorld.GetRotation(),
		ObjectQuery,
		FCollisionShape::MakeBox(Seat->ClearanceExtent),
		QueryParams);
}
