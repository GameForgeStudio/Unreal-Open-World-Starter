#include "OWSVehicleInteractionComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "OWSStockVehicleInteractionComponent.h"

namespace
{
	// Stable on-screen message key so the prompt refreshes in place each frame.
	constexpr uint64 GVehiclePromptKey = 715120412ull;
}

UOWSVehicleInteractionComponent::UOWSVehicleInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UOWSVehicleInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UOWSVehicleInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(GVehiclePromptKey);
	}
	Super::EndPlay(EndPlayReason);
}

void UOWSVehicleInteractionComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateInteraction(DeltaTime);
}

void UOWSVehicleInteractionComponent::ShowPrompt(const FString& Text) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			GVehiclePromptKey, 0.2f, FColor(255, 220, 60), Text,
			false, FVector2D(1.6f, 1.6f));
	}
}

bool UOWSVehicleInteractionComponent::DebugEnterNearestVehicle()
{
	const bool bEntered = TryEnterVehicle();
	UE_LOG(LogTemp, Warning, TEXT("[VIC] DebugEnterNearestVehicle -> %s (occupied=%s)"),
		bEntered ? TEXT("ENTERED") : TEXT("FAILED"),
		OccupiedVehicle ? *OccupiedVehicle->GetName() : TEXT("none"));
	return bEntered;
}

bool UOWSVehicleInteractionComponent::DebugExitVehicle()
{
	const bool bExited = TryExitVehicle();
	UE_LOG(LogTemp, Warning, TEXT("[VIC] DebugExitVehicle -> %s"),
		bExited ? TEXT("EXITED") : TEXT("FAILED"));
	return bExited;
}

void UOWSVehicleInteractionComponent::UpdateInteraction(const float DeltaTime)
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}

	// Poll raw key state so Enhanced Input mapping contexts can't swallow it.
	const bool bEnterDown =
		Controller->IsInputKeyDown(EKeys::F) ||
		Controller->IsInputKeyDown(EKeys::Gamepad_FaceButton_Left);
	const bool bExitDown =
		Controller->IsInputKeyDown(EKeys::F) ||
		Controller->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right);

	if (!OccupiedVehicle)
	{
		ExitHoldElapsed = 0.0f;
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());

		// Still tumbling from a bail-out: no entering until back on your feet.
		if (bRagdollActive)
		{
			UpdateRagdollRecovery(DeltaTime);
			ShowPrompt(TEXT("Bailed out!"));
			bInteractKeyWasDown = bEnterDown;
			return;
		}

		// Clear the bail-out lock once the player has walked back out of range,
		// so re-entry requires actually returning to the door.
		if (ReentryBlockedVehicle && Character)
		{
			const float AwayDistance = FVector::Dist(
				Character->GetActorLocation(), ReentryBlockedVehicle->GetActorLocation());
			if (AwayDistance > ReentryReleaseDistance)
			{
				ReentryBlockedVehicle = nullptr;
			}
		}

		bool bCanEnter = false;
		if (Character)
		{
			UOWSStockVehicleInteractionComponent* Interaction = nullptr;
			FName DoorId = NAME_None;
			FName SeatId = NAME_None;
			if (const APawn* Vehicle =
				FindEnterableVehicle(*Character, Interaction, DoorId, SeatId))
			{
				bCanEnter = true;
				ShowPrompt(FString::Printf(
					TEXT("Press  [F]  to enter  %s"), *Vehicle->GetActorNameOrLabel()));
			}
		}
		if (bEnterDown && !bInteractKeyWasDown && bCanEnter)
		{
			TryEnterVehicle();
		}
		bInteractKeyWasDown = bEnterDown;
		return;
	}

	// Keep the view on the vehicle. The OWS controller re-targets the camera on
	// possession changes, so re-assert it if something stole it back.
	if (Controller->GetViewTarget() != OccupiedVehicle)
	{
		SetCameraTarget(OccupiedVehicle, 0.0f);
	}

	// Occupying a vehicle: exit on tap when stopped, hold when moving.
	if (GetVehicleSpeedMph() <= ImmediateExitSpeedMph)
	{
		ExitHoldElapsed = 0.0f;
		ShowPrompt(TEXT("Press  [F]  to exit"));
		if (bExitDown && !bInteractKeyWasDown)
		{
			TryExitVehicle();
		}
	}
	else if (bExitDown)
	{
		ExitHoldElapsed += DeltaTime;
		const float Remaining = FMath::Max(0.0f, MovingExitHoldSeconds - ExitHoldElapsed);
		ShowPrompt(FString::Printf(
			TEXT("Hold  [F]  to bail out...  %.1fs"), Remaining));
		if (ExitHoldElapsed >= MovingExitHoldSeconds)
		{
			ExitHoldElapsed = 0.0f;
			TryExitVehicle();
		}
	}
	else
	{
		ExitHoldElapsed = 0.0f;
		ShowPrompt(TEXT("Hold  [F]  to bail out while moving"));
	}
	bInteractKeyWasDown = bExitDown;
}

bool UOWSVehicleInteractionComponent::TryEnterVehicle()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	if (!Controller || !Character)
	{
		return false;
	}

	UOWSStockVehicleInteractionComponent* Interaction = nullptr;
	FName DoorId = NAME_None;
	FName SeatId = NAME_None;
	APawn* Vehicle = FindEnterableVehicle(*Character, Interaction, DoorId, SeatId);
	if (!Vehicle || !Interaction || !Interaction->IsControlSeat(SeatId) ||
		!Interaction->OccupySeat(SeatId, Character))
	{
		return false;
	}

	FTransform SeatTransform;
	if (!Interaction->GetSeatWorldTransform(SeatId, SeatTransform))
	{
		Interaction->ReleaseSeat(Character);
		return false;
	}

	HomeCharacter = Character;
	OccupiedVehicle = Vehicle;
	OccupiedSeatId = SeatId;
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	Character->SetActorEnableCollision(false);
	Character->SetActorHiddenInGame(true);
	Character->AttachToActor(Vehicle, FAttachmentTransformRules::KeepWorldTransform);
	Character->SetActorTransform(SeatTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Controller->Possess(Vehicle);
	if (Controller->GetPawn() != Vehicle)
	{
		Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Character->SetActorHiddenInGame(false);
		Character->SetActorEnableCollision(true);
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		Interaction->ReleaseSeat(Character);
		HomeCharacter = nullptr;
		OccupiedVehicle = nullptr;
		OccupiedSeatId = NAME_None;
		return false;
	}

	Interaction->SetDriverPresent(true);
	ApplyVehicleInputContext(true);
	// Hand the view to the vehicle so its spring-arm chase camera is used.
	SetCameraTarget(Vehicle, CameraBlendSeconds);
	return true;
}

bool UOWSVehicleInteractionComponent::TryExitVehicle()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	ACharacter* Character = HomeCharacter.Get();
	APawn* Vehicle = OccupiedVehicle.Get();
	UOWSStockVehicleInteractionComponent* Interaction = Vehicle
		? Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>() : nullptr;
	if (!Controller || !Character || !Vehicle || !Interaction ||
		Controller->GetPawn() != Vehicle)
	{
		return false;
	}

	// Snapshot the vehicle's motion before we detach; if it is still rolling the
	// driver bails out into a physics ragdoll instead of stepping to the door.
	const FVector VehicleVelocity = GetVehicleVelocity();
	const bool bMovingFast =
		VehicleVelocity.Size() * 0.0223694f > ImmediateExitSpeedMph;

	// Driver door is on the left; this is the direction the body is thrown.
	FVector DoorDirection = -Vehicle->GetActorRightVector();
	DoorDirection.Z = 0.0f;
	DoorDirection.Normalize();

	Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (bMovingFast)
	{
		// Move the body clear of the door before physics takes over, otherwise
		// the ragdoll starts inside the car body and tangles with it.
		FTransform DoorTransform;
		const FVector DoorLocation =
			Interaction->GetDoorWorldTransform(TEXT("LeftDoor"), DoorTransform)
				? DoorTransform.GetLocation()
				: Vehicle->GetActorLocation();
		Character->SetActorLocation(
			DoorLocation + DoorDirection * BailOutDoorClearance + FVector(0.0f, 0.0f, 40.0f),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (!PlaceCharacterAtDriverDoor(*Character, *Vehicle, *Interaction))
	{
		FTransform SeatTransform;
		if (Interaction->GetSeatWorldTransform(OccupiedSeatId, SeatTransform))
		{
			Character->AttachToActor(Vehicle, FAttachmentTransformRules::KeepWorldTransform);
			Character->SetActorTransform(SeatTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
		return false;
	}

	Character->SetActorHiddenInGame(false);
	Character->SetActorEnableCollision(true);
	Controller->Possess(Character);
	if (Controller->GetPawn() != Character)
	{
		return false;
	}
	Interaction->ReleaseSeat(Character);
	Interaction->SetDriverPresent(false);
	if (bMovingFast)
	{
		EjectCharacterWithPhysics(*Character, VehicleVelocity, DoorDirection);
		// Bailing out must not be instantly undoable: make the player walk back.
		ReentryBlockedVehicle = Vehicle;
	}
	else if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	HomeCharacter = nullptr;
	ApplyVehicleInputContext(false);
	// Cut straight to the character when bailing: blending from a camera that is
	// racing away on the car sweeps the view across the level and reads as a
	// flicker. A stationary car can afford the smooth blend.
	SetCameraTarget(Character, bMovingFast ? 0.0f : CameraBlendSeconds);
	return true;
}

APawn* UOWSVehicleInteractionComponent::FindEnterableVehicle(
	ACharacter& Character,
	UOWSStockVehicleInteractionComponent*& OutInteraction,
	FName& OutDoorId,
	FName& OutSeatId) const
{
	OutInteraction = nullptr;
	OutDoorId = NAME_None;
	OutSeatId = NAME_None;
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<FOverlapResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OWSVehicleEntry), false, &Character);
	World->OverlapMultiByObjectType(
		Hits, Character.GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(MaximumEnterDistance), Params);

	APawn* BestVehicle = nullptr;
	float BestDistance = MaximumEnterDistance;
	for (const FOverlapResult& Hit : Hits)
	{
		APawn* Candidate = Cast<APawn>(Hit.GetActor());
		UOWSStockVehicleInteractionComponent* CandidateInteraction = Candidate
			? Candidate->FindComponentByClass<UOWSStockVehicleInteractionComponent>() : nullptr;
		// Reject a vehicle only if a *player* already controls it. Parked cars
		// spawn with an AIController (AutoPossessAI), and those should still be
		// enterable — possessing the vehicle auto-evicts the AIController.
		AController* CandidateController = Candidate ? Candidate->GetController() : nullptr;
		if (!Candidate || !CandidateInteraction ||
			(CandidateController && CandidateController->IsPlayerController()) ||
			Candidate == ReentryBlockedVehicle)
		{
			continue;
		}
		FName DoorId;
		float Distance = 0.0f;
		const bool bFaced = CandidateInteraction->FindFacedDoor(
			Character.GetActorLocation(), Character.GetActorForwardVector(),
			MaximumEnterDistance, DoorId, Distance);
		if (!bFaced || Distance >= BestDistance)
		{
			continue;
		}
		FName SeatId;
		const bool bSeat = CandidateInteraction->SelectAvailableSeat(DoorId, true, &Character, SeatId);
		if (!bSeat || !CandidateInteraction->IsControlSeat(SeatId))
		{
			continue;
		}
		BestVehicle = Candidate;
		BestDistance = Distance;
		OutInteraction = CandidateInteraction;
		OutDoorId = DoorId;
		OutSeatId = SeatId;
	}
	return BestVehicle;
}

bool UOWSVehicleInteractionComponent::PlaceCharacterAtDriverDoor(
	ACharacter& Character,
	const APawn& Vehicle,
	const UOWSStockVehicleInteractionComponent& Interaction) const
{
	UWorld* World = GetWorld();
	UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	FVector Right = Vehicle.GetActorRightVector();
	Right.Z = 0.0f;
	Right.Normalize();
	FVector Fwd = Vehicle.GetActorForwardVector();
	Fwd.Z = 0.0f;
	Fwd.Normalize();

	// Base points beside each door (left = driver preferred), plus front/back.
	// A little upward bias keeps the capsule test off the ground.
	const FVector Up = FVector(0.0f, 0.0f, 20.0f);
	FTransform DoorT;
	const FVector LeftBase = (Interaction.GetDoorWorldTransform(TEXT("LeftDoor"), DoorT)
		? DoorT.GetLocation() : Vehicle.GetActorLocation()) - Right * DriverDoorExitPadding + Up;
	const FVector RightBase = (Interaction.GetDoorWorldTransform(TEXT("RightDoor"), DoorT)
		? DoorT.GetLocation() : Vehicle.GetActorLocation()) + Right * DriverDoorExitPadding + Up;

	const FVector Candidates[] = {
		LeftBase,
		LeftBase + Fwd * 120.0f,
		LeftBase - Fwd * 120.0f,
		LeftBase - Right * 90.0f,
		RightBase,
		RightBase + Fwd * 120.0f,
		RightBase - Fwd * 120.0f,
		RightBase + Right * 90.0f,
		Vehicle.GetActorLocation() + Fwd * 260.0f + Up,
		Vehicle.GetActorLocation() - Fwd * 260.0f + Up
	};
	const FRotator ExitRotation(0.0f, Vehicle.GetActorRotation().Yaw, 0.0f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OWSDriverDoorExit), false, &Character);
	Params.AddIgnoredActor(&Vehicle);
	for (const FVector& Candidate : Candidates)
	{
		if (!World->OverlapAnyTestByChannel(
			Candidate, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(
				Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()),
			Params))
		{
			Character.SetActorLocationAndRotation(
				Candidate, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
			return true;
		}
	}

	// Everything is blocked (e.g. cars parked bumper to bumper). Don't trap the
	// player in the vehicle — drop them onto the roof as a last resort.
	Character.SetActorLocationAndRotation(
		Vehicle.GetActorLocation() + FVector(0.0f, 0.0f, 160.0f),
		ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
	return true;
}

void UOWSVehicleInteractionComponent::SetCameraTarget(
	AActor* Target,
	const float BlendSeconds) const
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Target)
	{
		return;
	}
	Controller->SetViewTargetWithBlend(
		Target, BlendSeconds, EViewTargetBlendFunction::VTBlend_Cubic);
	// An instant switch usually follows a teleport (seat -> outside the door).
	// Without a cut the gameplay camera rig damps that jump as if it were real
	// motion and swings for a dozen frames, which reads as flicker.
	if (BlendSeconds <= 0.0f)
	{
		if (APlayerCameraManager* Manager = Controller->PlayerCameraManager)
		{
			Manager->SetGameCameraCutThisFrame();
		}
	}
}

void UOWSVehicleInteractionComponent::EjectCharacterWithPhysics(
	ACharacter& Character,
	const FVector& VehicleVelocity,
	const FVector& DoorDirection)
{
	USkeletalMeshComponent* Mesh = Character.GetMesh();
	if (!Mesh)
	{
		return;
	}

	// Remember how the mesh was set up so RecoverFromRagdoll can restore it.
	CachedMeshRelativeTransform = Mesh->GetRelativeTransform();
	CachedMeshCollisionProfile = Mesh->GetCollisionProfileName();
	RagdollCharacter = &Character;
	bRagdollActive = true;
	RagdollElapsed = 0.0f;
	// Level the capsule out. The camera anchors to the actor, so any residual
	// pitch/roll here would tip the whole view while the body tumbles.
	Character.SetActorRotation(
		FRotator(0.0f, Character.GetActorRotation().Yaw, 0.0f), ETeleportType::TeleportPhysics);

	// The capsule and movement component would fight the ragdoll, so shut them
	// down and hand control of the body to the physics asset.
	if (UCharacterMovementComponent* Movement = Character.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = Character.GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// The camera rig hangs off the skeletal mesh, so it would tumble with the
	// ragdoll. Park it on the capsule first, which trails the body smoothly.
	ParkCameraRigsOnCapsule(Character);
	// Free the mesh from the capsule so the body tumbles independently and the
	// capsule can trail it for the camera.
	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	// Don't explode against the car we're bailing out of; still land on the world.
	Mesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	// The tumbling body sits on top of the camera boom's origin. If it blocks the
	// camera probe the arm collapses to minimum length and flickers, so hide the
	// ragdoll from the camera channel and keep the normal walking distance.
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetAllBodiesPhysicsBlendWeight(1.0f);
	Mesh->WakeAllRigidBodies();

	// Carry the car's momentum, shove sideways out of the door, and add an
	// upward kick so the driver clears the vehicle instead of scraping along it.
	const FVector LaunchVelocity =
		VehicleVelocity +
		DoorDirection * EjectionSidePush +
		FVector(0.0f, 0.0f, EjectionUpwardBoost);
	Mesh->SetAllPhysicsLinearVelocity(LaunchVelocity);
	// A little spin so the body tumbles rather than sliding rigidly.
	Mesh->SetAllPhysicsAngularVelocityInDegrees(DoorDirection * 180.0f);
}

void UOWSVehicleInteractionComponent::ParkCameraRigsOnCapsule(ACharacter& Character)
{
	USkeletalMeshComponent* Mesh = Character.GetMesh();
	UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	if (!Mesh || !Capsule)
	{
		return;
	}

	ParkedCameraRigs.Reset();
	ParkedCameraRigTransforms.Reset();
	// Copy the list first: re-attaching mutates the mesh's child array.
	TArray<USceneComponent*> Children = Mesh->GetAttachChildren();
	for (USceneComponent* Child : Children)
	{
		if (!Child || !Child->GetClass()->GetName().Contains(TEXT("Camera")))
		{
			continue;
		}
		const FTransform RelativeToMesh = Child->GetRelativeTransform();
		ParkedCameraRigs.Add(Child);
		ParkedCameraRigTransforms.Add(RelativeToMesh);
		Child->AttachToComponent(
			Capsule, FAttachmentTransformRules::KeepRelativeTransform);
		// Re-create the pose the rig would have if the mesh were still standing
		// in its default place, so framing matches the on-foot camera exactly.
		Child->SetRelativeTransform(RelativeToMesh * CachedMeshRelativeTransform);
	}
}

void UOWSVehicleInteractionComponent::RestoreCameraRigs(ACharacter& Character)
{
	USkeletalMeshComponent* Mesh = Character.GetMesh();
	if (Mesh)
	{
		for (int32 Index = 0; Index < ParkedCameraRigs.Num(); ++Index)
		{
			USceneComponent* Rig = ParkedCameraRigs[Index].Get();
			if (!Rig)
			{
				continue;
			}
			Rig->AttachToComponent(
				Mesh, FAttachmentTransformRules::KeepRelativeTransform);
			Rig->SetRelativeTransform(ParkedCameraRigTransforms[Index]);
		}
	}
	ParkedCameraRigs.Reset();
	ParkedCameraRigTransforms.Reset();
}

void UOWSVehicleInteractionComponent::UpdateRagdollRecovery(const float DeltaTime)
{
	ACharacter* Character = RagdollCharacter.Get();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh)
	{
		bRagdollActive = false;
		return;
	}

	// Trail the collision-free capsule behind the flying body so the camera,
	// which follows the character actor, actually watches the ragdoll. Track the
	// pelvis rather than the component origin (that spins with the root body),
	// and move no faster than a sprint so the camera rig keeps its normal
	// framing instead of lagging and clipping the way it did at car speed.
	const FVector BodyTarget = Mesh->DoesSocketExist(TEXT("pelvis"))
		? Mesh->GetSocketLocation(TEXT("pelvis"))
		: Mesh->GetComponentLocation();
	const FVector AnchorLocation = Character->GetActorLocation();
	const FVector ToBody = BodyTarget - AnchorLocation;
	const float BodyDistance = ToBody.Size();
	const float MaximumStep = RagdollAnchorMaxSpeed * DeltaTime;
	Character->SetActorLocation(
		(BodyDistance > MaximumStep)
			? AnchorLocation + ToBody / BodyDistance * MaximumStep
			: BodyTarget,
		false, nullptr, ETeleportType::None);

	RagdollElapsed += DeltaTime;
	if (RagdollElapsed < RagdollMinimumSeconds)
	{
		return;
	}
	// Wait for the body to stop AND for the camera anchor to have caught up,
	// so standing up never snaps the view across the gap.
	const bool bSettled =
		Mesh->GetPhysicsLinearVelocity().Size() <= RagdollSettleSpeed &&
		BodyDistance <= RagdollAnchorCatchUpDistance;
	if (bSettled || RagdollElapsed >= RagdollMaximumSeconds)
	{
		RecoverFromRagdoll();
	}
}

void UOWSVehicleInteractionComponent::RecoverFromRagdoll()
{
	ACharacter* Character = RagdollCharacter.Get();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Character || !Mesh || !Capsule)
	{
		bRagdollActive = false;
		return;
	}

	// Stand the capsule up where the body came to rest.
	const FVector BodyLocation = Mesh->DoesSocketExist(TEXT("pelvis"))
		? Mesh->GetSocketLocation(TEXT("pelvis"))
		: Mesh->GetComponentLocation();
	Mesh->SetSimulatePhysics(false);
	Mesh->SetAllBodiesSimulatePhysics(false);
	Mesh->SetCollisionProfileName(CachedMeshCollisionProfile);
	Mesh->AttachToComponent(Capsule, FAttachmentTransformRules::KeepRelativeTransform);
	Mesh->SetRelativeTransform(CachedMeshRelativeTransform);
	RestoreCameraRigs(*Character);

	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Character->SetActorLocation(
		BodyLocation + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight()),
		false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	bRagdollActive = false;
	RagdollElapsed = 0.0f;
	RagdollCharacter = nullptr;
}

FVector UOWSVehicleInteractionComponent::GetVehicleVelocity() const
{
	const APawn* Vehicle = OccupiedVehicle.Get();
	const UPrimitiveComponent* Body = Vehicle
		? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()) : nullptr;
	if (Body && Body->IsSimulatingPhysics())
	{
		return Body->GetPhysicsLinearVelocity();
	}
	return Vehicle ? Vehicle->GetVelocity() : FVector::ZeroVector;
}

float UOWSVehicleInteractionComponent::GetVehicleSpeedMph() const
{
	return GetVehicleVelocity().Size() * 0.0223694f;
}

void UOWSVehicleInteractionComponent::ApplyVehicleInputContext(const bool bVehicleActive) const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	UInputMappingContext* VehicleContext = LoadObject<UInputMappingContext>(
		nullptr, TEXT("/KinetiForge/Template/Input/IMC_VehicleDefault.IMC_VehicleDefault"));
	if (!InputSubsystem || !VehicleContext)
	{
		return;
	}
	InputSubsystem->RemoveMappingContext(VehicleContext);
	if (bVehicleActive)
	{
		InputSubsystem->AddMappingContext(VehicleContext, 200);
	}
}
