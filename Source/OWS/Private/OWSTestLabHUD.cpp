#include "OWSTestLabHUD.h"

#include "Core/Items/SigilItemInfo.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "OWSMoverComparisonPawn.h"
#include "OWSPrototypeAttributeSet.h"
#include "OWSPrototypeCharacter.h"
#include "OWSPrototypeInventoryComponents.h"
#include "OWSTestLabPlayerController.h"
#include "OWSStockVehicleInteractionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "SigilInventoryTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSTestLabHUD)

namespace OWS::TestLab::HUD
{
	constexpr float PanelX = 20.0f;
	constexpr float PanelY = 20.0f;
	constexpr float TextInset = 14.0f;
	constexpr float StandardPanelWidth = 980.0f;
	constexpr float StandardPanelHeight = 410.0f;
	constexpr float MoverPanelHeight = 250.0f;
}

void AOWSTestLabHUD::DrawHUD()
{
	Super::DrawHUD();

	if (Canvas == nullptr || GEngine == nullptr || PlayerOwner == nullptr)
	{
		return;
	}

	using namespace OWS::TestLab::HUD;

	const AOWSTestLabPlayerController* LabController =
		Cast<AOWSTestLabPlayerController>(PlayerOwner);
	if (LabController == nullptr)
	{
		return;
	}

	const AOWSMoverComparisonPawn* MoverPawn =
		Cast<AOWSMoverComparisonPawn>(LabController->GetPawn());
	const bool bMoverMode = LabController->IsInMoverMode() || MoverPawn != nullptr;
	const float PanelWidth = FMath::Min(StandardPanelWidth, Canvas->ClipX - PanelX * 2.0f);
	const float PanelHeight = bMoverMode ? MoverPanelHeight : StandardPanelHeight;

	FCanvasTileItem Background(
		FVector2D(PanelX, PanelY),
		FVector2D(FMath::Max(PanelWidth, 300.0f), PanelHeight),
		FLinearColor(0.015f, 0.025f, 0.045f, 0.86f));
	Background.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Background);

	const float TextX = PanelX + TextInset;
	float Y = PanelY + 10.0f;
	DrawLabLine(
		bMoverMode
			? TEXT("OWS TEST LAB  |  ISOLATED MOVER MODE")
			: TEXT("OWS SYSTEMS TEST LAB"),
		TextX,
		Y,
		FLinearColor(0.45f, 0.9f, 1.0f),
		true);

	if (bMoverMode)
	{
		DrawLabLine(
			TEXT("Movement authority: Epic Mover (separate from the standard Character pawn)"),
			TextX,
			Y,
			FLinearColor(0.75f, 0.9f, 1.0f));

		const FText CheckpointResult = MoverPawn != nullptr
			? UEnum::GetDisplayValueAsText(MoverPawn->GetLastCheckpointResult())
			: FText::FromString(TEXT("Waiting for Mover pawn"));
		DrawLabLine(
			FString::Printf(TEXT("Checkpoint status: %s"), *CheckpointResult.ToString()),
			TextX,
			Y,
			FLinearColor(0.75f, 1.0f, 0.75f));
		DrawLabLine(
			TEXT("KEYBOARD: WASD move  |  Mouse look  |  Space jump  |  F5 save  |  F9 load"),
			TextX,
			Y,
			FLinearColor::White);
		DrawLabLine(
			TEXT("GAMEPAD: Left Stick move  |  Right Stick look  |  A / Cross jump"),
			TextX,
			Y,
			FLinearColor::White);
		DrawLabLine(
			TEXT("M return to the standard lab"),
			TextX,
			Y,
			FLinearColor(1.0f, 0.9f, 0.45f));
		DrawLabLine(
			TEXT("Checkpoint scope: Walking/grounded only; saving in midair should be rejected."),
			TextX,
			Y,
			FLinearColor(0.9f, 0.75f, 0.45f));
		return;
	}

	const AOWSPrototypeCharacter* HomeCharacter =
		LabController->GetHomeCharacterForHUD();
	const APawn* Vehicle =
		LabController->GetCurrentLabVehicle();

	if (Vehicle != nullptr)
	{
		const UOWSStockVehicleInteractionComponent* Interaction =
			Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
		const TCHAR* VehicleSystem = Interaction != nullptr &&
			Interaction->GetKinetiForgeDriveAssembly() != nullptr
			? TEXT("STOCK KINETIFORGE RWD SPORTS CAR")
			: TEXT("EPIC STOCK CHAOS MODULAR VEHICLE");
		DrawLabLine(
			FString::Printf(
				TEXT("Active test: %s  |  Seat: %s  |  %s  |  %s"),
				VehicleSystem,
				*LabController->GetCurrentVehicleSeatId().ToString(),
				LabController->IsCurrentVehicleDriver()
					? TEXT("DRIVER")
					: TEXT("PASSENGER"),
				Interaction != nullptr &&
					(Interaction->HasControlSeatOccupant() || Vehicle->GetController() != nullptr)
					? TEXT("DRIVER CONTROLLED")
					: TEXT("NO DRIVER")),
			TextX,
			Y,
			FLinearColor(1.0f, 0.82f, 0.38f));
	}
	else
	{
		DrawLabLine(
			TEXT("Active test: ON FOOT  |  Standard Character Movement"),
			TextX,
			Y,
			FLinearColor(0.75f, 1.0f, 0.75f));
	}
	DrawLabLine(
		FString::Printf(
			TEXT("Vehicle interaction: %s"),
			*LabController->GetVehicleInteractionStatus()),
		TextX,
		Y,
		FLinearColor(0.75f, 0.9f, 1.0f));

	if (HomeCharacter == nullptr)
	{
		DrawLabLine(
			TEXT("Standard character state is unavailable."),
			TextX,
			Y,
			FLinearColor(1.0f, 0.35f, 0.35f));
	}
	else
	{
		DrawLabLine(HomeCharacter->GetPrototypeStatus(), TextX, Y, FLinearColor::White);

		if (const UOWSPrototypeAttributeSet* Attributes =
			HomeCharacter->GetPrototypeAttributes())
		{
			DrawLabLine(
				FString::Printf(
					TEXT("GAS attributes: Health %.0f / %.0f  |  Attack Power %.0f"),
					Attributes->GetHealth(),
					Attributes->GetMaxHealth(),
					Attributes->GetAttackPower()),
				TextX,
				Y,
				FLinearColor(0.65f, 1.0f, 0.65f));
		}

		int32 MainItemCount = 0;
		int32 EquippedItemCount = 0;
		if (UOWSPrototypeInventorySystemComponent* Inventory =
			HomeCharacter->FindComponentByClass<UOWSPrototypeInventorySystemComponent>())
		{
			TArray<FSigilItemInfo> MainItems;
			TArray<FSigilItemInfo> EquippedItems;
			Inventory->GetAllItemInfosInCollection(SigilCollectionTags::Main, MainItems);
			Inventory->GetAllItemInfosInCollection(
				SigilCollectionTags::Equipped,
				EquippedItems);

			for (const FSigilItemInfo& Item : MainItems)
			{
				MainItemCount += FMath::Max(Item.Amount, 0);
			}
			for (const FSigilItemInfo& Item : EquippedItems)
			{
				EquippedItemCount += FMath::Max(Item.Amount, 0);
			}
		}

		DrawLabLine(
			FString::Printf(
				TEXT("Sigil inventory: %d Main  |  %d Equipped  |  Save scalar %.1f"),
				MainItemCount,
				EquippedItemCount,
				HomeCharacter->GetPrototypeScalar()),
			TextX,
			Y,
			FLinearColor(1.0f, 0.9f, 0.5f));
	}

	Y += 4.0f;
	if (Vehicle != nullptr)
	{
		if (LabController->IsCurrentVehicleDriver())
		{
			const UOWSStockVehicleInteractionComponent* Interaction =
				Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
			const UPrimitiveComponent* VehicleBody = Interaction != nullptr
				? Interaction->GetVehiclePhysicsBody()
				: nullptr;
			const float SpeedCmPerSecond = VehicleBody != nullptr
				? VehicleBody->GetPhysicsLinearVelocity().Size()
				: 0.0f;
			DrawLabLine(
				FString::Printf(
					TEXT("SPEED: %.1f km/h  |  %.1f mph"),
					SpeedCmPerSecond * 0.036f,
					SpeedCmPerSecond * 0.0223694f),
				TextX,
				Y,
				FLinearColor(0.45f, 1.0f, 0.55f));
			DrawLabLine(
				TEXT("KEYBOARD: W accelerate  |  S brake/reverse  |  A/D steer  |  Space handbrake"),
				TextX,
				Y,
				FLinearColor::White);
			DrawLabLine(
				TEXT("FORZA GAMEPAD: RT accelerate  |  LT brake/reverse  |  Left Stick steer"),
				TextX,
				Y,
				FLinearColor::White);
			DrawLabLine(
				TEXT("GAMEPAD: Right Stick camera  |  A / Cross handbrake  |  B / Circle exit when stopped"),
				TextX,
				Y,
				FLinearColor::White);
			DrawLabLine(
				Interaction != nullptr && Interaction->GetKinetiForgeDriveAssembly() != nullptr
					? TEXT("Stock KinetiForge physics  |  OWS Forza-style input/park lifecycle only  |  F also exits")
					: TEXT("Epic stock Chaos physics  |  OWS Forza-style input/park lifecycle only  |  F also exits"),
				TextX,
				Y,
				FLinearColor(0.75f, 0.9f, 1.0f));
		}
		else
		{
			DrawLabLine(
				(Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>() != nullptr &&
					Vehicle->FindComponentByClass<UOWSStockVehicleInteractionComponent>()->HasControlSeatOccupant()) ||
					Vehicle->GetController() != nullptr
					? TEXT("Passenger seat occupied  |  Vehicle controls remain with the driver  |  F exit")
					: TEXT("Passenger seat occupied  |  No driver; vehicle remains parked  |  F exit"),
				TextX,
				Y,
				FLinearColor(0.75f, 0.9f, 1.0f));
		}
		DrawInteractionPrompt(
			TEXT("B / CIRCLE  OR  F    EXIT VEHICLE"),
			FLinearColor(0.35f, 0.95f, 1.0f));
	}
	else
	{
		DrawLabLine(
			TEXT("KEYBOARD: WASD move  |  Mouse look  |  Space jump  |  F Activate"),
			TextX,
			Y,
			FLinearColor::White);
		DrawLabLine(
			TEXT("GAMEPAD: Left Stick move  |  Right Stick look  |  A / Cross jump  |  X / Square Activate"),
			TextX,
			Y,
			FLinearColor::White);
		DrawLabLine(
			TEXT("E equip sword  |  Q unequip  |  U potion  |  P granted GAS pulse"),
			TextX,
			Y,
			FLinearColor::White);
		DrawLabLine(
			TEXT("F5 grounded save  |  F9 load  |  PageUp/PageDown scalar  |  M Mover lab"),
			TextX,
			Y,
			FLinearColor(0.75f, 0.9f, 1.0f));

		DrawLabLine(
			FString::Printf(
				TEXT("Vehicle auto-placement: %s  |  OFF rejects the faced door if its primary seat is occupied or blocked"),
				LabController->IsVehicleAutoPlacementEnabled()
					? TEXT("ON")
					: TEXT("OFF")),
			TextX,
			Y,
			FLinearColor(0.85f, 0.8f, 1.0f));

		const float VehicleDistance = LabController->GetNearestEnterableVehicleDistance();
		if (VehicleDistance >= 0.0f)
		{
			DrawLabLine(
				FString::Printf(
					TEXT("AVAILABLE VEHICLE SEAT: %.1f m  |  F or X / Square to Activate"),
					VehicleDistance / 100.0f),
				TextX,
				Y,
				FLinearColor(0.3f, 1.0f, 0.4f));
			DrawInteractionPrompt(
				TEXT("F  /  X-SQUARE    ACTIVATE VEHICLE"),
				FLinearColor(0.3f, 1.0f, 0.4f));
		}
	}

	DrawLabLine(
		TEXT("Save proof: grounded transform + scalar only; inventory, equipment, and GAS are not persisted."),
		TextX,
		Y,
		FLinearColor(0.95f, 0.65f, 0.4f));
}

void AOWSTestLabHUD::DrawLabLine(
	const FString& Text,
	const float X,
	float& Y,
	const FLinearColor& Color,
	const bool bTitle) const
{
	if (Canvas == nullptr || GEngine == nullptr)
	{
		return;
	}

	UFont* Font = bTitle ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
	Y += bTitle ? 38.0f : 28.0f;
}

void AOWSTestLabHUD::DrawInteractionPrompt(
	const FString& Text,
	const FLinearColor& Color) const
{
	if (Canvas == nullptr || GEngine == nullptr)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, Text, TextWidth, TextHeight);
	const FVector2D Padding(30.0f, 18.0f);
	const FVector2D Position(
		(Canvas->ClipX - TextWidth) * 0.5f,
		Canvas->ClipY - TextHeight - 95.0f);

	FCanvasTileItem Background(
		Position - Padding,
		FVector2D(TextWidth, TextHeight) + Padding * 2.0f,
		FLinearColor(0.01f, 0.02f, 0.035f, 0.9f));
	Background.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Background);

	FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, Color);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
}
