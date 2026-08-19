#include "OWSHotbarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Styling/SlateBrush.h"

void UOWSHotbarWidget::Configure(
	const EOWSHotbarCellShape InShape,
	const float InCellSize,
	const FVector2D InScreenPosition)
{
	Shape = InShape;
	CellSize = InCellSize;
	ScreenPosition = InScreenPosition;
	BuildCross();
}

void UOWSHotbarWidget::SetLayer(const EOWSHotbarLayer InLayer)
{
	SetVisibility(InLayer == EOWSHotbarLayer::None
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible);
}

void UOWSHotbarWidget::SetPressedSlot(const EOWSHotbarInput Input)
{
	if (!WidgetTree) return;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		if (UBorder* Border = Cast<UBorder>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("OWSHotbarCell%d"), Index)))))
		{
			Border->SetBrushColor(Index == static_cast<int32>(Input)
				? FLinearColor(0.18f, 0.95f, 0.48f, 0.95f)
				: FLinearColor(0.22f, 0.66f, 1.0f, 0.85f));
		}
	}
}

void UOWSHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildCross();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOWSHotbarWidget::BuildCross()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OWSHotbarCanvas"));
	WidgetTree->RootWidget = Canvas;
	const float Half = CellSize * 0.5f;
	const float Gap = CellSize * 1.15f;
	const FVector2D DPadCenter(-Gap * 1.8f, 0.0f);
	const FVector2D FaceCenter(Gap * 1.8f, 0.0f);
	const FVector2D Offsets[] = {
		FVector2D(0, -Gap), FVector2D(Gap, 0), FVector2D(0, Gap), FVector2D(-Gap, 0),
		FVector2D(0, -Gap), FVector2D(Gap, 0), FVector2D(0, Gap), FVector2D(-Gap, 0)};

	for (int32 Index = 0; Index < 8; ++Index)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SizeBox->SetWidthOverride(CellSize);
		SizeBox->SetHeightOverride(CellSize);
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), FName(*FString::Printf(TEXT("OWSHotbarCell%d"), Index)));
		FSlateBrush Brush;
		Brush.DrawAs = Shape == EOWSHotbarCellShape::Circle ? ESlateBrushDrawType::RoundedBox : ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(FLinearColor(0.06f, 0.08f, 0.12f, 0.78f));
		if (Shape == EOWSHotbarCellShape::Circle)
		{
			Brush.OutlineSettings.CornerRadii = FVector4(CellSize * 0.5f, CellSize * 0.5f, CellSize * 0.5f, CellSize * 0.5f);
		}
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor(0.22f, 0.66f, 1.0f, 0.85f));
		SizeBox->SetContent(Border);
		const FVector2D Center = Index < 4 ? DPadCenter : FaceCenter;
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(SizeBox);
		CanvasSlot->SetAnchors(FAnchors(ScreenPosition.X, ScreenPosition.Y));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(Center + Offsets[Index] - FVector2D(Half, Half));
	}
}
