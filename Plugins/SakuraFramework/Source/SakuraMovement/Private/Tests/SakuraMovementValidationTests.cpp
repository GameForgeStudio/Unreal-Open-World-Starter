#if WITH_DEV_AUTOMATION_TESTS

#include "SakuraMovementTypes.h"

#include "Misc/AutomationTest.h"

#include <limits>

namespace UE::SakuraMovement::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSakuraGroundedCaptureValidationTest,
		"Sakura.Movement.GroundedBaseline.CaptureValidation",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSakuraGroundedCaptureValidationTest::RunTest(const FString& Parameters)
	{
		FGroundedCaptureValidationInput Input;
		Input.bHasOwner = true;
		Input.bHasMover = true;
		Input.bHasWalkingMode = true;
		Input.bIsWalking = true;
		Input.bNextModeIsWalking = true;
		Input.bTransformIsValid = true;

		TestTrue(TEXT("Strict grounded state is accepted"),
			ValidateGroundedCapture(Input) == ESakuraGroundedMovementResult::Success);

		Input.bIsWalking = false;
		TestTrue(TEXT("Midair/non-Walking capture is rejected"),
			ValidateGroundedCapture(Input) == ESakuraGroundedMovementResult::NotWalking);
		Input.bIsWalking = true;

		Input.bNextModeIsWalking = false;
		TestTrue(TEXT("Queued movement-mode transition is rejected"),
			ValidateGroundedCapture(Input) ==
				ESakuraGroundedMovementResult::PendingMovementModeChange);
		Input.bNextModeIsWalking = true;

		Input.bHasLayeredMoves = true;
		TestTrue(TEXT("Layered movement state is rejected"),
			ValidateGroundedCapture(Input) ==
				ESakuraGroundedMovementResult::LayeredMovesActive);
		Input.bHasLayeredMoves = false;

		Input.bHasMovementModifiers = true;
		TestTrue(TEXT("Movement modifier state is rejected"),
			ValidateGroundedCapture(Input) ==
				ESakuraGroundedMovementResult::MovementModifiersActive);
		Input.bHasMovementModifiers = false;

		Input.bUsesAsyncBackend = true;
		TestTrue(TEXT("Async backend is rejected because this seam uses FTeleportEffect"),
			ValidateGroundedCapture(Input) ==
				ESakuraGroundedMovementResult::AsyncMoverBackendUnsupported);

		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSakuraGroundedRestoreValidationTest,
		"Sakura.Movement.GroundedBaseline.RestoreValidation",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSakuraGroundedRestoreValidationTest::RunTest(const FString& Parameters)
	{
		FGroundedRestoreValidationInput Input;
		Input.bHasOwner = true;
		Input.bHasMover = true;
		Input.bHasWalkingMode = true;
		Input.bHasCheckpoint = true;
		Input.bCheckpointTransformIsValid = true;

		TestTrue(TEXT("Grounded checkpoint restore is accepted"),
			ValidateGroundedRestore(Input) == ESakuraGroundedMovementResult::Success);

		Input.bHasPendingMovementModeChange = true;
		TestTrue(TEXT("Pending mode change rejects restore"),
			ValidateGroundedRestore(Input) ==
				ESakuraGroundedMovementResult::PendingMovementModeChange);
		Input.bHasPendingMovementModeChange = false;

		Input.bHasCheckpoint = false;
		TestTrue(TEXT("Missing checkpoint fails closed"),
			ValidateGroundedRestore(Input) ==
				ESakuraGroundedMovementResult::MissingCheckpoint);
		Input.bHasCheckpoint = true;

		Input.bCheckpointTransformIsValid = false;
		TestTrue(TEXT("Invalid checkpoint transform fails closed"),
			ValidateGroundedRestore(Input) ==
				ESakuraGroundedMovementResult::InvalidCheckpointTransform);

		const FTransform ValidTransform = FTransform::Identity;
		TestTrue(TEXT("Identity transform is valid"),
			IsCheckpointTransformValid(ValidTransform));

		FTransform InvalidTransform = FTransform::Identity;
		InvalidTransform.SetLocation(FVector(
			std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0));
		TestFalse(TEXT("NaN transform is invalid"),
			IsCheckpointTransformValid(InvalidTransform));

		return true;
	}
}

#endif
