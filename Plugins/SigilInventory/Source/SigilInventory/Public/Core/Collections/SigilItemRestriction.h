// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SigilItemInfo.h"
#include "SigilItemRestriction.generated.h"

/**
 * Base class for item restrictions used to limit item collection operations.
 * 用于限制道具集合操作的道具限制基类。
 * @details Subclasses can extend this to implement custom restrictions.
 * @细节 子类可以扩展此类以实现自定义限制。
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, CollapseCategories, Abstract, Const)
class SIGILINVENTORY_API USigilItemRestriction : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Checks if an item can be added to the collection.
	 * 检查道具是否可以添加到集合。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @param ReceivingCollection The collection to add the item to. 要添加到的集合。
	 * @return True if any valid item can be added, false otherwise. 如果可以添加任何有效道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category=ItemRestriction)
	bool CanAddItem(UPARAM(ref) FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const;

	/**
	 * Checks if an item can be removed from the collection.
	 * 检查道具是否可以从集合中移除。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @return True if any valid item can be removed, false otherwise. 如果可以移除任何有效道具则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category=ItemRestriction)
	bool CanRemoveItem(UPARAM(ref) FSigilItemInfo& ItemInfo) const;

	/**
	 * Gets the reason for rejecting an add operation.
	 * 获取拒绝添加操作的原因。
	 * @return The rejection reason text. 拒绝原因文本。
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category=ItemRestriction)
	FText GetRejectAddReason() const;

	/**
	 * Implementation of GetRejectAddReason.
	 * GetRejectAddReason 的实现。
	 * @return The rejection reason text. 拒绝原因文本。
	 */
	virtual FText GetRejectAddReason_Implementation() const { return RejectAddReason; }

	/**
	 * Gets the reason for rejecting a remove operation.
	 * 获取拒绝移除操作的原因。
	 * @return The rejection reason text. 拒绝原因文本。
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category=ItemRestriction)
	FText GetRejectRemoveReason() const;

	/**
	 * Implementation of GetRejectRemoveReason.
	 * GetRejectRemoveReason 的实现。
	 * @return The rejection reason text. 拒绝原因文本。
	 */
	virtual FText GetRejectRemoveReason_Implementation() const { return RejectAddReason; }

protected:
	/**
	 * Internal check for adding an item to the collection.
	 * 检查道具是否可以添加到集合的内部函数。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @param ReceivingCollection The collection to add the item to. 要添加到的集合。
	 * @return True if the item can be added, false otherwise. 如果道具可以添加则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, Category=ItemRestriction, meta=(DisplayName=CanAddItem))
	bool CanAddItemInternal(UPARAM(ref) FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const;

	/**
	 * Implementation of CanAddItemInternal.
	 * CanAddItemInternal 的实现。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @param ReceivingCollection The collection to add the item to. 要添加到的集合。
	 * @return True if the item can be added, false otherwise. 如果道具可以添加则返回true，否则返回false。
	 */
	virtual bool CanAddItemInternal_Implementation(FSigilItemInfo& ItemInfo, USigilItemCollection* ReceivingCollection) const;

	/**
	 * Internal check for removing an item from the collection.
	 * 检查道具是否可以从集合移除的内部函数。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @return True if the item can be removed, false otherwise. 如果道具可以移除则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintNativeEvent, Category=ItemRestriction, meta=(DisplayName=CanRemoveItem))
	bool CanRemoveItemInternal(UPARAM(ref) FSigilItemInfo& ItemInfo) const;

	/**
	 * Implementation of CanRemoveItemInternal.
	 * CanRemoveItemInternal 的实现。
	 * @param ItemInfo The item information to check (modifiable). 要检查的道具信息（可修改）。
	 * @return True if the item can be removed, false otherwise. 如果道具可以移除则返回true，否则返回false。
	 */
	virtual bool CanRemoveItemInternal_Implementation(FSigilItemInfo& ItemInfo) const;

	/**
	 * The reason for rejecting an add operation.
	 * 拒绝添加操作的原因。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ItemRestriction)
	FText RejectAddReason;

	/**
	 * The reason for rejecting a remove operation.
	 * 拒绝移除操作的原因。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ItemRestriction)
	FText RejectRemoveReason;
};