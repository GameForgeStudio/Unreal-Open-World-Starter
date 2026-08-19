// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "OmniWalkPro.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

UOmniWalkPro::UOmniWalkPro()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics; // Execute before movement logic
}

void UOmniWalkPro::BeginPlay()
{
	Super::BeginPlay();
	PostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UOmniWalkPro::ApplyFinalSurfaceFacing);
	if (bSurfaceMobilityEnabled)
	{
		HijackAndFixCharacter();
	}
}

void UOmniWalkPro::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveDismountInput();
	if (PostActorTickHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickHandle);
		PostActorTickHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

bool UOmniWalkPro::RequestSurfaceDismount()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;
	if (!CMC || !bHasActiveSurface || bAwaitingGroundAfterDismount)
	{
		return false;
	}

	bHasActiveSurface = false;
	bIsGrounded = false;
	bAwaitingGroundAfterDismount = true;
	ActiveSurfaceNormal = FVector::UpVector;
	PrevSurfaceNormal = FVector::UpVector;
	CMC->SetGravityDirection(FVector::DownVector);
	CMC->SetMovementMode(MOVE_Falling);
	return true;
}

void UOmniWalkPro::SetSurfaceMobilityEnabled(bool bEnabled)
{
	if (bSurfaceMobilityEnabled == bEnabled)
	{
		return;
	}

	bSurfaceMobilityEnabled = bEnabled;
	if (bSurfaceMobilityEnabled)
	{
		HijackAndFixCharacter();
	}
	else
	{
		RestoreCharacterSettings();
	}
}

void UOmniWalkPro::HijackAndFixCharacter()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char) return;
	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();

	if (!bSettingsCaptured)
	{
		bOriginalUseControllerRotationYaw = Char->bUseControllerRotationYaw;
		if (CMC)
		{
			bOriginalOrientRotationToMovement = CMC->bOrientRotationToMovement;
			OriginalWalkableFloorAngle = CMC->GetWalkableFloorAngle();
			OriginalGravityDirection = CMC->GetGravityDirection();
		}
		bSettingsCaptured = true;
	}

	if (bAutoFixPawnSettings)
	{
		Char->bUseControllerRotationYaw = false;
		if (CMC)
		{
			CMC->bOrientRotationToMovement = false;
			CMC->SetWalkableFloorAngle(90.0f);
		}
	}

}

void UOmniWalkPro::RestoreCharacterSettings()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !bSettingsCaptured)
	{
		return;
	}

	Char->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
	if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
		CMC->SetWalkableFloorAngle(OriginalWalkableFloorAngle);
		CMC->SetGravityDirection(OriginalGravityDirection);
	}

	bIsGrounded = false;
	PrevSurfaceNormal = FVector::UpVector;
	ActiveSurfaceNormal = FVector::UpVector;
	LastValidTangentForward = FVector::ForwardVector;
	PendingTransitionNormal = FVector::ZeroVector;
	PendingTransitionFrames = 0;
	bHasActiveSurface = false;
}

void UOmniWalkPro::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSurfaceMobilityEnabled) return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;
	RefreshDismountInput(Character);

	if (bAwaitingGroundAfterDismount)
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			CMC->SetGravityDirection(FMath::VInterpTo(CMC->GetGravityDirection(), FVector::DownVector, DeltaTime, AlignmentSpeed));
			if (CMC->IsMovingOnGround())
			{
				bAwaitingGroundAfterDismount = false;
			}
		}
		if (bAwaitingGroundAfterDismount) return;
	}

	UpdateSurfaceAdhesion(Character, DeltaTime);
	ApplyInputCorrection(Character);
}

void UOmniWalkPro::UpdateSurfaceAdhesion(ACharacter* Character, float DeltaTime)
{
	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!CMC) return;
	// GASPALS owns gravity and collision while executing a traversal montage.
	if (CMC->IsFlying()) return;

	// Resolve the current support and a leading movement probe independently.
	// The probe is allowed to hand off to a confirmed perpendicular surface even
	// while the current support remains valid: this is what carries a character
	// through a wall-to-wall or wall-to-ceiling corner instead of clinging to the
	// old surface until it disappears.
	FVector TargetUp = bUseMultiPointAveraging ? GetAveragedNormal(Character) : FVector::UpVector;
	if (bAllowSharpSurfaceTransfer)
	{
		const FVector ForwardNormal = GetForwardSurfaceNormal(Character);
		if (!ForwardNormal.IsNearlyZero() &&
			(TargetUp.Equals(FVector::UpVector, 0.01f) || FVector::DotProduct(ForwardNormal, TargetUp) < 0.95f))
		{
			if (PendingTransitionNormal.IsNearlyZero() || FVector::DotProduct(PendingTransitionNormal, ForwardNormal) < 0.98f)
			{
				PendingTransitionNormal = ForwardNormal;
				PendingTransitionFrames = 1;
			}
			else
			{
				++PendingTransitionFrames;
			}

			if (PendingTransitionFrames >= FMath::Max(1, SurfaceTransitionConfirmFrames))
			{
				TargetUp = PendingTransitionNormal;
				PendingTransitionNormal = FVector::ZeroVector;
				PendingTransitionFrames = 0;
			}
		}
		else
		{
			PendingTransitionNormal = FVector::ZeroVector;
			PendingTransitionFrames = 0;
		}
	}

	bool bFoundSurface = !TargetUp.Equals(FVector::UpVector, 0.01f);
	if (bFoundSurface)
	{
		ActiveSurfaceNormal = TargetUp;
		bHasActiveSurface = true;
	}
	else if (bHasActiveSurface)
	{
		// A front-probe acquisition begins with the pawn only partly rotated.
		// Keep the known support only until that same smooth interpolation has
		// reached the target frame; this is not a snap or a permanent cling.
		const float AlignmentDot = FVector::DotProduct(Character->GetActorUpVector(), ActiveSurfaceNormal);
		if (AlignmentDot < 0.999f)
		{
			TargetUp = ActiveSurfaceNormal;
			bFoundSurface = true;
		}
		else
		{
			bHasActiveSurface = false;
			ActiveSurfaceNormal = FVector::UpVector;
		}
	}
	bIsGrounded = bFoundSurface;

	// 2. Momentum & Adhesion Logic
	if (bFoundSurface)
	{
		// Force the CMC to accept vertical surfaces
		CMC->SetWalkableFloorAngle(90.0f);

		if (bPreserveMomentumOnCorners)
		{
			RealignVelocityOnSurfaceChange(CMC, TargetUp);
		}

		// Do not push a rotating capsule into the new plane. That creates overlap,
		// and CharacterMovement's depenetration can turn it into launch velocity.
		const bool bFrameAlignedToSupport = FVector::DotProduct(Character->GetActorUpVector(), TargetUp) > 0.95f;
		if (CMC->IsFalling() && bFrameAlignedToSupport)
		{
			CMC->AddImpulse(-TargetUp * AdhesionForce * DeltaTime, true);
		}
	}

	// 3. Update Native Gravity (UE 5.4+ support)
	CMC->SetGravityDirection(FMath::VInterpTo(CMC->GetGravityDirection(), -TargetUp, DeltaTime, AlignmentSpeed));

	PrevSurfaceNormal = TargetUp;
}

void UOmniWalkPro::ApplyFinalSurfaceFacing(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	if (!bSurfaceMobilityEnabled || World != GetWorld()) return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !CMC || CMC->IsFlying()) return;

	const FVector TargetUp = bAwaitingGroundAfterDismount ? FVector::UpVector
		: (bHasActiveSurface ? ActiveSurfaceNormal : Character->GetActorUpVector());
	FVector TargetForward = Character->GetActorForwardVector();
	if (bOrientRotationToMovementPro && CMC->Velocity.SizeSquared() > 1000.0f)
	{
		TargetForward = FVector::VectorPlaneProject(CMC->Velocity, TargetUp).GetSafeNormal();
	}
	else if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		const FVector CameraTangent = FVector::VectorPlaneProject(PC->GetControlRotation().Vector(), TargetUp).GetSafeNormal();
		if (!CameraTangent.IsNearlyZero())
		{
			TargetForward = CameraTangent;
		}
	}

	TargetForward = FVector::VectorPlaneProject(TargetForward, TargetUp).GetSafeNormal();
	if (TargetForward.IsNearlyZero())
	{
		TargetForward = FVector::VectorPlaneProject(LastValidTangentForward, TargetUp).GetSafeNormal();
	}
	if (TargetForward.IsNearlyZero())
	{
		TargetForward = FVector::VectorPlaneProject(FVector::ForwardVector, TargetUp).GetSafeNormal();
	}
	LastValidTangentForward = TargetForward;

	const FQuat TargetQuat = FRotationMatrix::MakeFromZX(TargetUp, TargetForward).ToQuat();
	const float RotationSmoothness = bIsGrounded ? AlignmentSpeed : AlignmentSpeed * 1.5f;
	Character->SetActorRotation(FMath::QInterpTo(Character->GetActorQuat(), TargetQuat, DeltaTime, RotationSmoothness));
}

void UOmniWalkPro::RefreshDismountInput(ACharacter* Character)
{
	if (!Character->IsLocallyControlled())
	{
		RemoveDismountInput();
		return;
	}
	if (DismountInputComponent) return;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController) return;

	DismountInputComponent = NewObject<UInputComponent>(PlayerController, TEXT("OWSSurfaceDismountInput"));
	DismountInputComponent->Priority = 900;
	DismountInputComponent->RegisterComponent();
	DismountInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &ThisClass::HandleJumpPressed);
	DismountInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ThisClass::HandleJumpPressed);
	PlayerController->PushInputComponent(DismountInputComponent);
	DismountInputOwner = PlayerController;
}

void UOmniWalkPro::RemoveDismountInput()
{
	if (DismountInputOwner.IsValid() && DismountInputComponent)
	{
		DismountInputOwner->PopInputComponent(DismountInputComponent);
	}
	DismountInputComponent = nullptr;
	DismountInputOwner.Reset();
	LastSurfaceJumpTime = -1.0f;
}

void UOmniWalkPro::HandleJumpPressed()
{
	if (!bHasActiveSurface || bAwaitingGroundAfterDismount) return;
	UWorld* World = GetWorld();
	if (!World) return;

	const float CurrentTime = World->GetTimeSeconds();
	if (LastSurfaceJumpTime >= 0.0f && CurrentTime - LastSurfaceJumpTime <= DoubleTapDismountWindow)
	{
		RequestSurfaceDismount();
		LastSurfaceJumpTime = -1.0f;
		return;
	}
	LastSurfaceJumpTime = CurrentTime;
}

FVector UOmniWalkPro::GetAveragedNormal(const ACharacter* Character)
{
	const FVector RootLoc = Character->GetActorLocation();
	// During surface acquisition, trace toward the accepted support rather than
	// the pawn's partially rotated frame. This prevents a stationary pawn from
	// losing the surface halfway through the same interpolation used while moving.
	const FVector CurrentUp = bHasActiveSurface ? ActiveSurfaceNormal : Character->GetActorUpVector();
	const float CapsuleHH = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float TraceLen = CapsuleHH + TraceDistance;

	// 5-Point sampling pattern
	const FVector Offsets[5] = {
		FVector::ZeroVector,
		Character->GetActorForwardVector() * MultiTraceOffset,
		-Character->GetActorForwardVector() * MultiTraceOffset,
		Character->GetActorRightVector() * MultiTraceOffset,
		-Character->GetActorRightVector() * MultiTraceOffset
	};

	FVector AccNormal = FVector::ZeroVector;
	int32 HitCount = 0;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	for (const FVector& Offset : Offsets)
	{
		FHitResult Hit;
		FVector TraceStart = RootLoc + Offset;
		FVector TraceEnd = TraceStart - (CurrentUp * TraceLen);

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) &&
			IsHitTraversable(Hit))
		{
			AccNormal += Hit.Normal;
			HitCount++;
		}
	}

	return (HitCount > 0) ? (AccNormal / (float)HitCount).GetSafeNormal() : FVector::UpVector;
}

FVector UOmniWalkPro::GetForwardSurfaceNormal(const ACharacter* Character) const
{
	const UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	// Surface travel can be entirely vertical in world space. SizeSquared2D()
	// therefore misses the exact wall-to-ceiling case this probe must detect.
	if (!CMC || CMC->Velocity.SizeSquared() < 100.0f) return FVector::ZeroVector;

	const FVector CurrentSupport = bHasActiveSurface ? ActiveSurfaceNormal : Character->GetActorUpVector();
	FVector Forward = FVector::VectorPlaneProject(CMC->Velocity, CurrentSupport).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Character->GetActorForwardVector().GetSafeNormal();
	}
	const FVector Start = Character->GetActorLocation() + Character->GetActorUpVector() * 10.0f;
	const FVector End = Start + Forward * ForwardProbeDistance;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	FHitResult Hit;
	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float EffectiveProbeRadius = FMath::Max(ForwardProbeRadius, CapsuleRadius);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(EffectiveProbeRadius), Params) && IsHitTraversable(Hit))
	{
		if (Hit.Distance > SurfaceTransitionCommitDistance)
		{
			return FVector::ZeroVector;
		}
		// This is a sharp-corner handoff probe: retain only a surface roughly
		// perpendicular to the active support. Its normal becomes the next floor.
		const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
		if (FMath::Abs(FVector::DotProduct(Normal, CurrentSupport)) < 0.25f)
		{
			return Normal;
		}
	}
	return FVector::ZeroVector;
}

bool UOmniWalkPro::IsHitTraversable(const FHitResult& Hit) const
{
	const AActor* HitActor = Hit.GetActor();
	return HitActor != nullptr &&
		(NonTraversableActorTag.IsNone() || !HitActor->ActorHasTag(NonTraversableActorTag));
}

void UOmniWalkPro::RealignVelocityOnSurfaceChange(UCharacterMovementComponent* CMC, FVector NewUp)
{
	if (!CMC || CMC->Velocity.IsNearlyZero() || PrevSurfaceNormal.IsNearlyZero() || PrevSurfaceNormal.Equals(NewUp, 0.01f)) return;

	// Rotate once into the new tangent frame. Interpolating only one frame and
	// then marking the new normal as previous leaves most velocity pointing into
	// the corner, which collision depenetration can convert into an ejection.
	const FQuat DeltaRotation = FQuat::FindBetweenNormals(PrevSurfaceNormal, NewUp);
	CMC->Velocity = DeltaRotation.RotateVector(CMC->Velocity);
}

void UOmniWalkPro::ApplyInputCorrection(ACharacter* Character)
{
	if (!Character->IsLocallyControlled()) return;

	const FVector RawInput = Character->ConsumeMovementInputVector();
	if (RawInput.SizeSquared() > 0.001f)
	{
		const FVector SurfaceNormal = bHasActiveSurface ? ActiveSurfaceNormal : Character->GetActorUpVector();
		if (SurfaceNormal.Equals(FVector::UpVector, 0.01f))
		{
			// Preserve the normal GASPALS ground-input path exactly.
			Character->AddMovementInput(RawInput.GetSafeNormal(), RawInput.Size());
			return;
		}

		const APlayerController* PC = Cast<APlayerController>(Character->GetController());
		const float ControlYaw = PC ? PC->GetControlRotation().Yaw : Character->GetActorRotation().Yaw;
		const FRotator YawRotation(0.0f, ControlYaw, 0.0f);
		const FVector ReferenceForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector ReferenceRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// The GASPALS input handler has already combined the two stick axes into
		// a world vector. Recover each scalar against its original camera-yaw
		// basis, then rebuild them in the traversable surface's tangent plane.
		const float ForwardAmount = FVector::DotProduct(RawInput, ReferenceForward);
		const float RightAmount = FVector::DotProduct(RawInput, ReferenceRight);
		// Recovering the stick scalars uses the original yaw-only GASPALS input
		// basis above. The resulting movement frame, however, must remain camera
		// relative: project the complete camera forward direction onto the current
		// traversable surface, exactly as normal ground movement does.
		FVector SurfaceForward = PC
			? FVector::VectorPlaneProject(PC->GetControlRotation().Vector(), SurfaceNormal).GetSafeNormal()
			: FVector::ZeroVector;
		if (SurfaceForward.IsNearlyZero())
		{
			// Looking directly into a surface has no mathematical forward direction
			// on that surface. Hold the most recent valid tangent until the camera
			// leaves that singular orientation; do not invent world-up movement.
			SurfaceForward = FVector::VectorPlaneProject(LastValidTangentForward, SurfaceNormal).GetSafeNormal();
		}
		if (SurfaceForward.IsNearlyZero())
		{
			SurfaceForward = FVector::VectorPlaneProject(Character->GetActorForwardVector(), SurfaceNormal).GetSafeNormal();
		}
		LastValidTangentForward = SurfaceForward;
		const FVector SurfaceRight = FVector::CrossProduct(SurfaceNormal, SurfaceForward).GetSafeNormal();

		const FVector SurfaceInput = SurfaceForward * ForwardAmount + SurfaceRight * RightAmount;
		if (!SurfaceInput.IsNearlyZero())
		{
			Character->AddMovementInput(SurfaceInput.GetSafeNormal(), SurfaceInput.Size());
		}
	}
}
