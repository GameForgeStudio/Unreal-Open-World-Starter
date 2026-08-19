// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "SigilCurrencyEntry.h"
#include "SigilCurrencyContainer.generated.h"

class USigilCurrencySystemComponent;
class USigilCurrencyDefinition;
struct FSigilCurrencyContainer;

/**
 * Container for storing currency entries.
 * 用于存储货币条目的容器。
 */
USTRUCT()
struct SIGILINVENTORY_API FSigilCurrencyContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	/**
	 * Default constructor for the currency container.
	 * 货币容器的默认构造函数。
	 */
	FSigilCurrencyContainer()
	{
	};

	/**
	 * Constructor for the currency container with an owning component.
	 * 使用拥有组件构造货币容器。
	 * @param InOwner The owning currency system component. 拥有此容器的货币系统组件。
	 */
	FSigilCurrencyContainer(USigilCurrencySystemComponent* InOwner)
		: OwningComponent(InOwner)
	{
	}

	//~FFastArraySerializer contract
	/**
	 * Called before entries are removed during replication.
	 * 复制期间移除条目前调用。
	 * @param RemovedIndices The indices of entries to remove. 要移除的条目索引。
	 * @param FinalSize The final size of the entries array after removal. 移除后条目数组的最终大小。
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * Called after entries are added during replication.
	 * 复制期间添加条目后调用。
	 * @param AddedIndices The indices of added entries. 添加的条目索引。
	 * @param FinalSize The final size of the entries array after addition. 添加后条目数组的最终大小。
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * Called after entries are changed during replication.
	 * 复制期间条目更改后调用。
	 * @param ChangedIndices The indices of changed entries. 更改的条目索引。
	 * @param FinalSize The final size of the entries array after change. 更改后条目数组的最终大小。
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	/**
	 * Handles delta serialization for network replication.
	 * 处理网络复制的增量序列化。
	 * @param DeltaParms The serialization parameters. 序列化参数。
	 * @return True if serialization was successful, false otherwise. 如果序列化成功则返回true，否则返回false。
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FSigilCurrencyEntry, FSigilCurrencyContainer>(Entries, DeltaParms, *this);
	}

	/**
	 * The owning currency system component.
	 * 拥有此容器的货币系统组件。
	 */
	UPROPERTY()
	TObjectPtr<USigilCurrencySystemComponent> OwningComponent{nullptr};

	/**
	 * Array of currency entries.
	 * 货币条目数组。
	 */
	UPROPERTY(VisibleAnywhere, Category="Currency", meta=(DisplayName="Currencies", TitleProperty="{Definition}->{Value}"))
	TArray<FSigilCurrencyEntry> Entries;
};

/**
 * Traits for the currency container to enable network delta serialization.
 * 货币容器的特性，用于启用网络增量序列化。
 */
template <>
struct TStructOpsTypeTraits<FSigilCurrencyContainer> : TStructOpsTypeTraitsBase2<FSigilCurrencyContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};