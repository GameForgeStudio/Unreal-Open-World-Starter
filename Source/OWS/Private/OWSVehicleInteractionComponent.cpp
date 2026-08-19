#include "OWSVehicleInteractionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
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
#include "TimerManager.h"

UOWSVehicleInteractionComponent::UOWSVehicleInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOWSVehicleInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InstallInput();
}

void UOWSVehicleInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExitHoldTimer);
	}
	RemoveInput();
	Super::EndPlay(EndPlayReason);
}

void UOWSVehicleInteractionComponent::InstallInput()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || InteractionInput)
	{
		return;
	}

	InteractionInput = NewObject<UInputComponent>(Controller, TEXT("OWSVehicleInteractionInput"));
	InteractionInput->Priority = 1200;
	InteractionInput->RegisterComponent();
	InteractionInput->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed,
		this, &ThisClass::HandleEnterPressed).bConsumeInput = false;
	InteractionInput->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed,
		this, &ThisClass::HandleExitPressed).bConsumeInput = false;
	InteractionInput->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Released,
		this, &ThisClass::HandleExitReleased).bConsumeInput = false;
	Controller->PushInputComponent(InteractionInput);
}

void UOWSVehicleInteractionComponent::RemoveInput()
{
	if (APlayerController* Controller = Cast<APlayerController>(GetOwner());
		Controller && InteractionInput)
	{
		Controller->PopInputComponent(InteractionInput);
	}
	InteractionInput = nullptr;
}

void UOWSVehicleInteractionComponent::HandleEnterPressed()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller ||
		Controller->IsInputKeyDown(EKeys::Gamepad_LeftShoulder) ||
		Controller->IsInputKeyDown(EKeys::Gamepad_RightShoulder) ||
		OccupiedVehicle)
	{
		return;
	}
	TryEnterVehicle();
}

void UOWSVehicleInteractionComponent::HandleExitPressed()
{
	if (!OccupiedVehicle)
	{
		return;
	}
	if (GetVehicleSpeedMph() <= ImmediateExitSpeedMph)
	{
		TryExitVehicle();
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ExitHoldTimer, this, &ThisClass::CompleteHeldExit,
			MovingExitHoldSeconds, false);
	}
}

void UOWSVehicleInteractionComponent::HandleExitReleased()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExitHoldTimer);
	}
}

void UOWSVehicleInteractionComponent::CompleteHeldExit()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (Controller && Controller->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right))
	{
		TryExitVehicle();
	}
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

	Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (!PlaceCharacterAtDriverDoor(*Character, *Vehicle, *Interaction))
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
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	OccupiedVehicle = nullptr;
	OccupiedSeatId = NAME_None;
	HomeCharacter = nullptr;
	ApplyVehicleInputContext(false);
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
		if (!Candidate || !CandidateInteraction || Candidate->GetController())
		{
			continue;
		}
		FName DoorId;
		float Distance = 0.0f;
		if (!CandidateInteraction->FindFacedDoor(
			Character.GetActorLocation(), Character.GetActorForwardVector(),
			MaximumEnterDistance, DoorId, Distance) || Distance >= BestDistance)
		{
			continue;
		}
		FName SeatId;
		if (!CandidateInteraction->SelectAvailableSeat(DoorId, true, &Character, SeatId) ||
			!CandidateInteraction->IsControlSeat(SeatId))
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
	FTransform DriverDoor;
	if (!World || !Capsule || !Interaction.GetDoorWorldTransform(TEXT("LeftDoor"), DriverDoor))
	{
		return false;
	}

	FVector Left = -Vehicle.GetActorRightVector();
	Left.Z = 0.0f;
	Left.Normalize();
	const FVector Base = DriverDoor.GetLocation() + Left * DriverDoorExitPadding;
	const FVector Candidates[] = {
		Base,
		Base + Vehicle.GetActorForwardVector() * 100.0f,
		Base - Vehicle.GetActorForwardVector() * 100.0f,
		Base + Left * 100.0f
	};
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
				Candidate,
				FRotator(0.0f, Vehicle.GetActorRotation().Yaw, 0.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
			return true;
		}
	}
	return false;
}

float UOWSVehicleInteractionComponent::GetVehicleSpeedMph() const
{
	const APawn* Vehicle = OccupiedVehicle.Get();
	const UPrimitiveComponent* Body = Vehicle
		? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()) : nullptr;
	const FVector Velocity = Body ? Body->GetPhysicsLinearVelocity() :
		(Vehicle ? Vehicle->GetVelocity() : FVector::ZeroVector);
	return Velocity.Size() * 0.0223694f;
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
