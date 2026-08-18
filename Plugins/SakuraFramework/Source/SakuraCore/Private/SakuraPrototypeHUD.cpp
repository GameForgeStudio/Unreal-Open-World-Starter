#include "SakuraPrototypeHUD.h"

#include "Core/Items/SigilItemInfo.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "SakuraPrototypeAttributeSet.h"
#include "SakuraPrototypeCharacter.h"
#include "SakuraPrototypeInventoryComponents.h"
#include "SigilInventoryTags.h"

void ASakuraPrototypeHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	const ASakuraPrototypeCharacter* Character = Cast<ASakuraPrototypeCharacter>(PlayerOwner->GetPawn());
	if (!Character)
	{
		return;
	}

	float Y = 24.0f;
	DrawStatusLine(TEXT("SAKURA FULL-STACK PROTOTYPE"), Y, FLinearColor(0.55f, 0.9f, 1.0f));
	DrawStatusLine(Character->GetPrototypeStatus(), Y, FLinearColor::White);

	if (const USakuraPrototypeAttributeSet* Attributes = Character->GetPrototypeAttributes())
	{
		DrawStatusLine(
			FString::Printf(TEXT("Health %.0f / %.0f    Attack Power %.0f"),
				Attributes->GetHealth(), Attributes->GetMaxHealth(), Attributes->GetAttackPower()),
			Y,
			FLinearColor(0.7f, 1.0f, 0.7f));
	}

	int32 MainItemCount = 0;
	int32 EquippedItemCount = 0;
	if (USakuraPrototypeInventorySystemComponent* Inventory =
		Character->FindComponentByClass<USakuraPrototypeInventorySystemComponent>())
	{
		TArray<FSigilItemInfo> MainItems;
		TArray<FSigilItemInfo> EquippedItems;
		Inventory->GetAllItemInfosInCollection(SigilCollectionTags::Main, MainItems);
		Inventory->GetAllItemInfosInCollection(SigilCollectionTags::Equipped, EquippedItems);
		for (const FSigilItemInfo& Item : MainItems)
		{
			MainItemCount += FMath::Max(Item.Amount, 0);
		}
		for (const FSigilItemInfo& Item : EquippedItems)
		{
			EquippedItemCount += FMath::Max(Item.Amount, 0);
		}
	}

	DrawStatusLine(
		FString::Printf(TEXT("Inventory: %d in Main, %d equipped    Saved scalar: %.1f"),
			MainItemCount, EquippedItemCount, Character->GetPrototypeScalar()),
		Y,
		FLinearColor(1.0f, 0.9f, 0.55f));
	Y += 8.0f;
	DrawStatusLine(TEXT("WASD move | Mouse look | Space jump"), Y, FLinearColor(0.8f, 0.8f, 0.8f));
	DrawStatusLine(TEXT("E equip | Q unequip | U potion | P granted ability"), Y, FLinearColor(0.8f, 0.8f, 0.8f));
	DrawStatusLine(TEXT("F5 grounded save | F9 load | PageUp/PageDown scalar"), Y, FLinearColor(0.8f, 0.8f, 0.8f));
}

void ASakuraPrototypeHUD::DrawStatusLine(
	const FString& Text,
	float& Y,
	const FLinearColor& Color) const
{
	if (!Canvas || !GEngine)
	{
		return;
	}

	FCanvasTextItem Item(FVector2D(24.0f, Y), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	Item.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Item);
	Y += 22.0f;
}
