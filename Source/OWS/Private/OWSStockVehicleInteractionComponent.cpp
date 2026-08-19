#include "OWSStockVehicleInteractionComponent.h"

#include "CollisionShape.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "VehicleDriveAssemblyComponent.h"

UOWSStockVehicleInteractionComponent::UOWSStockVehicleInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	FOWSStockVehicleSeatDefinition DriverSeat;
	DriverSeat.SeatId = TEXT("FrontLeft");
	DriverSeat.SeatType = TEXT("Driver");
	DriverSeat.bControlsVehicle = true;
	DriverSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator, FVector(20.0f, -45.0f, 55.0f));
	SeatDefinitions.Add(DriverSeat);

	FOWSStockVehicleSeatDefinition PassengerSeat;
	PassengerSeat.SeatId = TEXT("FrontRight");
	PassengerSeat.SeatType = TEXT("Passenger");
	PassengerSeat.RelativeTransform = FTransform(
		FRotator::ZeroRotator, FVector(20.0f, 45.0f, 55.0f));
	SeatDefinitions.Add(PassengerSeat);

	FOWSStockVehicleDoorDefinition LeftDoor;
	LeftDoor.DoorId = TEXT("LeftDoor");
	LeftDoor.RelativeTransform = FTransform(
		FRotator(0.0f, -90.0f, 0.0f), FVector(15.0f, -125.0f, 55.0f));
	LeftDoor.OrderedSeatIds = {DriverSeat.SeatId, PassengerSeat.SeatId};
	DoorDefinitions.Add(LeftDoor);

	FOWSStockVehicleDoorDefinition RightDoor;
	RightDoor.DoorId = TEXT("RightDoor");
	RightDoor.RelativeTransform = FTransform(
		FRotator(0.0f, 90.0f, 0.0f), FVector(15.0f, 125.0f, 55.0f));
	RightDoor.OrderedSeatIds = {PassengerSeat.SeatId, DriverSeat.SeatId};
	DoorDefinitions.Add(RightDoor);
}

void UOWSStockVehicleInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSeatOccupancy();
	SetDriverPresent(false);
}

void UOWSStockVehicleInteractionComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UOWSStockVehicleInteractionComponent, SeatOccupancy);
}

APawn* UOWSStockVehicleInteractionComponent::GetVehiclePawn() const
{
	return Cast<APawn>(GetOwner());
}

bool UOWSStockVehicleInteractionComponent::FindFacedDoor(
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
	for (const FOWSStockVehicleDoorDefinition& Door : DoorDefinitions)
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

bool UOWSStockVehicleInteractionComponent::SelectAvailableSeat(
	const FName DoorId,
	const bool bAutoPlacementEnabled,
	const AActor* EnteringActor,
	FName& OutSeatId) const
{
	OutSeatId = NAME_None;
	const FOWSStockVehicleDoorDefinition* Door = FindDoorDefinition(DoorId);
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
			for (const FOWSStockVehicleSeatDefinition& Seat : SeatDefinitions)
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

bool UOWSStockVehicleInteractionComponent::OccupySeat(
	const FName SeatId, AActor* OccupantActor)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || !IsValid(OccupantActor) ||
		GetSeatForOccupant(OccupantActor) != NAME_None ||
		!IsSeatAvailable(SeatId, OccupantActor))
	{
		return false;
	}
	for (FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
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

bool UOWSStockVehicleInteractionComponent::ReleaseSeat(AActor* OccupantActor)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || !IsValid(OccupantActor))
	{
		return false;
	}
	for (FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
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

bool UOWSStockVehicleInteractionComponent::GetSeatWorldTransform(
	const FName SeatId, FTransform& OutTransform) const
{
	const AActor* OwnerActor = GetOwner();
	if (const FOWSStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
		Seat != nullptr && OwnerActor != nullptr)
	{
		OutTransform = Seat->RelativeTransform * OwnerActor->GetActorTransform();
		return true;
	}
	return false;
}

bool UOWSStockVehicleInteractionComponent::GetDoorWorldTransform(
	const FName DoorId, FTransform& OutTransform) const
{
	const AActor* OwnerActor = GetOwner();
	if (const FOWSStockVehicleDoorDefinition* Door = FindDoorDefinition(DoorId);
		Door != nullptr && OwnerActor != nullptr)
	{
		OutTransform = Door->RelativeTransform * OwnerActor->GetActorTransform();
		return true;
	}
	return false;
}

bool UOWSStockVehicleInteractionComponent::IsControlSeat(const FName SeatId) const
{
	const FOWSStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	return Seat != nullptr && Seat->bControlsVehicle;
}

FName UOWSStockVehicleInteractionComponent::GetSeatForOccupant(
	const AActor* OccupantActor) const
{
	if (OccupantActor == nullptr)
	{
		return NAME_None;
	}
	for (const FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
	{
		if (State.Occupant == OccupantActor)
		{
			return State.SeatId;
		}
	}
	return NAME_None;
}

bool UOWSStockVehicleInteractionComponent::HasControlSeatOccupant() const
{
	for (const FOWSStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
		{
			if (State.SeatId == Seat.SeatId && IsValid(State.Occupant))
			{
				return true;
			}
		}
	}
	return false;
}

bool UOWSStockVehicleInteractionComponent::IsRecoveryOwner(
	const AActor* OccupantActor) const
{
	if (!IsValid(OccupantActor))
	{
		return false;
	}
	for (const FOWSStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.bControlsVehicle)
		{
			continue;
		}
		for (const FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
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
	for (const FOWSStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		for (const FOWSStockVehicleSeatOccupancy& State : SeatOccupancy)
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
UOWSStockVehicleInteractionComponent::GetVehicleSimulation() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<UModularVehicleBaseComponent>()
		: nullptr;
}

UVehicleDriveAssemblyComponent*
UOWSStockVehicleInteractionComponent::GetKinetiForgeDriveAssembly() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<UVehicleDriveAssemblyComponent>()
		: nullptr;
}

UPrimitiveComponent*
UOWSStockVehicleInteractionComponent::GetVehiclePhysicsBody() const
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
UOWSStockVehicleInteractionComponent::GetVehicleMesh() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
}

void UOWSStockVehicleInteractionComponent::SetDriverPresent(const bool bPresent)
{
	bDriverPresent = bPresent;
	if (UVehicleDriveAssemblyComponent* Drive = GetKinetiForgeDriveAssembly())
	{
		Drive->InputBrake(bPresent ? 0.0f : 1.0f, true);
		Drive->InputHandbrake(bPresent ? 0.0f : 1.0f, true);
	}
	if (!bPresent)
	{
		if (UPrimitiveComponent* Body = GetVehiclePhysicsBody())
		{
			Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Body->PutAllRigidBodiesToSleep();
		}
	}
}

#if 0 // Project-side engine, parking, and input mutation intentionally disabled; stock vehicle owns these systems.
void UOWSStockVehicleInteractionComponent::RequestEngineStart()
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

void UOWSStockVehicleInteractionComponent::RequestDriveRelease()
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

void UOWSStockVehicleInteractionComponent::EngageStationaryHold()
{
	if (bDriverPresent && bDriveReleased && !IsValid(ParkingConstraint))
	{
		ApplyParkedState();
	}
}

void UOWSStockVehicleInteractionComponent::ReleaseStationaryHold()
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

void UOWSStockVehicleInteractionComponent::ClearVehicleInputs() const
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

void UOWSStockVehicleInteractionComponent::ApplyParkedState()
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
			TEXT("OWSStockParkingConstraint"));
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

void UOWSStockVehicleInteractionComponent::ReleaseParkingLock()
{
	if (IsValid(ParkingConstraint))
	{
		ParkingConstraint->BreakConstraint();
		ParkingConstraint->DestroyComponent();
		ParkingConstraint = nullptr;
	}
}
#endif

void UOWSStockVehicleInteractionComponent::InitializeSeatOccupancy()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}
	SeatOccupancy.Reset(SeatDefinitions.Num());
	for (const FOWSStockVehicleSeatDefinition& Seat : SeatDefinitions)
	{
		if (!Seat.SeatId.IsNone())
		{
			FOWSStockVehicleSeatOccupancy& State = SeatOccupancy.AddDefaulted_GetRef();
			State.SeatId = Seat.SeatId;
		}
	}
	GetOwner()->ForceNetUpdate();
}

const FOWSStockVehicleSeatDefinition*
UOWSStockVehicleInteractionComponent::FindSeatDefinition(const FName SeatId) const
{
	return SeatDefinitions.FindByPredicate(
		[SeatId](const FOWSStockVehicleSeatDefinition& Seat)
		{
			return Seat.SeatId == SeatId;
		});
}

const FOWSStockVehicleDoorDefinition*
UOWSStockVehicleInteractionComponent::FindDoorDefinition(const FName DoorId) const
{
	return DoorDefinitions.FindByPredicate(
		[DoorId](const FOWSStockVehicleDoorDefinition& Door)
		{
			return Door.DoorId == DoorId;
		});
}

bool UOWSStockVehicleInteractionComponent::IsSeatAvailable(
	const FName SeatId, const AActor* EnteringActor) const
{
	const FOWSStockVehicleSeatDefinition* Seat = FindSeatDefinition(SeatId);
	const AActor* OwnerActor = GetOwner();
	if (Seat == nullptr || OwnerActor == nullptr)
	{
		return false;
	}
	const FOWSStockVehicleSeatOccupancy* State = SeatOccupancy.FindByPredicate(
		[SeatId](const FOWSStockVehicleSeatOccupancy& Candidate)
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
		SCENE_QUERY_STAT(OWSStockVehicleSeatClearance), false, OwnerActor);
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
