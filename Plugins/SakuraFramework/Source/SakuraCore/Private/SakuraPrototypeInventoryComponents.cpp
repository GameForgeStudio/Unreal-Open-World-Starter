#include "SakuraPrototypeInventoryComponents.h"

#include "Core/Collections/SigilItemCollection.h"
#include "Core/Collections/SigilItemSlotCollection.h"
#include "Core/Items/SigilItemDefinition.h"
#include "Core/Items/SigilItemInfo.h"
#include "Core/Fragments/SigilItemFragment_Equippable.h"
#include "SakuraGameplayTags.h"
#include "SakuraItemFragments.h"
#include "SigilInventoryTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const USigilItemCollectionDefinition* FindCollectionDefinition(
		const TArray<TObjectPtr<const USigilItemCollectionDefinition>>& Definitions,
		const FGameplayTag Tag)
	{
		const TObjectPtr<const USigilItemCollectionDefinition>* Match = Definitions.FindByPredicate(
			[Tag](const TObjectPtr<const USigilItemCollectionDefinition>& Definition)
			{
				return IsValid(Definition) && Definition->CollectionTag == Tag;
			});
		return Match ? Match->Get() : nullptr;
	}

	void AddCollectionIfMissing(
		TArray<TObjectPtr<const USigilItemCollectionDefinition>>& Definitions,
		const USigilItemCollectionDefinition* Candidate)
	{
		if (IsValid(Candidate) && !FindCollectionDefinition(Definitions, Candidate->CollectionTag))
		{
			Definitions.Add(Candidate);
		}
	}

}

USakuraPrototypeInventorySystemComponent::USakuraPrototypeInventorySystemComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInitializeOnBeginplay = false;

	static ConstructorHelpers::FObjectFinder<USigilItemCollectionDefinition> MainCollectionFinder(
		TEXT("/Game/SakuraPrototype/Data/DA_Collection_Main.DA_Collection_Main"));
	static ConstructorHelpers::FObjectFinder<USigilItemSlotCollectionDefinition> EquippedCollectionFinder(
		TEXT("/Game/SakuraPrototype/Data/DA_Collection_Equipped.DA_Collection_Equipped"));
	PrototypeMainCollection = MainCollectionFinder.Object;
	PrototypeEquippedCollection = EquippedCollectionFinder.Object;
	PrototypeSwordDefinition = TSoftObjectPtr<USigilItemDefinition>(FSoftObjectPath(
		TEXT("/Game/SakuraPrototype/Data/DA_Item_DemoSword.DA_Item_DemoSword")));
	PrototypePotionDefinition = TSoftObjectPtr<USigilItemDefinition>(FSoftObjectPath(
		TEXT("/Game/SakuraPrototype/Data/DA_Item_DemoPotion.DA_Item_DemoPotion")));
}

bool USakuraPrototypeInventorySystemComponent::PrepareAndValidatePrototypeConfiguration(FString& OutError)
{
	OutError.Reset();
	if (IsInventoryInitialized())
	{
		OutError = TEXT("Inventory is already initialized; prototype configuration must be prepared first.");
		return false;
	}

	AddCollectionIfMissing(CollectionDefinitions, PrototypeMainCollection);
	AddCollectionIfMissing(CollectionDefinitions, PrototypeEquippedCollection);

	const USigilItemCollectionDefinition* MainDefinition =
		FindCollectionDefinition(CollectionDefinitions, SigilCollectionTags::Main);
	const USigilItemSlotCollectionDefinition* EquippedDefinition = Cast<USigilItemSlotCollectionDefinition>(
		FindCollectionDefinition(CollectionDefinitions, SigilCollectionTags::Equipped));

	if (!MainDefinition)
	{
		OutError += TEXT("Assign a saved Main collection definition with tag Sigil.Inventory.Collection.Main. ");
	}
	if (!EquippedDefinition)
	{
		OutError += TEXT("Assign a saved slot collection definition with tag Sigil.Inventory.Collection.Equipped. ");
	}

	USigilItemDefinition* SwordDefinition = PrototypeSwordDefinition.LoadSynchronous();
	USigilItemDefinition* PotionDefinition = PrototypePotionDefinition.LoadSynchronous();
	if (bSeedPrototypeLoadout && !SwordDefinition)
	{
		OutError += TEXT("Assign a saved prototype sword item definition. ");
	}
	if (bSeedPrototypeLoadout && !PotionDefinition)
	{
		OutError += TEXT("Assign a saved prototype potion item definition. ");
	}

	if (SwordDefinition)
	{
		if (!SwordDefinition->bUnique || !SwordDefinition->ItemTags.HasTagExact(SakuraGameplayTags::Item_Weapon_DemoSword))
		{
			OutError += TEXT("The prototype sword must be Unique and carry Sakura.Item.Weapon.DemoSword. ");
		}
		if (!SwordDefinition->FindFragment<USigilItemFragment_Equippable>() ||
			!SwordDefinition->FindFragment<USakuraItemFragment_EquippedEffect>())
		{
			OutError += TEXT("The prototype sword needs Sakura Prototype Equippable and Sakura Equipped Gameplay Effect fragments. ");
		}

		if (EquippedDefinition)
		{
			const TArray<FSigilItemSlotDefinition>& Slots = EquippedDefinition->GetSlotDefinitions();
			const int32 MainHandIndex = Slots.IndexOfByPredicate(
				[](const FSigilItemSlotDefinition& Slot)
				{
					return Slot.Tag == SakuraGameplayTags::Inventory_Slot_MainHand;
				});
			int32 FirstSwordSlot = INDEX_NONE;
			for (int32 Index = 0; Index < Slots.Num(); ++Index)
			{
				if (!Slots[Index].TagQuery.IsEmpty() && Slots[Index].TagQuery.Matches(SwordDefinition->ItemTags))
				{
					FirstSwordSlot = Index;
					break;
				}
			}

			// Sigil 0.1.0's integer slot getter rejects index zero. Keeping the
			// usable hand slot later in the layout avoids that upstream defect.
			if (MainHandIndex <= 0 || FirstSwordSlot != MainHandIndex)
			{
				OutError += TEXT("Equipped must place MainHand after index 0 and make it the first slot whose non-empty query matches the sword. ");
			}
		}
	}

	if (PotionDefinition)
	{
		if (!PotionDefinition->ItemTags.HasTagExact(SakuraGameplayTags::Item_Consumable_DemoPotion) ||
			!PotionDefinition->FindFragment<USakuraItemFragment_UseEffect>())
		{
			OutError += TEXT("The prototype potion needs Sakura.Item.Consumable.DemoPotion and a Sakura Use Gameplay Effect fragment. ");
		}
	}

	if (!OutError.IsEmpty())
	{
		return false;
	}

	return true;
}

bool USakuraPrototypeInventorySystemComponent::SeedMissingPrototypeItems(FString& OutError)
{
	OutError.Reset();
	if (!bSeedPrototypeLoadout)
	{
		return true;
	}
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsInventoryInitialized())
	{
		OutError = TEXT("Prototype items can only be seeded by an initialized authority inventory.");
		return false;
	}

	auto AddMissing = [this, &OutError](
		const TSoftObjectPtr<USigilItemDefinition>& Definition,
		const int32 Amount,
		const TCHAR* Label)
	{
		if (Definition.IsNull())
		{
			OutError += FString::Printf(TEXT("%s definition is missing. "), Label);
			return;
		}
		if (HasEnoughItem(Definition, 1))
		{
			return;
		}

		const FSigilItemInfo Added = AddItemByDefinition(
			SigilCollectionTags::Main,
			Definition,
			FMath::Max(1, Amount));
		if (!Added.IsValid() || Added.Amount < 1)
		{
			OutError += FString::Printf(TEXT("Sigil could not add the %s. "), Label);
		}
	};

	AddMissing(PrototypeSwordDefinition, 1, TEXT("prototype sword"));
	AddMissing(PrototypePotionDefinition, PrototypePotionCount, TEXT("prototype potion"));
	return OutError.IsEmpty();
}

USakuraPrototypeEquipmentSystemComponent::USakuraPrototypeEquipmentSystemComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInitializeOnBeginPlay = false;
	TargetCollectionTag = SigilCollectionTags::Equipped;
}
