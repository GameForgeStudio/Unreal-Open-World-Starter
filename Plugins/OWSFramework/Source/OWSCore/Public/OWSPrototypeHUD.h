#pragma once

#include "GameFramework/HUD.h"
#include "OWSPrototypeHUD.generated.h"

/** Lightweight native HUD for the asset-light full-stack prototype. */
UCLASS()
class OWSCORE_API AOWSPrototypeHUD final : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawStatusLine(const FString& Text, float& Y, const FLinearColor& Color) const;
};
