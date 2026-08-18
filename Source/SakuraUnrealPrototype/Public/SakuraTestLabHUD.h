#pragma once

#include "GameFramework/HUD.h"

#include "SakuraTestLabHUD.generated.h"

/**
 * Persistent native test-lab readout.
 *
 * Unlike the original prototype HUD, this HUD gets the standard character from
 * the lab PlayerController. Inventory, GAS, and checkpoint state therefore stay
 * visible while that controller temporarily possesses a vehicle.
 */
UCLASS()
class SAKURAUNREALPROTOTYPE_API ASakuraTestLabHUD final : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawLabLine(
		const FString& Text,
		float X,
		float& Y,
		const FLinearColor& Color,
		bool bTitle = false) const;
	void DrawInteractionPrompt(const FString& Text, const FLinearColor& Color) const;
};
