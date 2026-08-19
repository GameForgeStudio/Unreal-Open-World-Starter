// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilInventorySubsystem.h"
#include "Engine/World.h"
#include "SigilInventorySystemSettings.h"
#include "SigilInventoryFactory.h"
#include "SigilInventoryLogChannels.h"
#include "Items/SigilItemDefinition.h"
#include "Items/SigilItemInstance.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventorySubsystem)

USigilInventorySubsystem* USigilInventorySubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		return WorldContextObject->GetWorld()->GetGameInstance()->GetSubsystem<USigilInventorySubsystem>();
	}
	return nullptr;
}

void USigilInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	InitializeFactory();
}

void USigilInventorySubsystem::Deinitialize()
{
	Super::Deinitialize();
	Factory = nullptr;
}

USigilItemInstance* USigilInventorySubsystem::CreateItem(AActor* Owner, TSoftObjectPtr<USigilItemDefinition> ItemDefinition)
{
	if (Factory && !ItemDefinition.IsNull())
	{
		USigilItemDefinition* LoadedDefinition = ItemDefinition.LoadSynchronous();
		check(LoadedDefinition)

		if (LoadedDefinition == nullptr)
		{
			SIGIL_INVENTORY_LOG(Error, "Cannot create Item with null Item Definition.");
			return nullptr;
		}

		return Factory->CreateItem(Owner, LoadedDefinition);
	}
	return nullptr;
}

USigilItemInstance* USigilInventorySubsystem::CreateItem(AActor* Owner, const USigilItemDefinition* ItemDefinition)
{
	if (Factory && ItemDefinition != nullptr)
	{
		return Factory->CreateItem(Owner, ItemDefinition);
	}
	return nullptr;
}

USigilItemInstance* USigilInventorySubsystem::DuplicateItem(AActor* Owner, USigilItemInstance* FromItem, bool bGenerateNewId)
{
	if (Factory)
	{
		return Factory->DuplicateItem(Owner, FromItem, bGenerateNewId);
	}
	return nullptr;
}

bool USigilInventorySubsystem::SerializeItem(USigilItemInstance* Item, FSigilItemRecord& Record)
{
	if (Factory)
	{
		return Factory->SerializeItem(Item, Record);
	}
	return false;
}

USigilItemInstance* USigilInventorySubsystem::DeserializeItem(AActor* Owner, const FSigilItemRecord& Record)
{
	if (Factory)
	{
		return Factory->DeserializeItem(Owner, Record);
	}
	return nullptr;
}

bool USigilInventorySubsystem::SerializeCollection(USigilItemCollection* ItemCollection, FSigilCollectionRecord& Record)
{
	if (Factory)
	{
		return Factory->SerializeCollection(ItemCollection, Record);
	}
	return false;
}

void USigilInventorySubsystem::DeserializeCollection(USigilInventorySystemComponent* InventorySystem, const FSigilCollectionRecord& Record, TMap<FGuid, USigilItemInstance*>& ItemsMap)
{
	if (Factory)
	{
		return Factory->DeserializeCollection(InventorySystem, Record, ItemsMap);
	}
}

bool USigilInventorySubsystem::SerializeInventory(USigilInventorySystemComponent* InventorySystem, FSigilInventoryRecord& Record)
{
	if (Factory)
	{
		return Factory->SerializeInventory(InventorySystem, Record);
	}
	return false;
}

void USigilInventorySubsystem::DeserializeInventory(USigilInventorySystemComponent* InventorySystem, const FSigilInventoryRecord& Record)
{
	if (Factory)
	{
		return Factory->DeserializeInventory(InventorySystem, Record);
	}
}


void USigilInventorySubsystem::InitializeFactory()
{
	if (USigilInventorySystemSettings::Get() == nullptr || USigilInventorySystemSettings::Get()->InventoryFactoryClass.IsNull())
	{
		SIGIL_INVENTORY_LOG(Error, "Missing ItemFactoryClass in inventory system settings.");
		return;
	}
	const UClass* FactoryClass = USigilInventorySystemSettings::Get()->InventoryFactoryClass.LoadSynchronous();

	if (FactoryClass == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "invalid ItemFactoryClass found inventory system settings.");
		return;
	}

	USigilInventoryFactory* TempFactory = NewObject<USigilInventoryFactory>(this, FactoryClass);
	if (TempFactory == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "Failed to create item factory instance.Class:%s", *FactoryClass->GetName());
		return;
	}
	Factory = TempFactory;
}
