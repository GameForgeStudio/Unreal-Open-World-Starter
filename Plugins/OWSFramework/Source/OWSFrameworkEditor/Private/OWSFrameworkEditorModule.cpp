#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/Collections/SigilItemCollection.h"
#include "Core/Collections/SigilItemSlotCollection.h"
#include "Core/Items/SigilItemDefinition.h"
#include "GameplayTagContainer.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "OWSGASEquipmentInstance.h"
#include "OWSGameplayTags.h"
#include "OWSItemFragments.h"
#include "OWSPrototypeCharacter.h"
#include "OWSPrototypeGameplayEffects.h"
#include "OWSPrototypeInventoryComponents.h"
#include "SigilInventoryTags.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogOWSFrameworkEditor, Log, All);

namespace OWS::PrototypeAssets
{
	struct FGeneratedAssetSpec
	{
		const TCHAR* PackageName;
		const TCHAR* AssetName;

		FString GetObjectPath() const
		{
			return FString::Printf(TEXT("%s.%s"), PackageName, AssetName);
		}
	};

	static constexpr FGeneratedAssetSpec MainCollectionSpec{
		TEXT("/Game/OWSPrototype/Data/DA_Collection_Main"),
		TEXT("DA_Collection_Main")};
	static constexpr FGeneratedAssetSpec EquippedCollectionSpec{
		TEXT("/Game/OWSPrototype/Data/DA_Collection_Equipped"),
		TEXT("DA_Collection_Equipped")};
	static constexpr FGeneratedAssetSpec SwordSpec{
		TEXT("/Game/OWSPrototype/Data/DA_Item_DemoSword"),
		TEXT("DA_Item_DemoSword")};
	static constexpr FGeneratedAssetSpec PotionSpec{
		TEXT("/Game/OWSPrototype/Data/DA_Item_DemoPotion"),
		TEXT("DA_Item_DemoPotion")};

	UObject* LoadOrCreateGeneratedAsset(
		const FGeneratedAssetSpec& Spec,
		UClass* RequiredClass,
		bool& bOutCreated)
	{
		bOutCreated = false;
		if (!RequiredClass)
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Cannot generate %s because its required class is unavailable."),
				*Spec.GetObjectPath());
			return nullptr;
		}

		const FString ObjectPath = Spec.GetObjectPath();
		if (UObject* Existing = StaticLoadObject(
			UObject::StaticClass(),
			nullptr,
			*ObjectPath,
			nullptr,
			LOAD_NoWarn | LOAD_Quiet))
		{
			if (Existing->GetClass() != RequiredClass)
			{
				UE_LOG(LogOWSFrameworkEditor, Error,
					TEXT("Refusing to replace generated asset %s: expected %s but found %s."),
					*ObjectPath,
					*RequiredClass->GetPathName(),
					*Existing->GetClass()->GetPathName());
				return nullptr;
			}

			Existing->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
			return Existing;
		}

		FString ExistingFilename;
		if (FPackageName::DoesPackageExist(Spec.PackageName, &ExistingFilename))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Refusing to overwrite package %s because it exists but does not contain %s."),
				Spec.PackageName,
				*ObjectPath);
			return nullptr;
		}

		UPackage* Package = CreatePackage(Spec.PackageName);
		if (!Package)
		{
			UE_LOG(LogOWSFrameworkEditor, Error, TEXT("Could not create package %s."), Spec.PackageName);
			return nullptr;
		}

		UObject* Asset = NewObject<UObject>(
			Package,
			RequiredClass,
			FName(Spec.AssetName),
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			UE_LOG(LogOWSFrameworkEditor, Error, TEXT("Could not create asset %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		bOutCreated = true;
		return Asset;
	}

	template <typename TFragment>
	TFragment* FindOrCreateNamedFragment(
		USigilItemDefinition* Definition,
		const FName FragmentName)
	{
		if (UObject* Existing = StaticFindObjectFast(UObject::StaticClass(), Definition, FragmentName))
		{
			if (Existing->GetClass() != TFragment::StaticClass())
			{
				UE_LOG(LogOWSFrameworkEditor, Error,
					TEXT("Refusing to replace fragment %s.%s: expected %s but found %s."),
					*Definition->GetPathName(),
					*FragmentName.ToString(),
					*TFragment::StaticClass()->GetPathName(),
					*Existing->GetClass()->GetPathName());
				return nullptr;
			}

			Existing->SetFlags(RF_Transactional);
			return CastChecked<TFragment>(Existing);
		}

		return NewObject<TFragment>(Definition, TFragment::StaticClass(), FragmentName, RF_Transactional);
	}

	bool ConfigureMainCollection(USigilItemCollectionDefinition* MainCollection)
	{
		MainCollection->Modify();
		MainCollection->CollectionTag = SigilCollectionTags::Main;
		MainCollection->Restrictions.Reset();
		MainCollection->OverflowOptions = FSigilItemOverflowOptions{};

		// Sigil 0.1.0 omits SIGILINVENTORY_API from the multistack definition
		// declaration. Reflection avoids linking against its unexported StaticClass
		// while still enforcing the two stable multistack defaults.
		FIntProperty* StackLimitProperty = FindFProperty<FIntProperty>(
			MainCollection->GetClass(),
			TEXT("DefaultStackSizeLimit"));
		FStructProperty* StackAttributeProperty = FindFProperty<FStructProperty>(
			MainCollection->GetClass(),
			TEXT("StackSizeLimitAttribute"));
		if (!StackLimitProperty || !StackAttributeProperty ||
			StackAttributeProperty->Struct != FGameplayTag::StaticStruct())
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Sigil multistack properties do not match the expected 0.1.0 API on %s."),
				*MainCollection->GetPathName());
			return false;
		}

		StackLimitProperty->SetPropertyValue_InContainer(MainCollection, 99);
		*StackAttributeProperty->ContainerPtrToValuePtr<FGameplayTag>(MainCollection) = FGameplayTag{};
		return true;
	}

	void ConfigureEquippedCollection(USigilItemSlotCollectionDefinition* EquippedCollection)
	{
		EquippedCollection->Modify();
		EquippedCollection->CollectionTag = SigilCollectionTags::Equipped;
		EquippedCollection->Restrictions.Reset();
		EquippedCollection->OverflowOptions = FSigilItemOverflowOptions{};
		EquippedCollection->bNewItemPriority = true;
		EquippedCollection->bTryGivePrevItemToNewItemCollection = true;
		EquippedCollection->SlotGroups.Reset();

		FSigilItemSlotDefinition ReservedSlot;
		ReservedSlot.Tag = SigilCollectionTags::Hidden;
		ReservedSlot.Name = FText::FromString(TEXT("Reserved"));
		ReservedSlot.Desc = FText::FromString(
			TEXT("Unusable compatibility slot reserved for Sigil 0.1.0 index zero."));
		ReservedSlot.TagQuery = FGameplayTagQuery{};

		FSigilItemSlotDefinition MainHandSlot;
		MainHandSlot.Tag = OWSGameplayTags::Inventory_Slot_MainHand;
		MainHandSlot.Name = FText::FromString(TEXT("Main Hand"));
		MainHandSlot.Desc = FText::FromString(TEXT("Prototype main-hand weapon slot."));
		MainHandSlot.TagQuery = FGameplayTagQuery::MakeQuery_MatchTag(
			OWSGameplayTags::Item_Weapon_DemoSword);

		EquippedCollection->SlotDefinitions.Reset(2);
		EquippedCollection->SlotDefinitions.Add(MoveTemp(ReservedSlot));
		EquippedCollection->SlotDefinitions.Add(MoveTemp(MainHandSlot));
	}

	bool ConfigureSword(USigilItemDefinition* Sword)
	{
		UOWSItemFragment_PrototypeEquippable* Equippable =
			FindOrCreateNamedFragment<UOWSItemFragment_PrototypeEquippable>(
				Sword,
				TEXT("OWSPrototypeEquippable"));
		UOWSItemFragment_EquippedEffect* EquippedEffect =
			FindOrCreateNamedFragment<UOWSItemFragment_EquippedEffect>(
				Sword,
				TEXT("OWSEquippedEffect"));
		if (!Equippable || !EquippedEffect)
		{
			return false;
		}

		Sword->Modify();
		Equippable->Modify();
		EquippedEffect->Modify();

		Sword->DisplayName = FText::FromString(TEXT("Demo Sword"));
		Sword->Description = FText::FromString(
			TEXT("OWS prototype weapon used to verify Sigil equipment and GAS integration."));
		Sword->Icon = nullptr;
		Sword->bUnique = true;
		Sword->ItemTags.Reset();
		Sword->ItemTags.AddTag(OWSGameplayTags::Item_Weapon_DemoSword);
		Sword->StaticFloatAttributes.Reset();
		Sword->StaticIntegerAttributes.Reset();

		Equippable->InstanceType = UOWSGASEquipmentInstance::StaticClass();
		Equippable->bAutoActivate = true;
		Equippable->ActorsToSpawn.Reset();
		EquippedEffect->EquippedEffect = UOWSPrototypeEquippedEffect::StaticClass();
		EquippedEffect->EffectLevel = 1.0f;

		Sword->Fragments.Reset(2);
		Sword->Fragments.Add(Equippable);
		Sword->Fragments.Add(EquippedEffect);
		return true;
	}

	bool ConfigurePotion(USigilItemDefinition* Potion)
	{
		UOWSItemFragment_UseEffect* UseEffect =
			FindOrCreateNamedFragment<UOWSItemFragment_UseEffect>(
				Potion,
				TEXT("OWSUseEffect"));
		if (!UseEffect)
		{
			return false;
		}

		Potion->Modify();
		UseEffect->Modify();

		Potion->DisplayName = FText::FromString(TEXT("Demo Potion"));
		Potion->Description = FText::FromString(
			TEXT("OWS prototype consumable used to verify Sigil item use and GAS integration."));
		Potion->Icon = nullptr;
		Potion->bUnique = false;
		Potion->ItemTags.Reset();
		Potion->ItemTags.AddTag(OWSGameplayTags::Item_Consumable_DemoPotion);
		Potion->StaticFloatAttributes.Reset();
		Potion->StaticIntegerAttributes.Reset();

		UseEffect->UseEffect = UOWSPrototypeHealEffect::StaticClass();
		UseEffect->EffectLevel = 1.0f;
		UseEffect->ConsumeCount = 1;
		UseEffect->bRequiresMissingHealth = true;

		Potion->Fragments.Reset(1);
		Potion->Fragments.Add(UseEffect);
		return true;
	}

	bool SaveGeneratedAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		Asset->PostEditChange();
		Package->MarkPackageDirty();

		FString Filename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(
			Package->GetName(),
			Filename,
			FPackageName::GetAssetPackageExtension()))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Could not resolve a filename for generated package %s."),
				*Package->GetName());
			return false;
		}

		if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Could not create the directory for generated package %s."),
				*Package->GetName());
			return false;
		}

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Failed to save generated asset %s to %s."),
				*Asset->GetPathName(),
				*Filename);
			return false;
		}

		return true;
	}

	bool WireNativeCharacterDefaults(
		USigilItemCollectionDefinition* MainCollection,
		USigilItemSlotCollectionDefinition* EquippedCollection,
		USigilItemDefinition* Sword,
		USigilItemDefinition* Potion)
	{
		AOWSPrototypeCharacter* CharacterDefaults = GetMutableDefault<AOWSPrototypeCharacter>();
		UOWSPrototypeInventorySystemComponent* InventoryDefaults = CharacterDefaults
			? Cast<UOWSPrototypeInventorySystemComponent>(
				CharacterDefaults->GetDefaultSubobjectByName(TEXT("InventorySystem")))
			: nullptr;
		if (!InventoryDefaults)
		{
			UE_LOG(LogOWSFrameworkEditor, Warning,
				TEXT("Assets were saved, but AOWSPrototypeCharacter has no InventorySystem default subobject to update."));
			return false;
		}

		CharacterDefaults->Modify();
		InventoryDefaults->Modify();
		InventoryDefaults->PrototypeMainCollection = MainCollection;
		InventoryDefaults->PrototypeEquippedCollection = EquippedCollection;
		InventoryDefaults->PrototypeSwordDefinition = TSoftObjectPtr<USigilItemDefinition>(FSoftObjectPath(Sword));
		InventoryDefaults->PrototypePotionDefinition = TSoftObjectPtr<USigilItemDefinition>(FSoftObjectPath(Potion));
		InventoryDefaults->PostEditChange();
		CharacterDefaults->PostEditChange();
		return true;
	}

	void Generate()
	{
		UClass* MultiStackClass = LoadClass<USigilItemCollectionDefinition>(
			nullptr,
			TEXT("/Script/SigilInventory.SigilItemMultiStackCollectionDefinition"));
		if (!MultiStackClass)
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Sigil's multistack collection definition class could not be resolved."));
			return;
		}

		bool bCreatedMain = false;
		bool bCreatedEquipped = false;
		bool bCreatedSword = false;
		bool bCreatedPotion = false;
		USigilItemCollectionDefinition* MainCollection = Cast<USigilItemCollectionDefinition>(
			LoadOrCreateGeneratedAsset(MainCollectionSpec, MultiStackClass, bCreatedMain));
		USigilItemSlotCollectionDefinition* EquippedCollection = Cast<USigilItemSlotCollectionDefinition>(
			LoadOrCreateGeneratedAsset(
				EquippedCollectionSpec,
				USigilItemSlotCollectionDefinition::StaticClass(),
				bCreatedEquipped));
		USigilItemDefinition* Sword = Cast<USigilItemDefinition>(
			LoadOrCreateGeneratedAsset(SwordSpec, USigilItemDefinition::StaticClass(), bCreatedSword));
		USigilItemDefinition* Potion = Cast<USigilItemDefinition>(
			LoadOrCreateGeneratedAsset(PotionSpec, USigilItemDefinition::StaticClass(), bCreatedPotion));
		if (!MainCollection || !EquippedCollection || !Sword || !Potion)
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Prototype asset generation stopped without overwriting any conflicting named asset."));
			return;
		}

		if (!ConfigureMainCollection(MainCollection) ||
			!ConfigureSword(Sword) ||
			!ConfigurePotion(Potion))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("Prototype asset generation stopped because the installed APIs did not match the expected contract."));
			return;
		}
		ConfigureEquippedCollection(EquippedCollection);

		const bool bSavedMain = SaveGeneratedAsset(MainCollection);
		const bool bSavedEquipped = SaveGeneratedAsset(EquippedCollection);
		const bool bSavedSword = SaveGeneratedAsset(Sword);
		const bool bSavedPotion = SaveGeneratedAsset(Potion);
		if (!(bSavedMain && bSavedEquipped && bSavedSword && bSavedPotion))
		{
			UE_LOG(LogOWSFrameworkEditor, Error,
				TEXT("One or more OWS prototype assets could not be saved; see preceding errors."));
			return;
		}
		const bool bWiredCharacterDefaults = WireNativeCharacterDefaults(
			MainCollection,
			EquippedCollection,
			Sword,
			Potion);

		UE_LOG(LogOWSFrameworkEditor, Display,
			TEXT("OWS prototype assets generated successfully (created flags: Main=%d Equipped=%d Sword=%d Potion=%d)."),
			bCreatedMain,
			bCreatedEquipped,
			bCreatedSword,
			bCreatedPotion);
		UE_LOG(LogOWSFrameworkEditor, Display, TEXT("Main: %s"), *MainCollectionSpec.GetObjectPath());
		UE_LOG(LogOWSFrameworkEditor, Display, TEXT("Equipped: %s"), *EquippedCollectionSpec.GetObjectPath());
		UE_LOG(LogOWSFrameworkEditor, Display, TEXT("Sword: %s"), *SwordSpec.GetObjectPath());
		UE_LOG(LogOWSFrameworkEditor, Display, TEXT("Potion: %s"), *PotionSpec.GetObjectPath());
		UE_LOG(LogOWSFrameworkEditor, Display,
			TEXT("AOWSPrototypeCharacter inventory defaults updated for this editor session: %s. The OWSCore constructor resolves the same exact paths on future starts."),
			bWiredCharacterDefaults ? TEXT("yes") : TEXT("no"));
	}
}

class FOWSFrameworkEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		GeneratePrototypeAssetsCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("OWS.GeneratePrototypeAssets"),
			TEXT("Idempotently creates or updates the four named OWS/Sigil prototype DataAssets."),
			FConsoleCommandDelegate::CreateStatic(&OWS::PrototypeAssets::Generate),
			ECVF_Default);
	}

	virtual void ShutdownModule() override
	{
		if (GeneratePrototypeAssetsCommand)
		{
			IConsoleManager::Get().UnregisterConsoleObject(GeneratePrototypeAssetsCommand);
			GeneratePrototypeAssetsCommand = nullptr;
		}
	}

private:
	IConsoleObject* GeneratePrototypeAssetsCommand = nullptr;
};

IMPLEMENT_MODULE(FOWSFrameworkEditorModule, OWSFrameworkEditor)
