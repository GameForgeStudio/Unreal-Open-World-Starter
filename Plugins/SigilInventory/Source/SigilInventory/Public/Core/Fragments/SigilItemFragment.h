// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilMixinTargetInterface.h"
#include "UObject/Object.h"
#include "SigilItemFragment.generated.h"

class USigilItemInstance;

/**
 * Base class for item fragments, which are data objects attached to item definitions.
 * 道具片段，即道具定义上附加的数据对象的基类。
 * @details Allows users to create custom item fragment classes for extending item functionality.
 * @细节 允许用户创建自定义道具片段类以扩展道具功能。
 */
UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew, Abstract, CollapseCategories, HideDropdown, Const)
class SIGILINVENTORY_API USigilItemFragment : public UObject, public ISigilMixinTargetInterface
{
	GENERATED_BODY()

public:
	/**
	 * Called when an item instance is created, allowing fragment-specific initialization.
	 * 在道具实例创建时调用，允许特定片段的初始化。
	 * @param ItemInstance The item instance being created. 被创建的道具实例。
	 */
	virtual void OnInstanceCreated(USigilItemInstance* ItemInstance) const
	{
		Bp_OnInstancedCrated(ItemInstance);
	}

	/**
	 * Determines if the fragment's runtime data can be serialized.
	 * 确定片段的运行时数据是否可以序列化。
	 * @return True if the data is serializable, false otherwise. 如果数据可序列化则返回true，否则返回false。
	 */
	virtual bool IsMixinDataSerializable() const override;

	/**
	 * Retrieves the compatible data structure for serialization or replication.
	 * 获取用于序列化或复制的兼容数据结构。
	 * @return The script struct compatible with this fragment. 与此片段兼容的脚本结构。
	 */
	virtual TObjectPtr<const UScriptStruct> GetCompatibleMixinDataType() const override;

	/**
	 * Generate default runtime data for compatible data type.
	 * 生成兼容的混合数据类型的默认值。
	 * @param DefaultState The default value of compatible data. 兼容的数据结构默认值。
	 * @return If has default value. 是否具备默认值？
	 */
	virtual bool MakeDefaultMixinData(FInstancedStruct& DefaultState) const override;

protected:
	/**
	 * Blueprint event for handling instance creation logic.
	 * 处理实例创建逻辑的蓝图事件。
	 * @param ItemInstance The item instance being created. 被创建的道具实例。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="ItemFragment", meta=(DisplayName="On Instance Created"))
	void Bp_OnInstancedCrated(USigilItemInstance* ItemInstance) const;

	/**
	 * Returns the struct type for runtime data serialization or replication.
	 * 返回运行时数据序列化或复制的结构类型。
	 * @return The compatible runtime data struct type. 兼容的运行时数据结构类型。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="ItemFragment")
	const UScriptStruct* GetCompatibleStateType() const;

	/**
	 * Provide default values for fragment's runtime data type.
	 * 针对运行时数据类型提供默认值。
	 * @param DefaultState The default data. 默认数据
	 * @return If has default value. 是否具备默认值。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="ItemFragment")
	bool MakeDefaultState(FInstancedStruct& DefaultState) const;

	/**
	 * Determines if the fragment's runtime data supports serialization to disk.
	 * 确定片段的运行时数据是否支持序列化到磁盘。
	 * @details Runtime data is replicated over the network, but serialization to disk is optional.
	 * @细节 运行时数据通过网络复制，但序列化到磁盘是可选的。
	 * @return True if the data supports serialization, false otherwise. 如果数据支持序列化则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="ItemFragment")
	bool IsStateSerializable() const;
};
