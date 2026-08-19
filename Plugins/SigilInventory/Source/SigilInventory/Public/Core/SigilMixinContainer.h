// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif
#include "SigilMixinContainer.generated.h"

/**
 * Record of mixin for serialization.
 * 用于序列化的混合数据记录。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilMixinRecord
{
	GENERATED_BODY()

	/**
	 * The asset path of target object.
	 * 目标对象的资产路径。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FString TargetPath;

	/**
	 * The runtime state of this fragment.
	 * 片段的运行时数据。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="GIS")
	FInstancedStruct Data;

	/**
	 * Equality operator to compare mixin records.
	 * 比较混合数据记录的相等性运算符。
	 * @param Other The another mixin record to compare with. 要比较的其他Mixin记录。
	 * @return True if the record's target path are equal, false otherwise. 如果目标路径相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilMixinRecord& Other) const;

	/**
	 * Checks if the mixin record is valid.
	 * 检查Mixin记录是否有效。
	 * @return True if the target path and data are valid, false otherwise. 如果片段类型和状态有效则返回true，否则返回false。
	 */
	bool IsValid() const;
};


/**
 * Stores the mixed-in data of a const target object (attaches additional runtime data to a const object).
 * 针对常量目标对象存储其混合数据（将额外的运行时数据附加到常量对象）。
 * @details A wrapper of InstancedStruct, allowing storage of any struct data. For save games, ensure struct fields are serializable (marked with SaveGame).
 * @细节 这是实例化结构的包装，允许存储任意结构体数据。对于存档，应确保结构体字段标记为SaveGame并受虚幻序列化系统支持。
 */
USTRUCT(BlueprintType)
struct FSigilMixin : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/**
	 * The target object to which the data is attached.
	 * 数据附加的目标对象。
	 */
	UPROPERTY(NotReplicated)
	TObjectPtr<const UObject> Target;

	/**
	 * The mixed-in data attached to the target object.
	 * 附加到目标对象的混合数据。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mixin")
	FInstancedStruct Data;

	/**
	 * Timestamp of the last data change.
	 * 数据最后更改的时间戳。
	 */
	UPROPERTY()
	float Timestamp;

	/**
	 * Timestamp of the last replication (not replicated).
	 * 最后一次复制的时间戳（不复制）。
	 */
	UPROPERTY(NotReplicated)
	float LastReplicatedTimestamp;

	/**
	 * Default constructor for mixin.
	 * 混合数据的默认构造函数。
	 */
	FSigilMixin()
		: FSigilMixin(nullptr, FInstancedStruct())
	{
	}

	/**
	 * Constructor for mixin with target class and data.
	 * 使用目标类和数据构造混合数据。
	 * @param InClass The target class for the mixin. 混合数据的目标类。
	 * @param InData The instanced struct data to mix in. 要混合的实例化结构数据。
	 */
	FSigilMixin(const TSubclassOf<UObject>& InClass, const FInstancedStruct& InData)
		: Target(InClass), Data(InData)
	{
		Timestamp = 0.f;
		LastReplicatedTimestamp = 0.f;
	}

	/**
	 * Destructor for mixin.
	 * 混合数据的析构函数。
	 */
	~FSigilMixin()
	{
		Reset();
	}

	/**
	 * Checks if the mixin is valid.
	 * 检查混合数据是否有效。
	 * @return True if the mixin is valid (target and data are set), false otherwise. 如果混合数据有效（目标和数据已设置）则返回true，否则返回false。
	 */
	bool IsValid() const
	{
		return Target != nullptr && Data.IsValid();
	}

	/**
	 * Resets the mixin to its default state.
	 * 将混合数据重置为默认状态。
	 */
	void Reset()
	{
		Target = nullptr;
		Data.Reset();
	}

	/**
	 * Computes the hash value for the mixin entry.
	 * 计算混合数据条目的哈希值。
	 * @param Entry The mixin entry to hash. 要哈希的混合数据条目。
	 * @return The hash value. 哈希值。
	 */
	friend uint32 GetTypeHash(const FSigilMixin& Entry);
};

/**
 * Template specialization to enable copying for FSigilMixin.
 * 为 FSigilMixin 启用复制的模板特化。
 */
template <>
struct TStructOpsTypeTraits<FSigilMixin> : TStructOpsTypeTraitsBase2<FSigilMixin>
{
	enum
	{
		WithCopy = true
	};
};

/**
 * Container for storing runtime data for various const objects within the owning object.
 * 用于存储拥有对象中多个常量对象的运行时数据的容器。
 * @details For example, ItemInstance uses this container to store runtime data for each fragment, as fragments themselves are const (cannot be modified at runtime). Only necessary runtime data is serialized/replicated instead of the entire fragments.
 * @细节 例如，ItemInstance 使用此容器存储每个片段的运行时数据，因为片段本身是常量（运行时无法修改）。仅序列化/复制必要的运行时数据，而不是整个片段。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilMixinContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	/**
	 * Default constructor for mixin container.
	 * 混合数据容器的默认构造函数。
	 */
	FSigilMixinContainer()
		: OwningObject(nullptr)
	{
	}

	/**
	 * Constructor for mixin container with an owning object.
	 * 使用所属对象构造混合数据容器。
	 * @param NewObject The object that owns this container. 拥有此容器的对象。
	 */
	explicit FSigilMixinContainer(UObject* NewObject)
		: OwningObject(NewObject)
	{
	}

	/**
	 * Retrieves data for a target class.
	 * 获取目标类的数据。
	 * @param TargetClass The class of the target object. 目标对象的类。
	 * @param OutData The retrieved instanced struct data (output). 检索到的实例化结构数据（输出）。
	 * @return True if data was found, false otherwise. 如果找到数据则返回true，否则返回false。
	 */
	// bool GetDataByTargetClass(const TSubclassOf<UObject>& TargetClass, FInstancedStruct& OutData) const;

	/**
	 * Retrieves data for a target object.
	 * 获取目标对象的数据。
	 * @param Target The target object which the data attached to. 数据所附加到的目标对象。
	 * @param OutData The retrieved instanced struct data (output). 检索到的实例化结构数据（输出）。
	 * @return True if data was found, false otherwise. 如果找到数据则返回true，否则返回false。
	 */
	bool GetDataByTarget(const UObject* Target, FInstancedStruct& OutData) const;

	/**
	 * Finds the index of a target object in the container.
	 * 查找容器中目标对象的索引。
	 * @param Target The target object to find. 要查找的目标对象。
	 * @return The index of the target object, or INDEX_NONE if not found. 目标对象的索引，如果未找到则返回INDEX_NONE。
	 */
	int32 IndexOfTarget(const UObject* Target) const;

	/**
	 * Finds the index of a target class in the container.
	 * 查找容器中目标类的索引。
	 * @param TargetClass The class of the target object. 目标对象的类。
	 * @return The index of the target class, or INDEX_NONE if not found. 目标类的索引，如果未找到则返回INDEX_NONE。
	 */
	int32 IndexOfTargetByClass(const TSubclassOf<UObject>& TargetClass) const;

	/**
	 * Sets data for a target object.
	 * 为目标对象设置数据。
	 * @param Target The target object. 目标对象。
	 * @param Data The instanced struct data to set. 要设置的实例化结构数据。
	 * @return The index of the updated or added entry. 更新或添加条目的索引。
	 */
	int32 SetDataForTarget(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data);

	bool IsObjectLoadedFromDisk(const UObject* Object) const;

	/**
	 * Updates data for a target class.
	 * 更新目标类的数据。
	 * @param TargetClass The class of the target object. 目标对象的类。
	 * @param Data The instanced struct data to update. 要更新的实例化结构数据。
	 * @return The index of the updated entry, or INDEX_NONE if not found. 更新条目的索引，如果未找到则返回INDEX_NONE。
	 */
	int32 UpdateDataByTargetClass(const TSubclassOf<UObject>& TargetClass, const FInstancedStruct& Data);

	/**
	 * Removes data associated with a target class.
	 * 移除与目标类关联的数据。
	 * @param TargetClass The class of the target object. 目标对象的类。
	 */
	void RemoveDataByTargetClass(const TSubclassOf<UObject>& TargetClass);

	/**
	 * Checks if the data is compatible with the target object.
	 * 检查数据是否与目标对象兼容。
	 * @param Target The target object. 目标对象。
	 * @param Data The instanced struct data to check. 要检查的实例化结构数据。
	 * @return True if the data is compatible, false otherwise. 如果数据兼容则返回true，否则返回false。
	 */
	bool CheckCompatibility(const UObject* Target, const FInstancedStruct& Data) const;

	/**
	 * Retrieves all data stored in the container.
	 * 获取容器中存储的所有数据。
	 * @return Array of instanced struct data. 实例化结构数据的数组。
	 */
	TArray<FInstancedStruct> GetAllData() const;

	/**
	 * Retrieves all serializable mixin in the container.
	 * 获取容器中所有可序列化的mixin
	 * @return Array of serializable mixin. 可序列化的Mixin数组
	 */
	TArray<FSigilMixin> GetSerializableMixins() const;

	TArray<FSigilMixinRecord> GetSerializableMixinRecords() const;

	void RestoreFromRecords(const TArray<FSigilMixinRecord>& Records);

	static TArray<FSigilMixin> ConvertRecordsToMixins(const TArray<FSigilMixinRecord>& Records);

	/**
	 * Retrieves all serializable data stored in the container.
	 * 获取容器中存储的所有可序列化数据。
	 * @return Array of serializable instanced struct data. 可序列化实例化结构数据的数组。
	 */
	TArray<FInstancedStruct> GetAllSerializableData() const;

	// -- Begin FFastArraySerializer implementation
	/**
	 * Called after items are added during replication.
	 * 复制期间在添加项目后调用。
	 * @param AddedIndices The indices of added items. 添加项目的索引。
	 * @param FinalSize The final size of the array after addition. 添加后数组的最终大小。
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * Called after items are changed during replication.
	 * 复制期间在项目更改后调用。
	 * @param ChangedIndices The indices of changed items. 更改项目的索引。
	 * @param FinalSize The final size of the array after changes. 更改后数组的最终大小。
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	/**
	 * Called before items are removed during replication.
	 * 复制期间在移除项目前调用。
	 * @param RemovedIndices The indices of removed items. 移除项目的索引。
	 * @param FinalSize The final size of the array after removal. 移除后数组的最终大小。
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * Handles delta serialization for the mixin container.
	 * 处理混合数据容器的增量序列化。
	 * @param DeltaParams The serialization parameters. 序列化参数。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
	// -- End FFastArraySerializer implementation

protected:
	/**
	 * Caches mixin data for faster access.
	 * 缓存混合数据以加速访问。
	 */
	void CacheMixins();

	/**
	 * Updates data at a specific index.
	 * 在指定索引更新数据。
	 * @param Idx The index of the entry to update. 要更新的条目索引。
	 * @param Data The instanced struct data to set. 要设置的实例化结构数据。
	 * @return The index of the updated entry. 更新条目的索引。
	 */
	int32 UpdateDataAt(int32 Idx, const FInstancedStruct& Data);

	/**
	 * The object that owns this container.
	 * 拥有此容器的对象。
	 */
	UPROPERTY(Transient)
	TObjectPtr<UObject> OwningObject;

	/**
	 * List of a target object to data pairs.
	 * 目标对象到数据的键值对列表。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mixin")
	TArray<FSigilMixin> Mixins;

	/**
	 * Cached map for faster access to mixin data (index may vary between client and server).
	 * 用于加速访问混合数据的缓存映射（客户端和服务器的索引可能不同）。
	 */
	UPROPERTY(NotReplicated)
	TMap<TObjectPtr<const UObject>, int32> AcceleratedMap;

	/**
	 * Hash of the last cached state.
	 * 最后缓存状态的哈希值。
	 */
	UPROPERTY()
	uint32 LastCachedHash = INDEX_NONE;
};

/**
 * Template specialization to enable network delta serialization for the mixin container.
 * 为混合数据容器启用网络增量序列化的模板特化。
 */
template <>
struct TStructOpsTypeTraits<FSigilMixinContainer> : TStructOpsTypeTraitsBase2<FSigilMixinContainer>
{
	enum { WithNetDeltaSerializer = true };
};
