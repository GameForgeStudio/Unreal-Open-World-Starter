// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "OWSVehicleAlgorithms.h"

#include "Curves/RichCurve.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSKinetiThrottleAlgorithmsTest,
	"OWS.Vehicle.Input.KinetiThrottleAlgorithms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOWSKinetiThrottleAlgorithmsTest::RunTest(const FString& Parameters)
{
	const FVector2f InterpSpeed(5.0f, 2.0f);

	TestTrue(
		TEXT("A non-zero target uses InterpSpeed.X"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::StepKinetiThrottle(0.0f, 1.0f, 0.1f, InterpSpeed),
			0.5f));
	TestTrue(
		TEXT("A zero target uses InterpSpeed.Y"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::StepKinetiThrottle(0.8f, 0.0f, 0.1f, InterpSpeed),
			0.6f));
	TestTrue(
		TEXT("A lower but non-zero target still uses InterpSpeed.X"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::StepKinetiThrottle(0.8f, 0.5f, 0.02f, InterpSpeed),
			0.7f,
			KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("A non-positive selected speed snaps to target"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::StepKinetiThrottle(0.0f, 0.75f, 0.1f, FVector2f::ZeroVector),
			0.75f));

	FRichCurve ResponseCurve;
	ResponseCurve.AddKey(0.0f, -0.25f);
	ResponseCurve.AddKey(1.0f, 1.25f);
	TestTrue(
		TEXT("Response curve output is clamped low"),
		FMath::IsNearlyZero(OWS::Vehicle::ApplyKinetiThrottleResponse(0.0f, &ResponseCurve)));
	TestTrue(
		TEXT("Response curve output is clamped high"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::ApplyKinetiThrottleResponse(1.0f, &ResponseCurve),
			1.0f));
	TestTrue(
		TEXT("Null response curve returns the clamped smoothed value"),
		FMath::IsNearlyEqual(
			OWS::Vehicle::ApplyKinetiThrottleResponse(0.4f, nullptr),
			0.4f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSLateralSlipSamplesTest,
	"OWS.Vehicle.Wheel.KinetiLateralSlipSamples",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOWSLateralSlipSamplesTest::RunTest(const FString& Parameters)
{
	FRichCurve LinearCurve;
	const FKeyHandle StartKey = LinearCurve.AddKey(0.0f, 0.0f);
	const FKeyHandle EndKey = LinearCurve.AddKey(1.0f, 1.0f);
	LinearCurve.SetKeyInterpMode(StartKey, RCIM_Linear);
	LinearCurve.SetKeyInterpMode(EndKey, RCIM_Linear);

	TArray<FVector2f> Samples;
	OWS::Vehicle::BuildSymmetricLateralSlipSamples(
		LinearCurve,
		10.0f,
		2.0f,
		Samples);

	const int32 ExpectedSampleCount = 2 * OWS::Vehicle::KinetiPositiveSlipSampleCount - 1;
	TestEqual(TEXT("The symmetric table contains 64 samples on each side with one shared zero"), Samples.Num(), ExpectedSampleCount);

	if (Samples.Num() == ExpectedSampleCount)
	{
		const int32 CenterIndex = OWS::Vehicle::KinetiPositiveSlipSampleCount - 1;
		TestTrue(TEXT("Negative endpoint angle is preserved"), FMath::IsNearlyEqual(Samples[0].X, -10.0f));
		TestTrue(TEXT("Positive endpoint angle is preserved"), FMath::IsNearlyEqual(Samples.Last().X, 10.0f));
		TestTrue(TEXT("Center sample is zero slip"), Samples[CenterIndex].IsNearlyZero());
		TestTrue(TEXT("Negative endpoint mirrors positive force"), FMath::IsNearlyEqual(Samples[0].Y, Samples.Last().Y));
		TestTrue(TEXT("Force scale is applied"), FMath::IsNearlyEqual(Samples.Last().Y, 2.0f));
	}

	FRichCurve NegativeCurve;
	NegativeCurve.AddKey(0.0f, -1.0f);
	OWS::Vehicle::BuildSymmetricLateralSlipSamples(
		NegativeCurve,
		10.0f,
		1.0f,
		Samples);

	const bool bAllForcesNonNegative = Samples.ContainsByPredicate(
		[](const FVector2f& Sample)
		{
			return Sample.Y < 0.0f;
		}) == false;
	TestTrue(TEXT("Negative grip values are clipped to zero"), bAllForcesNonNegative);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
