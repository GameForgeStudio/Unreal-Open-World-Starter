// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilItemFragment.h"
#include "Attributes/SigilGameplayTagFloat.h"
#include "Attributes/SigilGameplayTagInteger.h"
#include "SigilItemFragment_DynamicAttributes.generated.h"

/**
 * Item fragment for adding dynamic attributes to an item instance.
 * 为道具实例添加动态属性的道具片段。
 * @details Initial attributes are applied to the item instance upon creation.
 * @细节 初始属性在道具实例创建时应用。
 * @attention Use static attributes in the item definition for attributes that don't change at runtime.
 * @注意 对于运行时不更改的属性，请使用道具定义中的静态属性。
 */
UCLASS(DisplayName="Dynamic Attribute Settings", Category="BuiltIn")
class SIGILINVENTORY_API USigilItemFragment_DynamicAttributes : public USigilItemFragment
{
	GENERATED_BODY()

protected:
	/**
	 * Array of initial float attributes applied to the item instance.
	 * 应用于道具实例的初始浮点属性数组。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Attribute, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagFloat> InitialFloatAttributes;

	/**
	 * Array of initial integer attributes applied to the item instance.
	 * 应用于道具实例的初始整型属性数组。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Attribute, meta=(TitleProperty="{Tag} -> {Value}"))
	TArray<FSigilGameplayTagInteger> InitialIntegerAttributes;

	/**
	 * Cached map for faster access to integer attributes.
	 * 用于快速访问整型属性的缓存映射。
	 */
	UPROPERTY()
	TMap<FGameplayTag, int32> IntegerAttributeMap;

	/**
	 * Cached map for faster access to float attributes.
	 * 用于快速访问浮点属性的缓存映射。
	 */
	UPROPERTY()
	TMap<FGameplayTag, float> FloatAttributeMap;

public:
	/**
	 * Initializes the item instance with dynamic attributes upon creation.
	 * 在道具实例创建时使用动态属性进行初始化。
	 * @param Instance The item instance to initialize. 要初始化的道具实例。
	 */
	virtual void OnInstanceCreated(USigilItemInstance* Instance) const override;

	/**
	 * Retrieves the default value of a float attribute by tag.
	 * 通过标签获取浮点属性的默认值。
	 * @param AttributeTag The tag identifying the attribute. 标识属性的标签。
	 * @return The default value of the float attribute. 浮点属性的默认值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemFragment|DynamicAttributes")
	float GetFloatAttributeDefault(FGameplayTag AttributeTag) const;

	/**
	 * Retrieves the default value of an integer attribute by tag.
	 * 通过标签获取整型属性的默认值。
	 * @param AttributeTag The tag identifying the attribute. 标识属性的标签。
	 * @return The default value of the integer attribute. 整型属性的默认值。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|ItemFragment|DynamicAttributes")
	int32 GetIntegerAttributeDefault(FGameplayTag AttributeTag) const;

#if WITH_EDITOR
	/**
	 * Called before saving the object to perform editor-specific operations.
	 * 在保存对象之前调用以执行编辑器特定的操作。
	 * @param SaveContext The context for the save operation. 保存操作的上下文。
	 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};
