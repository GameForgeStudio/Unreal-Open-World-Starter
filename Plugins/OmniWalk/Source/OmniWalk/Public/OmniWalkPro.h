// Copyright (c) 2026 GregOrigin. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OmniWalkPro.generated.h"

// FORWARD DECLARATION: Fixes C2061
class UCharacterMovementComponent;
class ACharacter;
class APlayerController;
class UInputComponent;

UCLASS(ClassGroup = (OmniWalk), meta = (BlueprintSpawnableComponent))
class OMNIWALK_API UOmniWalkPro : public UActorComponent
{
	GENERATED_BODY()

public:
	UOmniWalkPro();

	/**
	 * The owning character can use traversable surfaces as its floor. This is
	 * exposed on every core character, but games may disable it per character.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OmniWalk|Activation")
	bool bSurfaceMobilityEnabled = true;

	/** Enables or disables surface mobility without removing the component. */
	UFUNCTION(BlueprintCallable, Category = "OmniWalk|Activation")
	void SetSurfaceMobilityEnabled(bool bEnabled);

	/** Releases the current surface and falls under world-down gravity until grounded. */
	UFUNCTION(BlueprintCallable, Category = "OmniWalk|Traversal")
	bool RequestSurfaceDismount();

	UPROPERTY(EditAnywhere, Category = "OmniWalk")
	float TraceDistance = 200.0f;

	UPROPERTY(EditAnywhere, Category = "OmniWalk")
	float AlignmentSpeed = 12.0f;

	UPROPERTY(EditAnywhere, Category = "OmniWalk")
	float AdhesionForce = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "OmniWalk")
	bool bOrientRotationToMovementPro = true;

	UPROPERTY(EditAnywhere, Category = "OmniWalk|Advanced")
	bool bUseMultiPointAveraging = true;

	/** Actors carrying this tag retain collision but cannot become OmniWalk support. */
	UPROPERTY(EditAnywhere, Category = "OmniWalk|Activation")
	FName NonTraversableActorTag = TEXT("OWS.NonTraversable");

	UPROPERTY(EditAnywhere, Category = "OmniWalk")
	bool bAutoFixPawnSettings = true;

	UPROPERTY(EditAnywhere, Category = "OmniWalk|Advanced")
	float MultiTraceOffset = 35.0f;

	UPROPERTY(EditAnywhere, Category = "OmniWalk|Advanced")
	bool bPreserveMomentumOnCorners = true;

	/** Enables a moving character to acquire an existing near-vertical surface in front of it. */
	UPROPERTY(EditAnywhere, Category = "OmniWalk|Surface Transfer")
	bool bAllowSharpSurfaceTransfer = true;

	UPROPERTY(EditAnywhere, Category = "OmniWalk|Surface Transfer", meta = (ClampMin = "0.0"))
	float ForwardProbeDistance = 150.0f;

	UPROPERTY(EditAnywhere, Category = "OmniWalk|Surface Transfer", meta = (ClampMin = "1.0"))
	float ForwardProbeRadius = 28.0f;

	/** A sharp handoff may only commit this close to capsule contact. */
	UPROPERTY(EditAnywhere, Category = "OmniWalk|Surface Transfer", meta = (ClampMin = "0.0"))
	float SurfaceTransitionCommitDistance = 35.0f;

	/** Consecutive leading-probe confirmations required before changing support. */
	UPROPERTY(EditAnywhere, Category = "OmniWalk|Surface Transfer", meta = (ClampMin = "1"))
	int32 SurfaceTransitionConfirmFrames = 2;

	/** Maximum interval between jump presses that requests a surface dismount. */
	UPROPERTY(EditAnywhere, Category = "OmniWalk|Traversal", meta = (ClampMin = "0.05"))
	float DoubleTapDismountWindow = 0.30f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void HijackAndFixCharacter();
	void RestoreCharacterSettings();
	void UpdateSurfaceAdhesion(ACharacter* Character, float DeltaTime);
	void ApplyInputCorrection(ACharacter* Character);
	void ApplyFinalSurfaceFacing(UWorld* World, ELevelTick TickType, float DeltaTime);
	void RefreshDismountInput(ACharacter* Character);
	void RemoveDismountInput();
	void HandleJumpPressed();

	// Internal Helpers
	FVector GetAveragedNormal(const ACharacter* Character);
	FVector GetForwardSurfaceNormal(const ACharacter* Character) const;
	bool IsHitTraversable(const FHitResult& Hit) const;
	void RealignVelocityOnSurfaceChange(UCharacterMovementComponent* CMC, FVector NewUp);

	FVector PrevSurfaceNormal = FVector::UpVector;
	// Keeps a newly acquired traversable surface stable while the character's
	// frame interpolates into it. Without this, traces follow the partially
	// rotated frame and can lose the surface when input stops mid-transition.
	FVector ActiveSurfaceNormal = FVector::UpVector;
	FVector LastValidTangentForward = FVector::ForwardVector;
	FVector PendingTransitionNormal = FVector::ZeroVector;
	int32 PendingTransitionFrames = 0;
	bool bHasActiveSurface = false;
	bool bIsGrounded = false;
	bool bSettingsCaptured = false;
	bool bOriginalUseControllerRotationYaw = false;
	bool bOriginalOrientRotationToMovement = false;
	float OriginalWalkableFloorAngle = 45.0f;
	FVector OriginalGravityDirection = FVector::DownVector;
	FDelegateHandle PostActorTickHandle;
	TObjectPtr<UInputComponent> DismountInputComponent;
	TWeakObjectPtr<APlayerController> DismountInputOwner;
	float LastSurfaceJumpTime = -1.0f;
	bool bAwaitingGroundAfterDismount = false;
};
