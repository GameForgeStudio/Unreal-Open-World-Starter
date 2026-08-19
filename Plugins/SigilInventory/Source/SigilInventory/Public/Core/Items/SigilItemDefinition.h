// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SigilGameplayTagFloat.h"
#include "SigilGameplayTagInteger.h"
#include "Engine/DataAsset.h"
#include "SigilItemDefinition.generated.h"

class USigilItemDefinitionSchema;
class USigilItemFragment;
class USigilItemInstance;
class UTexture2D;

/**
 * An item definition is a data asset. Creating a new item definition is equivalent to creating a new instance of the item definition class.
 * Essentially, item definitions are static data that can incorporate item data fragments to achieve complex data structures.
 * 道具定义是一个数据资产，创建一个新道具定义相当于新建一个类型为道具定义的资产。道具定义本质上是静态数据，可以添加道具数据片段，以实现复杂的数据结构。
 */
UCLASS(BlueprintType, Const)
class SIGILINVENTORY_API USigilItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the item definition.
	 * 道具定义的构造函数。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Display name of the item on the UI.
	 * 道具在UI上的显示名称。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	/**
	 * Description of the item on the UI.
	 * 道具在UI上的描述。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText Description;

	/**
	 * Icon of the item on the UI.
	 * 道具在UI上的图标。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	TObjectPtr<UTexture2D> Icon;

	/**
	 * Indicates whether the item is unique, preventing it from being stacked.
	 * 表示道具是否唯一，唯一道具不可堆叠。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Common)
	bool bUnique;

	/**
	 * Gameplay tags associated with the item for querying, categorization, and validation.
	 * 与道具关联的游戏标签，用于查询、分类和验证。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Common)
	FGameplayTagContainer ItemTags;

	/**
	 * Static tag-to-float attributes for the item definition.
	 * 道具定义的静态标签到浮点数的属性映射。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Settings, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagFloat> StaticFloatAttributes;

	/**
	 * Static tag-to-integer attributes for the item definition.
	 * 道具定义的静态标签到整数的属性映射。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Settings, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagInteger> StaticIntegerAttributes;

	/**
	 * Array of item data fragments to form simple or complex data structures. No duplicate fragment types are allowed.
	 * 数据片段数组，用于构成简单或复杂的道具数据结构。不允许重复的片段类型。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Settings, Instanced, meta=(NoElementDuplicate))
	TArray<TObjectPtr<USigilItemFragment>> Fragments;

	/**
	 * Gets a fragment by its class.
	 * 通过类获取数据片段。
	 * @param FragmentClass The class of the fragment to retrieve. 要检索的片段类。
	 * @return The fragment instance, or nullptr if not found. 片段实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemDefinition")
	const USigilItemFragment* GetFragment(TSubclassOf<USigilItemFragment> FragmentClass) const;

	/**
	 * Checks if the item definition has a specific float attribute.
	 * 检查道具定义是否具有特定的浮点属性。
	 * @param AttributeTag The tag of the attribute to check. 要检查的属性标签。
	 * @return True if the attribute exists, false otherwise. 如果属性存在则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemDefinition")
	bool HasFloatAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Gets the value of a static float attribute.
	 * 获取静态浮点属性的值。
	 * @param AttributeTag The tag of the attribute to retrieve. 要检索的属性标签。
	 * @return The value of the float attribute, or 0 if not found. 浮点属性的值，如果未找到则返回0。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemDefinition")
	float GetFloatAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Checks if the item definition has a specific integer attribute.
	 * 检查道具定义是否具有特定的整数属性。
	 * @param AttributeTag The tag of the attribute to check. 要检查的属性标签。
	 * @return True if the attribute exists, false otherwise. 如果属性存在则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemDefinition")
	bool HasIntegerAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Gets the value of a static integer attribute.
	 * 获取静态整数属性的值。
	 * @param AttributeTag The tag of the attribute to retrieve. 要检索的属性标签。
	 * @return The value of the integer attribute, or 0 if not found. 整数属性的值，如果未找到则返回0。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemDefinition")
	int32 GetIntegerAttribute(FGameplayTag AttributeTag) const;

	/**
	 * Template function to find a fragment by its class.
	 * 模板函数，通过类查找数据片段。
	 * @param ResultClass The class of the fragment to find. 要查找的片段类。
	 * @return The fragment instance cast to the specified class, or nullptr if not found. 转换为指定类的片段实例，如果未找到则返回nullptr。
	 */
	template <typename ResultClass>
	const ResultClass* FindFragment() const
	{
		return static_cast<const ResultClass*>(GetFragment(ResultClass::StaticClass()));
	}

	/**
	 * Cached map for faster access to integer attributes.
	 * 用于快速访问整数属性的缓存映射。
	 */
	UPROPERTY()
	TMap<FGameplayTag, int32> IntegerAttributeMap;

	/**
	 * Cached map for faster access to float attributes.
	 * 用于快速访问浮点属性的缓存映射。
	 */
	UPROPERTY()
	TMap<FGameplayTag, float> FloatAttributeMap;

	/**
	 * Finds a fragment in an item definition by its class.
	 * 在道具定义中通过类查找数据片段。
	 * @param ItemDefinition The item definition to query. 要查询的道具定义。
	 * @param FragmentClass The class of the fragment to find. 要查找的片段类。
	 * @return The fragment instance, or nullptr if not found. 片段实例，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="GIS|ItemDefinition", meta=(DeterminesOutputType=FragmentClass))
	static const USigilItemFragment* FindFragmentOfItemDefinition(TSoftObjectPtr<USigilItemDefinition> ItemDefinition, TSubclassOf<USigilItemFragment> FragmentClass);

#if WITH_EDITOR
	/**
	 * Called before the item definition is saved in the editor.
	 * 编辑器中道具定义保存前调用。
	 * @param SaveContext The save context. 保存上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	/**
	 * Validates the item definition's data in the editor.
	 * 在编辑器中验证道具定义的数据。
	 * @param Context The validation context. 验证上下文。
	 * @return The result of the data validation. 数据验证的结果。
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
