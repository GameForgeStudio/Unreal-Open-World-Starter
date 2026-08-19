// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Async/SigilAsyncAction_Wait.h"
#include "TimerManager.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilAsyncAction_Wait)


USigilAsyncAction_Wait::USigilAsyncAction_Wait()
{
}

bool USigilAsyncAction_Wait::ShouldBroadcastDelegates() const
{
	return Super::ShouldBroadcastDelegates() && IsValid(GetActor());
}

void USigilAsyncAction_Wait::StopWaiting()
{
	const UWorld* World = GetWorld();
	if (TimerHandle.IsValid() && IsValid(World))
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TimerHandle);
	}
}

void USigilAsyncAction_Wait::Cleanup()
{
	AActor* Actor = GetActor();

	if (IsValid(Actor))
	{
		Actor->OnDestroyed.RemoveDynamic(this, &ThisClass::OnTargetDestroyed);
	}

	StopWaiting();
}

void USigilAsyncAction_Wait::Activate()
{
	const UWorld* World = GetWorld();
	AActor* Actor = GetActor();
	if (IsValid(World) && IsValid(Actor))
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.SetTimer(TimerHandle, this, &ThisClass::OnTimer, WaitInterval, true, 0);
		Actor->OnDestroyed.AddDynamic(this, &ThisClass::OnTargetDestroyed);
	}
	else
	{
		Cancel();
	}
}

UWorld* USigilAsyncAction_Wait::GetWorld() const
{
	if (WorldPtr.IsValid() && WorldPtr->IsValidLowLevelFast())
	{
		return WorldPtr.Get();
	}

	return nullptr;
}

AActor* USigilAsyncAction_Wait::GetActor() const
{
	if (TargetActorPtr.IsValid() && TargetActorPtr->IsValidLowLevelFast())
	{
		return TargetActorPtr.Get();
	}

	return nullptr;
}

void USigilAsyncAction_Wait::Cancel()
{
	Super::Cancel();

	Cleanup();
}

void USigilAsyncAction_Wait::OnTargetDestroyed(AActor* DestroyedActor)
{
	Cancel();
}

void USigilAsyncAction_Wait::SetWorld(UWorld* NewWorld)
{
	WorldPtr = NewWorld;
}

void USigilAsyncAction_Wait::SetTargetActor(AActor* NewTargetActor)
{
	TargetActorPtr = NewTargetActor;
}

void USigilAsyncAction_Wait::SetWaitInterval(float NewWaitInterval)
{
	WaitInterval = NewWaitInterval;
}

void USigilAsyncAction_Wait::SetMaxWaitTimes(int32 NewMaxWaitTimes)
{
	MaxWaitTimes = NewMaxWaitTimes;
}

void USigilAsyncAction_Wait::OnTimer()
{
	AActor* Actor = GetActor();
	if (!IsValid(Actor))
	{
		Cancel();
		return;
	}

	OnExecutionAction();

	if (MaxWaitTimes > 0)
	{
		WaitTimes++;
		if (WaitTimes > MaxWaitTimes)
		{
			Cancel();
		}
	}
}

void USigilAsyncAction_Wait::OnExecutionAction()
{
}

void USigilAsyncAction_Wait::Complete()
{
	Super::Cancel();
	OnCompleted.Broadcast();
	Cleanup();
}
