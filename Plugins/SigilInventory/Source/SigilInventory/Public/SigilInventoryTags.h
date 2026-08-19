// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

/**
 * Namespace for collection-related gameplay tags.
 * 集合相关游戏标签的命名空间。
 */
namespace SigilCollectionTags
{
	/** Main inventory collection tag. 主要库存集合标签。 */
	SIGILINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Main)
	/** Hidden inventory collection tag. 隐藏库存集合标签。 */
	SIGILINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hidden)
	/** Equipped items collection tag. 已装备道具集合标签。 */
	SIGILINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipped)
	/** Quick bar collection tag. 快捷栏集合标签。 */
	SIGILINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(QuickBar);
}

/**
 * Namespace for attribute-related gameplay tags.
 * 属性相关游戏标签的命名空间。
 */
namespace SigilAttributeTags
{
	/** Tag for the stack size limit of an item. 道具堆叠数量限制标签。 */
	SIGILINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StackSizeLimit);
}
