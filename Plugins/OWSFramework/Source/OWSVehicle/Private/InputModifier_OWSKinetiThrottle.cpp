// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Throttle shaping is adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#include "InputModifier_OWSKinetiThrottle.h"

#include "Curves/CurveFloat.h"
#include "OWSVehicleAlgorithms.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InputModifier_OWSKinetiThrottle)

FInputActionValue UInputModifier_OWSKinetiThrottle::ModifyRaw_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue CurrentValue,
	const float DeltaTime)
{
	PreviousValue = OWS::Vehicle::StepKinetiThrottle(
		PreviousValue,
		CurrentValue.Get<float>(),
		DeltaTime,
		InterpSpeed);

	const FRichCurve* RichResponseCurve = ResponseCurve != nullptr
		? &ResponseCurve->FloatCurve
		: nullptr;
	const float FinalValue = OWS::Vehicle::ApplyKinetiThrottleResponse(
		PreviousValue,
		RichResponseCurve);

	return FInputActionValue(FinalValue);
}

void UInputModifier_OWSKinetiThrottle::ReceiveModifierReinstanced_Implementation(
	const UInputModifier* OldModifier)
{
	Super::ReceiveModifierReinstanced_Implementation(OldModifier);

	if (const UInputModifier_OWSKinetiThrottle* OldThrottleModifier =
		Cast<UInputModifier_OWSKinetiThrottle>(OldModifier))
	{
		PreviousValue = OldThrottleModifier->PreviousValue;
	}
}
