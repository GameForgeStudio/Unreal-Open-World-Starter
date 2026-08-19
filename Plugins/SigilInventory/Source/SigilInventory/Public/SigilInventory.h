// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * Module class for the Generic Inventory System.
 * 通用库存系统的模块类。
 * @details Implements the module interface for initializing and shutting down the inventory system.
 * @细节 实现模块接口以初始化和关闭库存系统。
 */
class FSigilInventoryModule : public IModuleInterface
{
public:
	/**
	 * Called when the module is loaded into memory.
	 * 模块加载到内存时调用。
	 */
	virtual void StartupModule() override;

	/**
	 * Called when the module is unloaded from memory.
	 * 模块从内存中卸载时调用。
	 */
	virtual void ShutdownModule() override;
};