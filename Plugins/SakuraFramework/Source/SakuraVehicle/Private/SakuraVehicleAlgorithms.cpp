// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Portions adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#include "SakuraVehicleAlgorithms.h"

#include "Curves/RichCurve.h"

namespace Sakura::Vehicle
{
	float StepKinetiThrottle(
		const float Current,
		const float Target,
		const float DeltaTime,
		const FVector2f& InterpSpeed)
	{
		const float ClampedTarget = FMath::Clamp(Target, 0.0f, 1.0f);
		const float SelectedSpeed = ClampedTarget < SMALL_NUMBER ? InterpSpeed.Y : InterpSpeed.X;

		return SelectedSpeed <= 0.0f
			? ClampedTarget
			: FMath::FInterpConstantTo(Current, ClampedTarget, DeltaTime, SelectedSpeed);
	}

	float ApplyKinetiThrottleResponse(
		const float SmoothedValue,
		const FRichCurve* ResponseCurve)
	{
		const float ResponseValue = ResponseCurve != nullptr
			? ResponseCurve->Eval(SmoothedValue)
			: SmoothedValue;

		return FMath::Clamp(ResponseValue, 0.0f, 1.0f);
	}

	void BuildSymmetricLateralSlipSamples(
		const FRichCurve& NormalizedCurve,
		const float NormalizedSlipDegrees,
		const float ForceScale,
		TArray<FVector2f>& OutSamples)
	{
		OutSamples.Reset();

		if (NormalizedSlipDegrees <= SMALL_NUMBER)
		{
			return;
		}

		constexpr int32 LastPositiveSampleIndex = KinetiPositiveSlipSampleCount - 1;
		OutSamples.Reserve(2 * KinetiPositiveSlipSampleCount - 1);

		const float NonNegativeForceScale = FMath::Max(ForceScale, 0.0f);
		for (int32 SignedSampleIndex = -LastPositiveSampleIndex;
			SignedSampleIndex <= LastPositiveSampleIndex;
			++SignedSampleIndex)
		{
			const float SignedNormalizedSlip =
				static_cast<float>(SignedSampleIndex) / static_cast<float>(LastPositiveSampleIndex);
			const float NormalizedSlip = FMath::Abs(SignedNormalizedSlip);
			const float NonNegativeCurveValue = FMath::Max(NormalizedCurve.Eval(NormalizedSlip), 0.0f);

			OutSamples.Emplace(
				SignedNormalizedSlip * NormalizedSlipDegrees,
				NonNegativeCurveValue * NonNegativeForceScale);
		}
	}
}
