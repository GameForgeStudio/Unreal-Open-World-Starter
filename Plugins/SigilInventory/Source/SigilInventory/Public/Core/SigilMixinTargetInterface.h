// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif
#include "UObject/Interface.h"
#include "SigilMixinTargetInterface.generated.h"

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class USigilMixinTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects to handle data mixin functionality.
 * 处理数据混合功能的接口。
 * @details Defines methods for serialization and compatibility of mixin data.
 * @细节 定义了混合数据的序列化和兼容性方法。
 */
class SIGILINVENTORY_API ISigilMixinTargetInterface
{
	GENERATED_BODY()

public:
	/**
	 * Determines if the mixin data can be serialized.
	 * 确定混合数据是否可以序列化。
	 * @return True if the data is serializable, false otherwise. 如果数据可序列化则返回true，否则返回false。
	 */
	virtual bool IsMixinDataSerializable() const = 0;

	/**
	 * Retrieves the compatible data structure for the mixin.
	 * 获取混合数据的兼容数据结构。
	 * @return The script struct compatible with the mixin data. 与混合数据兼容的脚本结构。
	 */
	virtual TObjectPtr<const UScriptStruct> GetCompatibleMixinDataType() const = 0;

	/**
	 * Generate default runtime data for compatible data type.
	 * 生成兼容的混合数据类型的默认值。
	 * @param DefaultState The default value of compatible data. 兼容的数据结构默认值。
	 * @return If has default value. 是否具备默认值？
	 */
	virtual bool MakeDefaultMixinData(FInstancedStruct& DefaultState) const = 0;
};
