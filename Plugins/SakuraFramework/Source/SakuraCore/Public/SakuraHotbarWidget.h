#pragma once

#include "Blueprint/UserWidget.h"
#include "SakuraControllerHotbarComponent.h"
#include "SakuraHotbarWidget.generated.h"

/** Native fallback presentation for the generic OWS cross hotbar. */
UCLASS()
class SAKURACORE_API USakuraHotbarWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(ESakuraHotbarCellShape InShape, float InCellSize, FVector2D InScreenPosition);
	void SetLayer(ESakuraHotbarLayer InLayer);
	void SetPressedSlot(ESakuraHotbarInput Input);

protected:
	virtual void NativeConstruct() override;

private:
	void BuildCross();

	ESakuraHotbarCellShape Shape = ESakuraHotbarCellShape::Square;
	float CellSize = 48.0f;
	FVector2D ScreenPosition = FVector2D(0.5f, 0.78f);
};
