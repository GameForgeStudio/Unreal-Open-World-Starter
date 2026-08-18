// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Throttle shaping is adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "InputModifier_SakuraKinetiThrottle.generated.h"

class UCurveFloat;

/**
 * Shapes an Axis1D throttle before it enters Chaos Modular Vehicle's input history.
 * Add this modifier to the throttle Input Action, then pass that action's resulting
 * value to USakuraChaosVehicleInputRouterComponent::SetShapedThrottleInput.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Sakura Kineti Throttle Shape"))
class SAKURAVEHICLE_API UInputModifier_SakuraKinetiThrottle : public UInputModifier
{
	GENERATED_BODY()

public:
	/** X: rate for any non-zero target. Y: rate when returning to zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Throttle", meta = (ClampMin = "0.0"))
	FVector2f InterpSpeed = FVector2f(5.0f, 5.0f);

	/** Optional normalized response curve evaluated after constant interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Throttle")
	TObjectPtr<UCurveFloat> ResponseCurve = nullptr;

	/** Clear accumulated interpolation state when control ownership ends. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Vehicle|Throttle")
	void ResetThrottleState() { PreviousValue = 0.0f; }

protected:
	virtual FInputActionValue ModifyRaw_Implementation(
		const UEnhancedPlayerInput* PlayerInput,
		FInputActionValue CurrentValue,
		float DeltaTime) override;

	virtual void ReceiveModifierReinstanced_Implementation(const UInputModifier* OldModifier) override;

private:
	float PreviousValue = 0.0f;
};
