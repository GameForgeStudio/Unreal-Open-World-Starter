// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Portions adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#pragma once

#include "CoreMinimal.h"

struct FRichCurve;

namespace Sakura::Vehicle
{
	/** KinetiForge uses 64 uniformly spaced positive-slip samples. */
	inline constexpr int32 KinetiPositiveSlipSampleCount = 64;

	/**
	 * Advance an unsigned throttle value using KinetiForge's constant interpolation rule.
	 * InterpSpeed.X is used for every non-zero target; InterpSpeed.Y is used when the
	 * target is zero. A non-positive selected speed snaps directly to the target.
	 */
	SAKURAVEHICLE_API float StepKinetiThrottle(
		float Current,
		float Target,
		float DeltaTime,
		const FVector2f& InterpSpeed);

	/** Apply the optional response curve and the original [0, 1] output clamp. */
	SAKURAVEHICLE_API float ApplyKinetiThrottleResponse(
		float SmoothedValue,
		const FRichCurve* ResponseCurve);

	/**
	 * Bake a normalized [0, 1] lateral-slip curve into uniformly spaced, symmetric
	 * signed-angle samples suitable for Chaos::FGraph. Negative curve values are
	 * clipped to zero, matching KinetiForge's tire LUT construction.
	 */
	SAKURAVEHICLE_API void BuildSymmetricLateralSlipSamples(
		const FRichCurve& NormalizedCurve,
		float NormalizedSlipDegrees,
		float ForceScale,
		TArray<FVector2f>& OutSamples);
}
