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
#include "SigilMixinOwnerInterface.generated.h"

/**
 * Interface class for objects that own a mixin container.
 * 拥有混合数据容器的对象的接口类。
 */
UINTERFACE()
class SIGILINVENTORY_API USigilMixinOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that own a mixin container, to be notified of mixin data changes.
 * 拥有混合数据容器的对象应实现的接口，用于接收混合数据变更通知。
 * @details For example, an item instance will implement this interface to get notified when data attached to different fragment classes is added, changed, or removed.
 * @细节 例如，道具实例将实现此接口，以在附加到不同片段类的数据被添加、更改或移除时收到通知。
 */
class SIGILINVENTORY_API ISigilMixinOwnerInterface
{
	GENERATED_BODY()

public:
	/**
	 * Called when mixin data is added to the owning object.
	 * 当混合数据被添加到拥有对象时调用。
	 * @param Target The target object to which the data is attached. 数据附加到的对象。
	 * @param Data The added instanced struct data. 添加的实例化结构数据。
	 */
	virtual void OnMixinDataAdded(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) = 0;

	/**
	 * Called when mixin data is updated in the owning object.
	 * 当拥有对象中的混合数据被更新时调用。
	 * @param Target The target object to which the data is attached. 数据附加到的对象。
	 * @param Data The updated instanced struct data. 更新的实例化结构数据。
	 */
	virtual void OnMixinDataUpdated(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) = 0;

	/**
	 * Called when mixin data is removed from the owning object.
	 * 当从拥有对象中移除混合数据时调用。
	 * @param Target The target object to which the data was attached. 数据附加到的对象。
	 * @param Data The removed instanced struct data. 移除的实例化结构数据。
	 */
	virtual void OnMixinDataRemoved(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data) = 0;
};
