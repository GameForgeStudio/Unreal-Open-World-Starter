#pragma once

#include "Blueprint/UserWidget.h"
#include "OWSControllerHotbarComponent.h"
#include "OWSHotbarWidget.generated.h"

/** Native fallback presentation for the generic OWS cross hotbar. */
UCLASS()
class OWSCORE_API UOWSHotbarWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(EOWSHotbarCellShape InShape, float InCellSize, FVector2D InScreenPosition);
	void SetLayer(EOWSHotbarLayer InLayer);
	void SetPressedSlot(EOWSHotbarInput Input);

protected:
	virtual void NativeConstruct() override;

private:
	void BuildCross();

	EOWSHotbarCellShape Shape = EOWSHotbarCellShape::Square;
	float CellSize = 48.0f;
	FVector2D ScreenPosition = FVector2D(0.5f, 0.78f);
};
