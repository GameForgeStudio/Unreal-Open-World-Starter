// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "Templates/SubclassOf.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif
#include "SigilAsyncAction_WaitItemFragmentDataChanged.generated.h"

class USigilItemFragment;
class USigilItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSigilWaitFragmentStateChangedSignature, const USigilItemFragment*, Fragment, const FInstancedStruct&, Data);


/**
 * Async action to wait for a fragment data changed on an item instance.
 * 在道具实例上等待指定道具片段的运行时数据变更。
 */
UCLASS()
class SIGILINVENTORY_API USigilAsyncAction_WaitItemFragmentDataChanged : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	/**
	 * Wait for a fragment data changed on an item instance.
	 * 在道具实例上等待指定道具片段的运行时数据变更。
	 * @param WorldContext The world context object to get the world reference. 用于获取世界引用的世界上下文对象。
	 * @param ItemInstance The target item instance to wait for. 要等待的目标道具。
	 * @param FragmentClass The fragment type to wait for. 要等待的片段类型。 
	 * @return The created wait action. 创建的等待动作。
	 */
	UFUNCTION(BlueprintCallable, Category="GIS|Async", meta = (WorldContext = "WorldContext", DefaultToSelf="ItemInstnace", BlueprintInternalUseOnly = "true"))
	static USigilAsyncAction_WaitItemFragmentDataChanged* WaitItemFragmentStateChanged(UObject* WorldContext, USigilItemInstance* ItemInstance, TSoftClassPtr<USigilItemFragment> FragmentClass);

	virtual void Activate() override;
	virtual void Cancel() override;

	UPROPERTY(BlueprintAssignable, Category="GIS|Async")
	FSigilWaitFragmentStateChangedSignature OnStateChanged;

protected:
	UFUNCTION()
	void OnFragmentStateChanged(const USigilItemFragment* Fragment, const FInstancedStruct& State);

	TWeakObjectPtr<USigilItemInstance> ItemInstance;

	TSubclassOf<USigilItemFragment> FragmentClass;
};
