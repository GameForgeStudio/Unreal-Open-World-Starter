// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SigilInventorySystemSettings.generated.h"

class USigilInventoryFactory;

/**
 * Structure representing a mapping of asset path prefix to item definition schema.
 * 表示资产路径前缀到道具定义模式的映射结构体。
 */
USTRUCT()
struct SIGILINVENTORY_API FSigilItemDefinitionSchemaEntry
{
	GENERATED_BODY()

	/**
	 * The path prefix for assets to apply this schema (e.g., "/Game/ActionRPG").
	 * 应用此模式的资产路径前缀（例如，"/Game/ActionRPG"）。
	 */
	UPROPERTY(config, EditAnywhere, Category="Settings", meta=(ContentDir))
	FString PathPrefix;

	/**
	 * The schema to apply for assets under this path prefix.
	 * 对此路径前缀下的资产应用的模式。
	 */
	UPROPERTY(config, EditAnywhere, Category="Settings", meta=(AllowedClasses="/Script/SigilInventory.SigilItemDefinitionSchema"))
	FSoftObjectPath Schema;
};

/**
 * Settings for the Generic Inventory System.
 * 通用库存系统的设置。
 */
UCLASS(Config=Game, defaultconfig)
class SIGILINVENTORY_API USigilInventorySystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the inventory system settings.
	 * 库存系统设置的构造函数。
	 */
	USigilInventorySystemSettings();

	/**
	 * Gets the category name for these settings.
	 * 获取这些设置的类别名称。
	 * @return The category name. 类别名称。
	 */
	virtual FName GetCategoryName() const override;

	/**
	 * Gets the inventory system settings instance.
	 * 获取库存系统设置实例。
	 * @return The inventory system settings. 库存系统设置。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|Settings", meta=(DisplayName="Get Inventory System Settings"))
	static const USigilInventorySystemSettings* Get();

	/**
	 * Gets the item definition schema for a given asset path.
	 * 获取给定资产路径的道具定义模式。
	 * @param AssetPath The path of the asset to find a schema for. 要查找模式的资产路径。
	 * @return The schema to use, or nullptr if none found. 要使用的模式，如果未找到则返回nullptr。
	 */
	const class USigilItemDefinitionSchema* GetItemDefinitionSchemaForAsset(const FString& AssetPath) const;

	/**
	 * Inventory Factory class for managing low level operation in GIS.
	 * 库存工厂类用于管理GIS中的底层操作。
	 */
	UPROPERTY(config, Category="Settings", EditDefaultsOnly, NoClear)
	TSoftClassPtr<USigilInventoryFactory> InventoryFactoryClass;

	/**
	 * Array of path-to-schema mappings for item definition validation.
	 * 用于道具定义验证的路径到模式的映射数组。
	 * @details Assets under the specified path prefix will use the corresponding schema. If no match is found, the default schema is used.
	 * @细节 指定路径前缀下的资产将使用对应的模式。如果未找到匹配，则使用默认模式。
	 */
	UPROPERTY(config, EditAnywhere, Category="Settings")
	TArray<FSigilItemDefinitionSchemaEntry> ItemDefinitionSchemaMap;

	/**
	 * The default schema to enforce item definition layout consistency when no path-specific schema is found.
	 * 当未找到路径特定模式时，用于强制道具定义布局一致性的默认模式。
	 */
	UPROPERTY(config, EditAnywhere, Category="Settings", meta=(AllowedClasses="/Script/SigilInventory.SigilItemDefinitionSchema"))
	FSoftObjectPath DefaultItemDefinitionSchema;
};
