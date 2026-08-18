#include "SakuraMovementTypes.h"

namespace UE::SakuraMovement
{
	bool IsCheckpointTransformValid(const FTransform& Transform)
	{
		return !Transform.ContainsNaN() && Transform.GetRotation().IsNormalized();
	}

	ESakuraGroundedMovementResult ValidateGroundedCapture(
		const FGroundedCaptureValidationInput& Input)
	{
		if (!Input.bHasOwner)
		{
			return ESakuraGroundedMovementResult::MissingOwner;
		}
		if (!Input.bHasMover)
		{
			return ESakuraGroundedMovementResult::MissingMoverComponent;
		}
		if (Input.bUsesAsyncBackend)
		{
			return ESakuraGroundedMovementResult::AsyncMoverBackendUnsupported;
		}
		if (!Input.bHasWalkingMode)
		{
			return ESakuraGroundedMovementResult::WalkingModeUnavailable;
		}
		if (!Input.bIsWalking)
		{
			return ESakuraGroundedMovementResult::NotWalking;
		}
		if (!Input.bNextModeIsWalking)
		{
			return ESakuraGroundedMovementResult::PendingMovementModeChange;
		}
		if (Input.bHasLayeredMoves)
		{
			return ESakuraGroundedMovementResult::LayeredMovesActive;
		}
		if (Input.bHasMovementModifiers)
		{
			return ESakuraGroundedMovementResult::MovementModifiersActive;
		}
		if (!Input.bTransformIsValid)
		{
			return ESakuraGroundedMovementResult::InvalidCheckpointTransform;
		}

		return ESakuraGroundedMovementResult::Success;
	}

	ESakuraGroundedMovementResult ValidateGroundedRestore(
		const FGroundedRestoreValidationInput& Input)
	{
		if (!Input.bHasOwner)
		{
			return ESakuraGroundedMovementResult::MissingOwner;
		}
		if (!Input.bHasMover)
		{
			return ESakuraGroundedMovementResult::MissingMoverComponent;
		}
		if (Input.bUsesAsyncBackend)
		{
			return ESakuraGroundedMovementResult::AsyncMoverBackendUnsupported;
		}
		if (!Input.bHasWalkingMode)
		{
			return ESakuraGroundedMovementResult::WalkingModeUnavailable;
		}
		if (Input.bHasPendingMovementModeChange)
		{
			return ESakuraGroundedMovementResult::PendingMovementModeChange;
		}
		if (Input.bHasLayeredMoves)
		{
			return ESakuraGroundedMovementResult::LayeredMovesActive;
		}
		if (Input.bHasMovementModifiers)
		{
			return ESakuraGroundedMovementResult::MovementModifiersActive;
		}
		if (!Input.bHasCheckpoint)
		{
			return ESakuraGroundedMovementResult::MissingCheckpoint;
		}
		if (!Input.bCheckpointTransformIsValid)
		{
			return ESakuraGroundedMovementResult::InvalidCheckpointTransform;
		}

		return ESakuraGroundedMovementResult::Success;
	}
}
