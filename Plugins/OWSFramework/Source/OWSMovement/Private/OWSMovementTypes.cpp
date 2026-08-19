#include "OWSMovementTypes.h"

namespace UE::OWSMovement
{
	bool IsCheckpointTransformValid(const FTransform& Transform)
	{
		return !Transform.ContainsNaN() && Transform.GetRotation().IsNormalized();
	}

	EOWSGroundedMovementResult ValidateGroundedCapture(
		const FGroundedCaptureValidationInput& Input)
	{
		if (!Input.bHasOwner)
		{
			return EOWSGroundedMovementResult::MissingOwner;
		}
		if (!Input.bHasMover)
		{
			return EOWSGroundedMovementResult::MissingMoverComponent;
		}
		if (Input.bUsesAsyncBackend)
		{
			return EOWSGroundedMovementResult::AsyncMoverBackendUnsupported;
		}
		if (!Input.bHasWalkingMode)
		{
			return EOWSGroundedMovementResult::WalkingModeUnavailable;
		}
		if (!Input.bIsWalking)
		{
			return EOWSGroundedMovementResult::NotWalking;
		}
		if (!Input.bNextModeIsWalking)
		{
			return EOWSGroundedMovementResult::PendingMovementModeChange;
		}
		if (Input.bHasLayeredMoves)
		{
			return EOWSGroundedMovementResult::LayeredMovesActive;
		}
		if (Input.bHasMovementModifiers)
		{
			return EOWSGroundedMovementResult::MovementModifiersActive;
		}
		if (!Input.bTransformIsValid)
		{
			return EOWSGroundedMovementResult::InvalidCheckpointTransform;
		}

		return EOWSGroundedMovementResult::Success;
	}

	EOWSGroundedMovementResult ValidateGroundedRestore(
		const FGroundedRestoreValidationInput& Input)
	{
		if (!Input.bHasOwner)
		{
			return EOWSGroundedMovementResult::MissingOwner;
		}
		if (!Input.bHasMover)
		{
			return EOWSGroundedMovementResult::MissingMoverComponent;
		}
		if (Input.bUsesAsyncBackend)
		{
			return EOWSGroundedMovementResult::AsyncMoverBackendUnsupported;
		}
		if (!Input.bHasWalkingMode)
		{
			return EOWSGroundedMovementResult::WalkingModeUnavailable;
		}
		if (Input.bHasPendingMovementModeChange)
		{
			return EOWSGroundedMovementResult::PendingMovementModeChange;
		}
		if (Input.bHasLayeredMoves)
		{
			return EOWSGroundedMovementResult::LayeredMovesActive;
		}
		if (Input.bHasMovementModifiers)
		{
			return EOWSGroundedMovementResult::MovementModifiersActive;
		}
		if (!Input.bHasCheckpoint)
		{
			return EOWSGroundedMovementResult::MissingCheckpoint;
		}
		if (!Input.bCheckpointTransformIsValid)
		{
			return EOWSGroundedMovementResult::InvalidCheckpointTransform;
		}

		return EOWSGroundedMovementResult::Success;
	}
}
