// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilGameplayTagFloat.h"
#include "SigilGameplayTagInteger.h"
#include "Engine/DataAsset.h"
#include "SigilItemDefinitionSchema.generated.h"

class USigilItemInstance;
class USigilItemDefinition;
class USigilItemFragment;

/**
 * Structure representing a validation entry for item definitions.
 * 表示道具定义验证条目的结构体。
 */
USTRUCT()
struct SIGILINVENTORY_API FSigilItemDefinitionValidationEntry
{
	GENERATED_BODY()

	/**
	 * Gameplay tag query to match items for this validation.
	 * 用于匹配此验证的游戏标签查询。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	FGameplayTagQuery ItemTagQuery;

	/**
	 * The required item instance class for matching items.
	 * 匹配道具所需的道具实例类。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TSoftClassPtr<USigilItemInstance> InstanceClass;

	/**
	 * List of required fragment classes for the item definition.
	 * 道具定义所需的片段类列表。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TArray<TSoftClassPtr<USigilItemFragment>> RequiredFragments;

	/**
	 * List of forbidden fragment classes for the item definition.
	 * 道具定义禁止的片段类列表。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TArray<TSoftClassPtr<USigilItemFragment>> ForbiddenFragments;

	/**
	 * List of required float attributes with default values.
	 * 必需的浮点属性列表，包含默认值。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagFloat> RequiredFloatAttributes;

	/**
	 * List of required integer attributes with default values.
	 * 必需的整数属性列表，包含默认值。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagInteger> RequiredIntegerAttributes;

	/**
	 * Whether to enforce the bUnique property.
	 * 是否强制执行 bUnique 属性。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	bool bEnforceUnique = false;

	/**
	 * The required value for bUnique if enforced.
	 * 如果强制执行 bUnique，则指定其值。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema, meta=(EditCondition="bEnforceUnique"))
	bool RequiredUniqueValue = false;
};

/**
 * The item definition schema is a data asset used in the editor for item definition data validation.
 * 道具定义模式是一种数据资产，用于在编辑器中进行道具定义的数据验证。
 */
UCLASS(NotBlueprintable)
class SIGILINVENTORY_API USigilItemDefinitionSchema : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Validates an item definition against the schema.
	 * 针对模式验证道具定义。
	 * @param Definition The item definition to validate. 要验证的道具定义。
	 * @param OutError The error message if validation fails. 如果验证失败，则返回错误消息。
	 * @return True if the validation passes, false otherwise. 如果验证通过则返回true，否则返回false。
	 */
	static bool TryValidateItemDefinition(const USigilItemDefinition* Definition, FText& OutError);

	/**
	 * Performs pre-save validation and auto-fixes for an item definition.
	 * 对道具定义进行保存前验证和自动修复。
	 * @param Definition The item definition to validate and fix. 要验证和修复的道具定义。
	 * @param OutError The error message if validation fails. 如果验证失败，则返回错误消息。
	 */
	static void TryPreSaveItemDefinition(USigilItemDefinition* Definition, FText& OutError);

	/**
	 * Validates an item definition against the schema (instance method).
	 * 针对模式验证道具定义（实例方法）。
	 * @param Definition The item definition to validate. 要验证的道具定义。
	 * @param OutError The error message if validation fails. 如果验证失败，则返回错误消息。
	 * @return True if the validation passes, false otherwise. 如果验证通过则返回true，否则返回false。
	 */
	virtual bool TryValidate(const USigilItemDefinition* Definition, FText& OutError) const;

	/**
	 * Performs pre-save validation and auto-fixes for an item definition (instance method).
	 * 对道具定义进行保存前验证和自动修复（实例方法）。
	 * @param Definition The item definition to validate and fix. 要验证和修复的道具定义。
	 * @param OutError The error message if validation fails. 如果验证失败，则返回错误消息。
	 */
	virtual void TryPreSave(USigilItemDefinition* Definition, FText& OutError) const;

#if WITH_EDITOR
	/**
	 * Called before the schema is saved in the editor.
	 * 编辑器中模式保存前调用。
	 * @param SaveContext The save context. 保存上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	/**
	 * Validates the schema's data in the editor.
	 * 在编辑器中验证模式的数据。
	 * @param Context The validation context. 验证上下文。
	 * @return The result of the data validation. 数据验证的结果。
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	/**
	 * Array of validation entries for the schema.
	 * 模式的验证条目数组。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TArray<FSigilItemDefinitionValidationEntry> ValidationEntries;

	/**
	 * List of required fragment classes for all item definitions.
	 * 所有道具定义所需的常规片段类列表。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TArray<TSoftClassPtr<USigilItemFragment>> CommonRequiredFragments;

	/**
	 * The required parent tag for all item definitions' ItemTags.
	 * 所有道具定义的 ItemTags 必须包含的父级标签。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	FGameplayTag RequiredParentTag;

	/**
	 * Global fragment order for all item definitions.
	 * 所有道具定义的全局片段排序。
	 */
	UPROPERTY(EditDefaultsOnly, Category = Schema)
	TArray<TSoftClassPtr<USigilItemFragment>> FragmentOrder;
};
