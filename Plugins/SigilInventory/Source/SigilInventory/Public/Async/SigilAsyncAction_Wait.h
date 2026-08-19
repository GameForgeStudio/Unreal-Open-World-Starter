// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilInventoryLogChannels.h"
#include "Engine/TimerHandle.h"
#include "Engine/Engine.h"
#include "UObject/Object.h"
#include "Engine/CancellableAsyncAction.h"
#include "SigilAsyncAction_Wait.generated.h"

/**
 * Delegate triggered when an async wait action completes or is cancelled.
 * 异步等待动作完成或取消时触发的委托。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSigilAsyncAction_WaitSignature);

/**
 * Base class for asynchronous wait actions on an actor.
 * 在演员上执行异步等待动作的基类。
 * @details Provides functionality to wait for specific conditions on an actor, with timer-based checks.
 * @细节 提供在演员上等待特定条件的功能，通过定时器检查。
 */
UCLASS(Abstract)
class SIGILINVENTORY_API USigilAsyncAction_Wait : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the async wait action.
	 * 异步等待动作的构造函数。
	 */
	USigilAsyncAction_Wait();

	/**
	 * Delegate triggered when the wait action completes successfully.
	 * 等待动作成功完成时触发的委托。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilAsyncAction_WaitSignature OnCompleted;

	/**
	 * Delegate triggered when the wait action is cancelled.
	 * 等待动作取消时触发的委托。
	 */
	UPROPERTY(BlueprintAssignable)
	FSigilAsyncAction_WaitSignature OnCancelled;

	/**
	 * Gets the world associated with this action.
	 * 获取与此动作关联的世界。
	 * @return The world object, or nullptr if not set. 世界对象，如果未设置则返回nullptr。
	 */
	virtual UWorld* GetWorld() const override;

	/**
	 * Gets the target actor for the wait action.
	 * 获取等待动作的目标演员。
	 * @return The target actor, or nullptr if not set. 目标演员，如果未设置则返回nullptr。
	 */
	virtual AActor* GetActor() const;

	/**
	 * Activates the wait action, starting the timer.
	 * 激活等待动作，启动定时器。
	 */
	virtual void Activate() override;

	/**
	 * Completes the wait action, triggering the OnCompleted delegate.
	 * 完成等待动作，触发OnCompleted委托。
	 */
	virtual void Complete();

	/**
	 * Cancels the wait action, triggering the OnCancelled delegate.
	 * 取消等待动作，触发OnCancelled委托。
	 */
	virtual void Cancel() override;

	/**
	 * Determines whether delegates should be broadcast.
	 * 确定是否应广播委托。
	 * @return True if delegates should be broadcast, false otherwise. 如果应广播委托则返回true，否则返回false。
	 */
	virtual bool ShouldBroadcastDelegates() const override;

	/**
	 * Called when the target actor is destroyed.
	 * 目标演员销毁时调用。
	 * @param DestroyedActor The actor that was destroyed. 被销毁的演员。
	 */
	UFUNCTION()
	virtual void OnTargetDestroyed(AActor* DestroyedActor);

protected:
	/**
	 * Creates a new wait action instance.
	 * 创建新的等待动作实例。
	 * @param WorldContext The world context object to get the world reference. 用于获取世界引用的世界上下文对象。
	 * @param TargetActor The target actor to wait for. 要等待的目标演员。
	 * @param WaitInterval The interval between checks (in seconds). 检查间隔（以秒为单位）。
	 * @param MaxWaitTimes The maximum number of checks before timeout (-1 for no limit). 最大检查次数，超时前（-1表示无限制）。
	 * @return The created wait action, or nullptr if invalid parameters. 创建的等待动作，如果参数无效则返回nullptr。
	 * @details Logs warnings if the world context, world, target actor, or wait interval is invalid.
	 * @细节 如果世界上下文、世界、目标演员或等待间隔无效，则记录警告。
	 */
	template <typename ActionType = USigilAsyncAction_Wait>
	static ActionType* CreateWaitAction(UObject* WorldContext, AActor* TargetActor, float WaitInterval, int32 MaxWaitTimes)
	{
		if (!IsValid(WorldContext))
		{
			SIGIL_INVENTORY_LOG(Warning, "invalid world context!")
			return nullptr;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
		if (!IsValid(World))
		{
			SIGIL_INVENTORY_LOG(Warning, "can't get world from context:%s", *GetNameSafe(WorldContext));
			return nullptr;
		}

		if (!IsValid(TargetActor))
		{
			SIGIL_INVENTORY_LOG(Warning, "invalid target actor.");
			return nullptr;
		}

		if (WaitInterval <= 0.f)
		{
			SIGIL_INVENTORY_LOG(Warning, "WaitInterval %f must be greater than zero!", WaitInterval);
			return nullptr;
		}

		ActionType* NewAction = Cast<ActionType>(NewObject<USigilAsyncAction_Wait>(GetTransientPackage(), ActionType::StaticClass()));
		NewAction->SetWorld(World);
		NewAction->SetTargetActor(TargetActor);
		NewAction->SetWaitInterval(WaitInterval);
		NewAction->SetMaxWaitTimes(MaxWaitTimes);
		NewAction->RegisterWithGameInstance(World->GetGameInstance());
		return NewAction;
	}

	/**
	 * Sets the world for the wait action.
	 * 设置等待动作的世界。
	 * @param NewWorld The world to set. 要设置的世界。
	 */
	void SetWorld(UWorld* NewWorld);

	/**
	 * Sets the target actor for the wait action.
	 * 设置等待动作的目标演员。
	 * @param NewTargetActor The target actor to set. 要设置的目标演员。
	 */
	void SetTargetActor(AActor* NewTargetActor);

	/**
	 * Sets the interval between checks.
	 * 设置检查间隔。
	 * @param NewWaitInterval The interval (in seconds). 间隔（以秒为单位）。
	 */
	void SetWaitInterval(float NewWaitInterval);

	/**
	 * Sets the maximum number of checks before timeout.
	 * 设置超时前的最大检查次数。
	 * @param NewMaxWaitTimes The maximum number of checks (-1 for no limit). 最大检查次数（-1表示无限制）。
	 */
	void SetMaxWaitTimes(int32 NewMaxWaitTimes);

	/**
	 * Stops the timer for the wait action.
	 * 停止等待动作的定时器。
	 */
	void StopWaiting();

	/**
	 * Called when the timer ticks to check the wait condition.
	 * 定时器触发时调用以检查等待条件。
	 */
	UFUNCTION()
	void OnTimer();

	/**
	 * Cleans up resources used by the wait action.
	 * 清理等待动作使用的资源。
	 */
	virtual void Cleanup();

	/**
	 * Executes the specific wait condition check.
	 * 执行特定的等待条件检查。
	 */
	UFUNCTION()
	virtual void OnExecutionAction();

private:
	/**
	 * Weak reference to the world for the wait action.
	 * 等待动作的世界的弱引用。
	 */
	TWeakObjectPtr<UWorld> WorldPtr{nullptr};

	/**
	 * Weak reference to the target actor for the wait action.
	 * 等待动作的目标演员的弱引用。
	 */
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActorPtr{nullptr};

	/**
	 * Handle for the timer used to check the wait condition.
	 * 用于检查等待条件的定时器句柄。
	 */
	UPROPERTY()
	FTimerHandle TimerHandle;

	/**
	 * Interval between checks (in seconds).
	 * 检查间隔（以秒为单位）。
	 */
	float WaitInterval = 0.2f;

	/**
	 * Current number of checks performed.
	 * 当前执行的检查次数。
	 */
	int32 WaitTimes = 0;

	/**
	 * Maximum number of checks before timeout (-1 for no limit).
	 * 超时前的最大检查次数（-1表示无限制）。
	 */
	int32 MaxWaitTimes{-1};
};
