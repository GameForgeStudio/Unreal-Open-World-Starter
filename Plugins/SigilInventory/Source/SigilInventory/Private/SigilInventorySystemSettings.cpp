// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilInventorySystemSettings.h"
#include "SigilInventoryFactory.h"
#include "Items/SigilItemDefinitionSchema.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventorySystemSettings)

USigilInventorySystemSettings::USigilInventorySystemSettings()
{
	InventoryFactoryClass = USigilInventoryFactory::StaticClass();
}

FName USigilInventorySystemSettings::GetCategoryName() const
{
	return TEXT("Game");
}

const USigilInventorySystemSettings* USigilInventorySystemSettings::Get()
{
	return GetDefault<USigilInventorySystemSettings>();
}

const USigilItemDefinitionSchema* USigilInventorySystemSettings::GetItemDefinitionSchemaForAsset(const FString& AssetPath) const
{
	// Check path-specific schemas first
	for (const FSigilItemDefinitionSchemaEntry& Entry : ItemDefinitionSchemaMap)
	{
		if (!Entry.PathPrefix.IsEmpty() && AssetPath.StartsWith(Entry.PathPrefix))
		{
			if (Entry.Schema.IsValid())
			{
				if (USigilItemDefinitionSchema* Schema = Cast<USigilItemDefinitionSchema>(Entry.Schema.TryLoad()))
				{
					UE_LOG(LogTemp, Log, TEXT("Using path-specific schema %s for asset %s"), *Entry.Schema.ToString(), *AssetPath);
					return Schema;
				}
			}
		}
	}

	// Fall back to default schema
	if (DefaultItemDefinitionSchema.IsValid())
	{
		if (USigilItemDefinitionSchema* Schema = Cast<USigilItemDefinitionSchema>(DefaultItemDefinitionSchema.TryLoad()))
		{
			UE_LOG(LogTemp, Log, TEXT("Using default schema %s for asset %s"), *DefaultItemDefinitionSchema.ToString(), *AssetPath);
			return Schema;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No valid schema found for asset %s"), *AssetPath);
	return nullptr;
}
