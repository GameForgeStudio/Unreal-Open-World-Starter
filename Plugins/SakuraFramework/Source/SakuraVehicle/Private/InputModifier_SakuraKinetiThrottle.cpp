// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Throttle shaping is adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#include "InputModifier_SakuraKinetiThrottle.h"

#include "Curves/CurveFloat.h"
#include "SakuraVehicleAlgorithms.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InputModifier_SakuraKinetiThrottle)

FInputActionValue UInputModifier_SakuraKinetiThrottle::ModifyRaw_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue CurrentValue,
	const float DeltaTime)
{
	PreviousValue = Sakura::Vehicle::StepKinetiThrottle(
		PreviousValue,
		CurrentValue.Get<float>(),
		DeltaTime,
		InterpSpeed);

	const FRichCurve* RichResponseCurve = ResponseCurve != nullptr
		? &ResponseCurve->FloatCurve
		: nullptr;
	const float FinalValue = Sakura::Vehicle::ApplyKinetiThrottleResponse(
		PreviousValue,
		RichResponseCurve);

	return FInputActionValue(FinalValue);
}

void UInputModifier_SakuraKinetiThrottle::ReceiveModifierReinstanced_Implementation(
	const UInputModifier* OldModifier)
{
	Super::ReceiveModifierReinstanced_Implementation(OldModifier);

	if (const UInputModifier_SakuraKinetiThrottle* OldThrottleModifier =
		Cast<UInputModifier_SakuraKinetiThrottle>(OldModifier))
	{
		PreviousValue = OldThrottleModifier->PreviousValue;
	}
}
