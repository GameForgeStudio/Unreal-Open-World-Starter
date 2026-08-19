#include "OWSTestLabPlayerController.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "OWSPrototypeCharacter.h"
#include "OWSStockVehicleInteractionComponent.h"
#include "OWSUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSTestLabPlayerController)

namespace OWS::TestLab::PlayerController
{
	const FString StandardModePath(
		TEXT("/Script/OWS.OWSTestLabGameMode"));
	const FString MoverModePath(
		TEXT("/Script/OWS.OWSTestLabMoverGameMode"));

}

AOWSTestLabPlayerController::AOWSTestLabPlayerController()
{
	bShowMouseCursor = false;
}

void AOWSTestLabPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AOWSTestLabPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!HasAuthority() || IsInMoverMode())
	{
		return;
	}

	if (!OccupiedSeatId.IsNone() &&
		(!IsValid(OccupiedVehicle) || OccupiedVehicle->IsActorBeingDestroyed()))
	{
		RestoreAfterVehicleLossAuthority();
	}

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}

	// A shared vehicle has exactly one recovery owner. Passengers must never
	// record or teleport the same rigid body from independent controller state.
	if (APawn* AssociatedVehicle = GetCurrentLabVehicle())
	{
		const AOWSPrototypeCharacter* Occupant = GetHomeCharacterForHUD();
		const UOWSStockVehicleInteractionComponent* Interaction =
			AssociatedVehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
		if (Interaction == nullptr || !Interaction->IsRecoveryOwner(Occupant))
		{
			return;
		}
	}

	const float PawnZ = ControlledPawn->GetActorLocation().Z;
	if (PawnZ < HardRecoveryZ)
	{
		RecoverFallenPawn();
		NoGroundRecoveryElapsed = 0.0f;
		return;
	}

	// The proving ground intentionally contains valleys, banked roads, ramps,
	// bridges, and underpasses. A low Z value alone is therefore not a fall.
	// Only begin the delayed recovery after leaving the terrain envelope while
	// moving downward and finding no WorldStatic surface below the pawn.
	bool bFallingWithoutGround = false;
	if (PawnZ < NoGroundRecoveryActivationZ && ControlledPawn->GetVelocity().Z < -50.0f)
	{
		if (UWorld* World = GetWorld())
		{
			FCollisionQueryParams QueryParams(
				SCENE_QUERY_STAT(OWSNoGroundRecovery),
				false,
				ControlledPawn);
			QueryParams.AddIgnoredActor(ControlledPawn);
			FHitResult GroundHit;
			const FVector Start = ControlledPawn->GetActorLocation();
			const FVector End = Start - FVector(0.0f, 0.0f, NoGroundTraceDistance);
			bFallingWithoutGround = !World->LineTraceSingleByChannel(
				GroundHit,
				Start,
				End,
				ECC_WorldStatic,
				QueryParams);
		}
	}

	NoGroundRecoveryElapsed = bFallingWithoutGround
		? NoGroundRecoveryElapsed + DeltaTime
		: 0.0f;
	if (NoGroundRecoveryElapsed >= NoGroundRecoveryDelay)
	{
		RecoverFallenPawn();
		NoGroundRecoveryElapsed = 0.0f;
		return;
	}

	UpdateRecoveryCheckpoint(DeltaTime);
}

bool AOWSTestLabPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (Params.Key == EKeys::Gamepad_FaceButton_Right &&
		Params.Event == IE_Pressed && IsLocalController())
	{
		if (GetCurrentLabVehicle() != nullptr)
		{
			RequestToggleVehicle();
			return true;
		}
	}

	return Super::InputKey(Params);
}

void AOWSTestLabPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	InputComponent->BindKey(
		EKeys::F,
		IE_Pressed,
		this,
		&ThisClass::RequestToggleVehicle);
	InputComponent->BindKey(
		EKeys::Gamepad_FaceButton_Left,
		IE_Pressed,
		this,
		&ThisClass::RequestActivate);
	InputComponent->BindKey(
		EKeys::M,
		IE_Pressed,
		this,
		&ThisClass::RequestToggleMoverMode);
}


void AOWSTestLabPlayerController::RequestActivate()
{
	if (APawn* Vehicle = GetCurrentLabVehicle())
	{
		return;
	}

	RequestToggleVehicle();
}

void AOWSTestLabPlayerController::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AOWSTestLabPlayerController, HomeCharacter, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AOWSTestLabPlayerController, OccupiedVehicle, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AOWSTestLabPlayerController, OccupiedSeatId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AOWSTestLabPlayerController, EnteredDoorId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(
		AOWSTestLabPlayerController,
		VehicleInteractionStatus,
		COND_OwnerOnly);
}

AOWSPrototypeCharacter*
AOWSTestLabPlayerController::GetHomeCharacterForHUD() const
{
	if (AOWSPrototypeCharacter* PossessedCharacter =
		Cast<AOWSPrototypeCharacter>(GetPawn()))
	{
		return PossessedCharacter;
	}

	return IsValid(HomeCharacter) && HomeCharacter->GetWorld() == GetWorld()
		? HomeCharacter.Get()
		: nullptr;
}

APawn*
AOWSTestLabPlayerController::GetCurrentLabVehicle() const
{
	if (IsValid(OccupiedVehicle) && OccupiedVehicle->GetWorld() == GetWorld())
	{
		return OccupiedVehicle.Get();
	}
	APawn* ControlledPawn = GetPawn();
	return ControlledPawn != nullptr &&
		ControlledPawn->FindComponentByClass<UOWSStockVehicleInteractionComponent>() != nullptr
		? ControlledPawn
		: nullptr;
}

bool AOWSTestLabPlayerController::IsCurrentVehicleDriver() const
{
	const APawn* Vehicle = GetCurrentLabVehicle();
	const UOWSStockVehicleInteractionComponent* Interaction = Vehicle != nullptr
		? Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>()
		: nullptr;
	return Vehicle != nullptr && !OccupiedSeatId.IsNone() &&
		Interaction != nullptr && Interaction->IsControlSeat(OccupiedSeatId) &&
		GetPawn() == Vehicle;
}

bool AOWSTestLabPlayerController::IsInMoverMode() const
{
	using namespace OWS::TestLab::PlayerController;

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	UClass* GameModeClass = nullptr;
	if (const AGameModeBase* AuthorityGameMode = World->GetAuthGameMode())
	{
		GameModeClass = AuthorityGameMode->GetClass();
	}
	else if (const AGameStateBase* GameState = World->GetGameState())
	{
		GameModeClass = GameState->GameModeClass.Get();
	}

	return GameModeClass != nullptr &&
		GameModeClass->GetPathName().Equals(MoverModePath, ESearchCase::CaseSensitive);
}

float AOWSTestLabPlayerController::GetNearestEnterableVehicleDistance() const
{
	float Distance = -1.0f;
	FindNearestEnterableVehicle(
		IsVehicleAutoPlacementEnabled(),
		&Distance,
		nullptr,
		nullptr);
	return Distance;
}

void AOWSTestLabPlayerController::RequestToggleVehicle()
{
	const bool bAutoPlacementEnabled = IsVehicleAutoPlacementEnabled();
	if (HasAuthority())
	{
		ToggleVehicleAuthority(bAutoPlacementEnabled);
	}
	else
	{
		ServerToggleVehicle(bAutoPlacementEnabled);
	}
}

void AOWSTestLabPlayerController::RequestToggleMoverMode()
{
	if (HasAuthority())
	{
		ToggleMoverModeAuthority();
	}
	else
	{
		ServerToggleMoverMode();
	}
}

void AOWSTestLabPlayerController::ServerToggleVehicle_Implementation(
	const bool bAutoPlacementEnabled)
{
	ToggleVehicleAuthority(bAutoPlacementEnabled);
}

void AOWSTestLabPlayerController::ServerToggleMoverMode_Implementation()
{
	ToggleMoverModeAuthority();
}

void AOWSTestLabPlayerController::ToggleVehicleAuthority(
	const bool bAutoPlacementEnabled)
{
	if (!HasAuthority() || IsInMoverMode())
	{
		return;
	}

	if (APawn* CurrentVehicle = GetCurrentLabVehicle())
	{
		UOWSStockVehicleInteractionComponent* Interaction =
			CurrentVehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
		if (Interaction == nullptr)
		{
			return;
		}
		bool bExited = false;
		if (IsCurrentVehicleDriver())
		{
			bExited = ExitVehicleAuthority(*CurrentVehicle, *Interaction);
		}
		else
		{
			bExited = ExitPassengerSeatAuthority(*CurrentVehicle, *Interaction);
		}
		VehicleInteractionStatus = bExited
			? TEXT("Exited the vehicle through a clear nearby position.")
			: TEXT("Exit rejected: no safe unblocked position is available.");
		ForceNetUpdate();
		return;
	}

	AOWSPrototypeCharacter* PrototypeCharacter =
		Cast<AOWSPrototypeCharacter>(GetPawn());
	if (PrototypeCharacter == nullptr || PrototypeCharacter->GetController() != this)
	{
		VehicleInteractionStatus = TEXT("Activate rejected: no controllable on-foot character.");
		ForceNetUpdate();
		return;
	}

	FName DoorId = NAME_None;
	FName SeatId = NAME_None;
	APawn* Vehicle = FindNearestEnterableVehicle(
		bAutoPlacementEnabled,
		nullptr,
		&DoorId,
		&SeatId);
	if (Vehicle == nullptr)
	{
		VehicleInteractionStatus = bAutoPlacementEnabled
			? TEXT("Entry rejected: face an in-range door with an available seat.")
			: TEXT("AUTO PLACEMENT OFF: face an in-range door; its primary seat must be free and clear.");
		ForceNetUpdate();
		return;
	}
	UOWSStockVehicleInteractionComponent* Interaction =
		Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
	if (Interaction == nullptr)
	{
		return;
	}

	const bool bEntered = EnterVehicleAuthority(
		*PrototypeCharacter,
		*Vehicle,
		*Interaction,
		DoorId,
		SeatId);
	if (bEntered)
	{
		if (Interaction->IsControlSeat(SeatId))
		{
			VehicleInteractionStatus = TEXT(
				"Driver seat entered: Epic stock Chaos Modular Vehicle controls are active.");
		}
		else
		{
			VehicleInteractionStatus = FString::Printf(
				TEXT("Passenger seat '%s' entered."),
				*SeatId.ToString());
		}
	}
	else
	{
		VehicleInteractionStatus =
			TEXT("Entry rejected: the selected door or seat became unavailable.");
	}
	ForceNetUpdate();
}

void AOWSTestLabPlayerController::ToggleMoverModeAuthority()
{
	using namespace OWS::TestLab::PlayerController;

	if (!HasAuthority())
	{
		return;
	}

	// A mode switch changes the world for everyone. Until the engine has an
	// explicit admin/host permission model, only the local server controller may
	// request this lab-only travel; remote clients cannot force global travel.
	if (GetNetMode() != NM_Standalone && !IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || !World->NextURL.IsEmpty() || World->IsInSeamlessTravel())
	{
		return;
	}

	FString MapPackageName = UWorld::RemovePIEPrefix(
		World->GetOutermost()->GetName());
	if (MapPackageName.IsEmpty())
	{
		return;
	}

	const FString& TargetMode = IsInMoverMode()
		? StandardModePath
		: MoverModePath;
	const FString TravelURL = FString::Printf(
		TEXT("%s?game=%s"),
		*MapPackageName,
		*TargetMode);

	World->ServerTravel(TravelURL, true);
}

APawn*
AOWSTestLabPlayerController::FindNearestEnterableVehicle(
	const bool bAutoPlacementEnabled,
	float* OutDistance,
	FName* OutDoorId,
	FName* OutSeatId) const
{
	if (OutDistance != nullptr)
	{
		*OutDistance = -1.0f;
	}
	if (OutDoorId != nullptr)
	{
		*OutDoorId = NAME_None;
	}
	if (OutSeatId != nullptr)
	{
		*OutSeatId = NAME_None;
	}

	if (IsInMoverMode() || IsValid(OccupiedVehicle) ||
		(GetPawn() != nullptr &&
			GetPawn()->FindComponentByClass<UOWSStockVehicleInteractionComponent>() != nullptr))
	{
		return nullptr;
	}

	const AOWSPrototypeCharacter* PrototypeCharacter =
		Cast<AOWSPrototypeCharacter>(GetPawn());
	UWorld* World = GetWorld();
	if (PrototypeCharacter == nullptr || World == nullptr)
	{
		return nullptr;
	}

	const FVector CharacterLocation = PrototypeCharacter->GetActorLocation();
	const FVector CharacterForward = PrototypeCharacter->GetActorForwardVector();
	double BestDistance = static_cast<double>(MaximumVehicleEnterDistance);
	APawn* BestVehicle = nullptr;
	UOWSStockVehicleInteractionComponent* BestInteraction = nullptr;
	FName BestDoorId = NAME_None;

	// Broadphase only nearby dynamic collision instead of scanning every loaded
	// vehicle in an 8 km World Partition world every time the HUD asks.
	TArray<FOverlapResult> NearbyOverlaps;
	FCollisionQueryParams NearbyQuery(
		SCENE_QUERY_STAT(OWSNearbyVehicleBroadphase),
		false,
		PrototypeCharacter);
	NearbyQuery.AddIgnoredActor(PrototypeCharacter);
	World->OverlapMultiByObjectType(
		NearbyOverlaps,
		CharacterLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(MaximumVehicleEnterDistance),
		NearbyQuery);

	TSet<APawn*> NearbyVehicles;
	for (const FOverlapResult& Overlap : NearbyOverlaps)
	{
		if (APawn* Candidate = Cast<APawn>(Overlap.GetActor());
			Candidate != nullptr &&
			Candidate->FindComponentByClass<UOWSStockVehicleInteractionComponent>() != nullptr)
		{
			NearbyVehicles.Add(Candidate);
		}
	}

	for (APawn* Candidate : NearbyVehicles)
	{
		if (!IsValid(Candidate) || Candidate->IsActorBeingDestroyed())
		{
			continue;
		}

		UOWSStockVehicleInteractionComponent* CandidateInteraction =
			Candidate->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
		if (CandidateInteraction == nullptr)
		{
			continue;
		}

		FName CandidateDoorId = NAME_None;
		float CandidateDistance = -1.0f;
		if (!CandidateInteraction->FindFacedDoor(
			CharacterLocation,
			CharacterForward,
			MaximumVehicleEnterDistance,
			CandidateDoorId,
			CandidateDistance))
		{
			continue;
		}

		FTransform DoorWorldTransform;
		if (!CandidateInteraction->GetDoorWorldTransform(
			CandidateDoorId, DoorWorldTransform))
		{
			continue;
		}

		// Facing and range must describe a physically reachable door, not one
		// selected through a wall or another opaque obstruction.
		FCollisionQueryParams VisibilityQuery(
			SCENE_QUERY_STAT(OWSVehicleDoorVisibility),
			false,
			PrototypeCharacter);
		VisibilityQuery.AddIgnoredActor(PrototypeCharacter);
		VisibilityQuery.AddIgnoredActor(Candidate);
		const FVector VisibilityStart =
			CharacterLocation + FVector(0.0f, 0.0f, 50.0f);
		if (World->LineTraceTestByChannel(
			VisibilityStart,
			DoorWorldTransform.GetLocation(),
			ECC_Visibility,
			VisibilityQuery))
		{
			continue;
		}

		const bool bCloser = CandidateDistance < BestDistance - KINDA_SMALL_NUMBER;
		const bool bStableTieBreak = FMath::IsNearlyEqual(
			CandidateDistance,
			static_cast<float>(BestDistance)) &&
			(BestVehicle == nullptr || Candidate->GetPathName().Compare(
				BestVehicle->GetPathName(),
				ESearchCase::CaseSensitive) < 0);
		if (bCloser || bStableTieBreak)
		{
			BestDistance = CandidateDistance;
			BestVehicle = Candidate;
			BestInteraction = CandidateInteraction;
			BestDoorId = CandidateDoorId;
		}
	}

	if (BestVehicle != nullptr && BestInteraction != nullptr)
	{
		FName BestSeatId = NAME_None;
		if (!BestInteraction->SelectAvailableSeat(
			BestDoorId,
			bAutoPlacementEnabled,
			PrototypeCharacter,
			BestSeatId))
		{
			// The faced vehicle remains the activation target. A blocked/full
			// target must not silently redirect this request into a nearby car.
			return nullptr;
		}

		if (OutDistance != nullptr)
		{
			*OutDistance = static_cast<float>(BestDistance);
		}
		if (OutDoorId != nullptr)
		{
			*OutDoorId = BestDoorId;
		}
		if (OutSeatId != nullptr)
		{
			*OutSeatId = BestSeatId;
		}
	}

	return BestVehicle;
}

bool AOWSTestLabPlayerController::EnterVehicleAuthority(
	AOWSPrototypeCharacter& PrototypeCharacter,
	APawn& Vehicle,
	UOWSStockVehicleInteractionComponent& Interaction,
	const FName DoorId,
	const FName SeatId)
{
	if (!HasAuthority() || GetPawn() != &PrototypeCharacter ||
		PrototypeCharacter.GetController() != this || Vehicle.IsActorBeingDestroyed() ||
		DoorId.IsNone() || SeatId.IsNone())
	{
		return false;
	}

	FName FacedDoorId = NAME_None;
	float DoorDistance = -1.0f;
	if (!Interaction.FindFacedDoor(
		PrototypeCharacter.GetActorLocation(),
		PrototypeCharacter.GetActorForwardVector(),
		MaximumVehicleEnterDistance,
		FacedDoorId,
		DoorDistance) || FacedDoorId != DoorId)
	{
		return false;
	}

	const bool bControlSeat = Interaction.IsControlSeat(SeatId);
	if ((bControlSeat && Vehicle.GetController() != nullptr) ||
		!Interaction.OccupySeat(SeatId, &PrototypeCharacter))
	{
		return false;
	}

	HomeCharacter = &PrototypeCharacter;
	OccupiedVehicle = &Vehicle;
	OccupiedSeatId = SeatId;
	EnteredDoorId = DoorId;
	LastSafeCharacterTransform = PrototypeCharacter.GetActorTransform();
	LastSafeCharacterTransform.SetRotation(
		FRotator(0.0f, PrototypeCharacter.GetActorRotation().Yaw, 0.0f).Quaternion());
	bHasSafeCharacterTransform = true;
	ForceNetUpdate();

	if (!bControlSeat)
	{
		if (EnterPassengerSeatAuthority(
			PrototypeCharacter,
			Vehicle,
			Interaction,
			DoorId,
			SeatId))
		{
			return true;
		}

		Interaction.ReleaseSeat(&PrototypeCharacter);
		OccupiedVehicle = nullptr;
		OccupiedSeatId = NAME_None;
		EnteredDoorId = NAME_None;
		ForceNetUpdate();
		return false;
	}

	FTransform SeatWorldTransform;
	if (!Interaction.GetSeatWorldTransform(SeatId, SeatWorldTransform))
	{
		Interaction.ReleaseSeat(&PrototypeCharacter);
		OccupiedVehicle = nullptr;
		OccupiedSeatId = NAME_None;
		EnteredDoorId = NAME_None;
		ForceNetUpdate();
		return false;
	}

	if (UCharacterMovementComponent* Movement = PrototypeCharacter.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	PrototypeCharacter.SetActorHiddenInGame(true);
	PrototypeCharacter.SetVehicleSeatedPresentation(true);
	if (!PrototypeCharacter.AttachToActor(
		&Vehicle,
		FAttachmentTransformRules::KeepWorldTransform))
	{
		PrototypeCharacter.SetVehicleSeatedPresentation(false);
		PrototypeCharacter.SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* Movement = PrototypeCharacter.GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		Interaction.ReleaseSeat(&PrototypeCharacter);
		OccupiedVehicle = nullptr;
		OccupiedSeatId = NAME_None;
		EnteredDoorId = NAME_None;
		ForceNetUpdate();
		return false;
	}
	PrototypeCharacter.SetActorTransform(
		SeatWorldTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	PrototypeCharacter.ForceNetUpdate();

	Possess(&Vehicle);
	if (GetPawn() == &Vehicle)
	{
		Interaction.SetDriverPresent(true);
		return true;
	}

	// Possession should only fail if external state changed between validation
	// and the authority operation. Restore the Character rather than stranding it.
	PrototypeCharacter.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	PrototypeCharacter.SetVehicleSeatedPresentation(false);
	PrototypeCharacter.SetActorHiddenInGame(false);
	if (UCharacterMovementComponent* Movement = PrototypeCharacter.GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	Interaction.ReleaseSeat(&PrototypeCharacter);
	Interaction.SetDriverPresent(false);
	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	EnteredDoorId = NAME_None;
	ForceNetUpdate();
	return false;
}

bool AOWSTestLabPlayerController::EnterPassengerSeatAuthority(
	AOWSPrototypeCharacter& PrototypeCharacter,
	APawn& Vehicle,
	UOWSStockVehicleInteractionComponent& Interaction,
	const FName DoorId,
	const FName SeatId)
{
	if (!HasAuthority() || GetPawn() != &PrototypeCharacter ||
		PrototypeCharacter.GetController() != this || DoorId.IsNone() ||
		SeatId.IsNone() || Interaction.IsControlSeat(SeatId))
	{
		return false;
	}

	FTransform SeatWorldTransform;
	if (!Interaction.GetSeatWorldTransform(SeatId, SeatWorldTransform))
	{
		return false;
	}

	if (UCharacterMovementComponent* Movement = PrototypeCharacter.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	PrototypeCharacter.SetVehicleSeatedPresentation(true);
	PrototypeCharacter.SetActorHiddenInGame(true);
	if (!PrototypeCharacter.AttachToActor(
		&Vehicle,
		FAttachmentTransformRules::KeepWorldTransform))
	{
		PrototypeCharacter.SetActorHiddenInGame(false);
		PrototypeCharacter.SetVehicleSeatedPresentation(false);
		if (UCharacterMovementComponent* Movement = PrototypeCharacter.GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		return false;
	}
	PrototypeCharacter.SetActorTransform(
		SeatWorldTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	PrototypeCharacter.ForceNetUpdate();
	return true;
}

bool AOWSTestLabPlayerController::ExitVehicleAuthority(
	APawn& Vehicle,
	UOWSStockVehicleInteractionComponent& Interaction)
{
	AOWSPrototypeCharacter* PrototypeCharacter = HomeCharacter.Get();
	if (!HasAuthority() || GetPawn() != &Vehicle ||
		Vehicle.GetController() != this || !IsValid(PrototypeCharacter) ||
		PrototypeCharacter->GetWorld() != GetWorld() ||
		PrototypeCharacter->GetController() != nullptr ||
		OccupiedVehicle != &Vehicle || OccupiedSeatId.IsNone() ||
		!Interaction.IsControlSeat(OccupiedSeatId))
	{
		return false;
	}

	PrototypeCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (!PlaceHomeCharacterBesideVehicle(
		*PrototypeCharacter,
		Vehicle,
		Interaction,
		EnteredDoorId))
	{
		FTransform SeatWorldTransform;
		if (Interaction.GetSeatWorldTransform(OccupiedSeatId, SeatWorldTransform))
		{
			PrototypeCharacter->AttachToActor(
				&Vehicle,
				FAttachmentTransformRules::KeepWorldTransform);
			PrototypeCharacter->SetActorTransform(
				SeatWorldTransform,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		return false;
	}

	PrototypeCharacter->SetVehicleSeatedPresentation(false);
	PrototypeCharacter->SetActorHiddenInGame(false);
	PrototypeCharacter->ForceNetUpdate();
	Possess(PrototypeCharacter);

	if (GetPawn() != PrototypeCharacter)
	{
		PrototypeCharacter->SetActorHiddenInGame(true);
		PrototypeCharacter->SetVehicleSeatedPresentation(true);
		FTransform SeatWorldTransform;
		if (Interaction.GetSeatWorldTransform(OccupiedSeatId, SeatWorldTransform))
		{
			PrototypeCharacter->AttachToActor(
				&Vehicle,
				FAttachmentTransformRules::KeepWorldTransform);
			PrototypeCharacter->SetActorTransform(
				SeatWorldTransform,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		return false;
	}

	if (!Interaction.ReleaseSeat(PrototypeCharacter))
	{
		return false;
	}
	Interaction.SetDriverPresent(false);
	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	EnteredDoorId = NAME_None;
	ForceNetUpdate();
	if (UCharacterMovementComponent* Movement = PrototypeCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	return true;
}

bool AOWSTestLabPlayerController::ExitPassengerSeatAuthority(
	APawn& Vehicle,
	UOWSStockVehicleInteractionComponent& Interaction)
{
	AOWSPrototypeCharacter* PrototypeCharacter = HomeCharacter.Get();
	if (!HasAuthority() || !IsValid(PrototypeCharacter) ||
		GetPawn() != PrototypeCharacter || PrototypeCharacter->GetController() != this ||
		OccupiedVehicle != &Vehicle || OccupiedSeatId.IsNone() ||
		Interaction.IsControlSeat(OccupiedSeatId))
	{
		return false;
	}

	PrototypeCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (!PlaceHomeCharacterBesideVehicle(
		*PrototypeCharacter,
		Vehicle,
		Interaction,
		EnteredDoorId))
	{
		FTransform SeatWorldTransform;
		if (Interaction.GetSeatWorldTransform(OccupiedSeatId, SeatWorldTransform))
		{
			PrototypeCharacter->AttachToActor(
				&Vehicle,
				FAttachmentTransformRules::KeepWorldTransform);
			PrototypeCharacter->SetActorTransform(
				SeatWorldTransform,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		return false;
	}

	if (!Interaction.ReleaseSeat(PrototypeCharacter))
	{
		PrototypeCharacter->SetVehicleSeatedPresentation(true);
		FTransform SeatWorldTransform;
		if (Interaction.GetSeatWorldTransform(OccupiedSeatId, SeatWorldTransform))
		{
			PrototypeCharacter->AttachToActor(
				&Vehicle,
				FAttachmentTransformRules::KeepWorldTransform);
			PrototypeCharacter->SetActorTransform(
				SeatWorldTransform,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		return false;
	}

	PrototypeCharacter->SetVehicleSeatedPresentation(false);
	PrototypeCharacter->SetActorHiddenInGame(false);
	PrototypeCharacter->ForceNetUpdate();
	if (UCharacterMovementComponent* Movement = PrototypeCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	EnteredDoorId = NAME_None;
	ForceNetUpdate();
	return true;
}

bool AOWSTestLabPlayerController::PlaceHomeCharacterBesideVehicle(
	AOWSPrototypeCharacter& PrototypeCharacter,
	const APawn& Vehicle,
	const UOWSStockVehicleInteractionComponent& Interaction,
	const FName PreferredDoorId) const
{
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = PrototypeCharacter.GetCapsuleComponent();
	if (World == nullptr || Capsule == nullptr)
	{
		return false;
	}

	FVector BoundsOrigin = Vehicle.GetActorLocation();
	FVector BoundsExtent = FVector(200.0, 100.0, 75.0);
	Vehicle.GetActorBounds(true, BoundsOrigin, BoundsExtent);

	FVector Right = Vehicle.GetActorRightVector();
	Right.Z = 0.0;
	if (!Right.Normalize())
	{
		Right = FVector::RightVector;
	}

	FVector Forward = Vehicle.GetActorForwardVector();
	Forward.Z = 0.0;
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float VehicleHorizontalRadius = FVector2D(
		BoundsExtent.X,
		BoundsExtent.Y).Size();
	const float ExitDistance =
		VehicleHorizontalRadius + CapsuleRadius + VehicleExitPadding;
	const float ExitZ =
		BoundsOrigin.Z - BoundsExtent.Z + CapsuleHalfHeight + 5.0f;
	const FVector ExitBase(BoundsOrigin.X, BoundsOrigin.Y, ExitZ);
	const FRotator ExitRotation(0.0, Vehicle.GetActorRotation().Yaw, 0.0);
	TArray<FVector, TInlineAllocator<5>> CandidateLocations;
	CandidateLocations.Reserve(5);
	FTransform PreferredDoorWorld;
	if (!PreferredDoorId.IsNone() &&
		Interaction.GetDoorWorldTransform(PreferredDoorId, PreferredDoorWorld))
	{
		FVector DoorOutward = PreferredDoorWorld.GetRotation().GetForwardVector();
		DoorOutward.Z = 0.0f;
		if (DoorOutward.Normalize())
		{
			// Start from the vehicle bounds, not the decorative door marker.
			// The prior marker-relative point could still overlap the chassis,
			// causing TeleportTo to reject it and fall through to the opposite side.
			FVector DoorExit = ExitBase + DoorOutward * ExitDistance;
			DoorExit.Z = ExitZ;
			CandidateLocations.Add(DoorExit);
		}
	}
	CandidateLocations.Append(
	{
		ExitBase + Right * ExitDistance,
		ExitBase - Right * ExitDistance,
		ExitBase - Forward * ExitDistance,
		ExitBase + Forward * ExitDistance
	});

	// TeleportTo delegates to UWorld::FindTeleportSpot and will adjust or reject
	// each destination rather than embedding the capsule in blocking geometry.
	// Actor-level collision must be enabled while it performs that encroachment
	// test; otherwise the capsule is treated as NoCollision and every candidate
	// appears clear. No world tick occurs between enabling collision and moving.
	PrototypeCharacter.SetActorEnableCollision(true);
	for (const FVector& CandidateLocation : CandidateLocations)
	{
		if (PrototypeCharacter.TeleportTo(
			CandidateLocation,
			ExitRotation,
			false,
			false))
		{
			return true;
		}
	}

	PrototypeCharacter.SetActorEnableCollision(false);
	return false;
}

bool AOWSTestLabPlayerController::IsVehicleAutoPlacementEnabled() const
{
	if (GEngine != nullptr)
	{
		if (const UOWSUserSettings* Settings =
			Cast<UOWSUserSettings>(GEngine->GetGameUserSettings()))
		{
			return Settings->IsVehicleAutoPlacementEnabled();
		}
	}
	return true;
}

void AOWSTestLabPlayerController::UpdateRecoveryCheckpoint(const float DeltaTime)
{
	RecoveryCheckpointElapsed += DeltaTime;
	if (RecoveryCheckpointElapsed < RecoveryCheckpointInterval)
	{
		return;
	}
	RecoveryCheckpointElapsed = 0.0f;

	if (AOWSPrototypeCharacter* PrototypeCharacter =
		Cast<AOWSPrototypeCharacter>(GetPawn()))
	{
		const UCharacterMovementComponent* Movement = PrototypeCharacter->GetCharacterMovement();
		if (Movement != nullptr && Movement->IsMovingOnGround())
		{
			LastSafeCharacterTransform = PrototypeCharacter->GetActorTransform();
			LastSafeCharacterTransform.SetRotation(
				FRotator(0.0f, PrototypeCharacter->GetActorRotation().Yaw, 0.0f).Quaternion());
			bHasSafeCharacterTransform = true;
		}
	}
}

void AOWSTestLabPlayerController::RecoverFallenPawn()
{
	AOWSPrototypeCharacter* PrototypeCharacter =
		Cast<AOWSPrototypeCharacter>(GetPawn());
	if (PrototypeCharacter == nullptr)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = PrototypeCharacter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	FTransform RecoveryTransform = bHasSafeCharacterTransform
		? LastSafeCharacterTransform
		: FTransform(FRotator::ZeroRotator, FVector(-450.0f, 0.0f, 200.0f));
	RecoveryTransform.AddToTranslation(FVector(0.0f, 0.0f, 75.0f));
	PrototypeCharacter->SetActorTransform(
		RecoveryTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}

void AOWSTestLabPlayerController::RestoreAfterVehicleLossAuthority()
{
	if (!HasAuthority())
	{
		return;
	}

	AOWSPrototypeCharacter* PrototypeCharacter = HomeCharacter.Get();
	if (IsValid(PrototypeCharacter))
	{
		PrototypeCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		PrototypeCharacter->SetVehicleSeatedPresentation(false);
		PrototypeCharacter->SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* Movement = PrototypeCharacter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (bHasSafeCharacterTransform)
		{
			PrototypeCharacter->SetActorTransform(
				LastSafeCharacterTransform,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		if (GetPawn() != PrototypeCharacter)
		{
			Possess(PrototypeCharacter);
		}
		PrototypeCharacter->ForceNetUpdate();
	}

	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	EnteredDoorId = NAME_None;
	VehicleInteractionStatus = TEXT(
		"The occupied vehicle unloaded or was destroyed; the character was restored.");
	ForceNetUpdate();
}
