#if WITH_DEV_AUTOMATION_TESTS

#include "OWSMovementTypes.h"

#include "Misc/AutomationTest.h"

#include <limits>

namespace UE::OWSMovement::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FOWSGroundedCaptureValidationTest,
		"OWS.Movement.GroundedBaseline.CaptureValidation",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FOWSGroundedCaptureValidationTest::RunTest(const FString& Parameters)
	{
		FGroundedCaptureValidationInput Input;
		Input.bHasOwner = true;
		Input.bHasMover = true;
		Input.bHasWalkingMode = true;
		Input.bIsWalking = true;
		Input.bNextModeIsWalking = true;
		Input.bTransformIsValid = true;

		TestTrue(TEXT("Strict grounded state is accepted"),
			ValidateGroundedCapture(Input) == EOWSGroundedMovementResult::Success);

		Input.bIsWalking = false;
		TestTrue(TEXT("Midair/non-Walking capture is rejected"),
			ValidateGroundedCapture(Input) == EOWSGroundedMovementResult::NotWalking);
		Input.bIsWalking = true;

		Input.bNextModeIsWalking = false;
		TestTrue(TEXT("Queued movement-mode transition is rejected"),
			ValidateGroundedCapture(Input) ==
				EOWSGroundedMovementResult::PendingMovementModeChange);
		Input.bNextModeIsWalking = true;

		Input.bHasLayeredMoves = true;
		TestTrue(TEXT("Layered movement state is rejected"),
			ValidateGroundedCapture(Input) ==
				EOWSGroundedMovementResult::LayeredMovesActive);
		Input.bHasLayeredMoves = false;

		Input.bHasMovementModifiers = true;
		TestTrue(TEXT("Movement modifier state is rejected"),
			ValidateGroundedCapture(Input) ==
				EOWSGroundedMovementResult::MovementModifiersActive);
		Input.bHasMovementModifiers = false;

		Input.bUsesAsyncBackend = true;
		TestTrue(TEXT("Async backend is rejected because this seam uses FTeleportEffect"),
			ValidateGroundedCapture(Input) ==
				EOWSGroundedMovementResult::AsyncMoverBackendUnsupported);

		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FOWSGroundedRestoreValidationTest,
		"OWS.Movement.GroundedBaseline.RestoreValidation",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FOWSGroundedRestoreValidationTest::RunTest(const FString& Parameters)
	{
		FGroundedRestoreValidationInput Input;
		Input.bHasOwner = true;
		Input.bHasMover = true;
		Input.bHasWalkingMode = true;
		Input.bHasCheckpoint = true;
		Input.bCheckpointTransformIsValid = true;

		TestTrue(TEXT("Grounded checkpoint restore is accepted"),
			ValidateGroundedRestore(Input) == EOWSGroundedMovementResult::Success);

		Input.bHasPendingMovementModeChange = true;
		TestTrue(TEXT("Pending mode change rejects restore"),
			ValidateGroundedRestore(Input) ==
				EOWSGroundedMovementResult::PendingMovementModeChange);
		Input.bHasPendingMovementModeChange = false;

		Input.bHasCheckpoint = false;
		TestTrue(TEXT("Missing checkpoint fails closed"),
			ValidateGroundedRestore(Input) ==
				EOWSGroundedMovementResult::MissingCheckpoint);
		Input.bHasCheckpoint = true;

		Input.bCheckpointTransformIsValid = false;
		TestTrue(TEXT("Invalid checkpoint transform fails closed"),
			ValidateGroundedRestore(Input) ==
				EOWSGroundedMovementResult::InvalidCheckpointTransform);

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
