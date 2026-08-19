#pragma once

#include "CoreMinimal.h"

#include "OWSMovementTypes.generated.h"

/**
 * Result of validating or requesting OWS's deliberately narrow grounded
 * Mover checkpoint seam.
 */
UENUM(BlueprintType)
enum class EOWSGroundedMovementResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	MissingOwner UMETA(DisplayName = "Missing Owner"),
	MissingMoverComponent UMETA(DisplayName = "Missing Mover Component"),
	AsyncMoverBackendUnsupported UMETA(DisplayName = "Async Mover Backend Unsupported"),
	WalkingModeUnavailable UMETA(DisplayName = "Walking Mode Unavailable"),
	NotWalking UMETA(DisplayName = "Not Walking"),
	PendingMovementModeChange UMETA(DisplayName = "Pending Movement Mode Change"),
	LayeredMovesActive UMETA(DisplayName = "Layered Moves Active"),
	MovementModifiersActive UMETA(DisplayName = "Movement Modifiers Active"),
	MissingCheckpoint UMETA(DisplayName = "Missing Checkpoint"),
	InvalidCheckpointTransform UMETA(DisplayName = "Invalid Checkpoint Transform"),
	InvalidSlotName UMETA(DisplayName = "Invalid Slot Name"),
	SaveManagerUnavailable UMETA(DisplayName = "Save Manager Unavailable"),
	OwnerNotIncludedInSaveSlot UMETA(DisplayName = "Owner Not Included In Save Slot"),
	ComponentNotIncludedInSaveSlot UMETA(DisplayName = "Component Not Included In Save Slot"),
	RequestRejected UMETA(DisplayName = "Request Rejected")
};

/**
 * The only Mover data this experiment persists. It intentionally excludes
 * velocity, non-Walking modes, layered moves, movement modifiers, rollback
 * history, queued effects, montage state, animation state, and actor scale.
 */
USTRUCT(BlueprintType)
struct OWSMOVEMENT_API FOWSGroundedMovementCheckpoint
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement")
	FTransform WorldTransform = FTransform::Identity;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement")
	bool bIsValid = false;

	void Reset()
	{
		WorldTransform = FTransform::Identity;
		bIsValid = false;
	}
};

namespace UE::OWSMovement
{
	/** Plain-data input kept independent of Mover so validation is unit-testable. */
	struct FGroundedCaptureValidationInput
	{
		bool bHasOwner = false;
		bool bHasMover = false;
		bool bUsesAsyncBackend = false;
		bool bHasWalkingMode = false;
		bool bIsWalking = false;
		bool bNextModeIsWalking = false;
		bool bHasLayeredMoves = false;
		bool bHasMovementModifiers = false;
		bool bTransformIsValid = false;
	};

	/** Plain-data input kept independent of Mover so validation is unit-testable. */
	struct FGroundedRestoreValidationInput
	{
		bool bHasOwner = false;
		bool bHasMover = false;
		bool bUsesAsyncBackend = false;
		bool bHasWalkingMode = false;
		bool bHasPendingMovementModeChange = false;
		bool bHasLayeredMoves = false;
		bool bHasMovementModifiers = false;
		bool bHasCheckpoint = false;
		bool bCheckpointTransformIsValid = false;
	};

	OWSMOVEMENT_API bool IsCheckpointTransformValid(const FTransform& Transform);

	OWSMOVEMENT_API EOWSGroundedMovementResult ValidateGroundedCapture(
		const FGroundedCaptureValidationInput& Input);

	OWSMOVEMENT_API EOWSGroundedMovementResult ValidateGroundedRestore(
		const FGroundedRestoreValidationInput& Input);
}
