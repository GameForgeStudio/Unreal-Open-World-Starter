// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Async/SigilAsyncAction_WaitInventorySystem.h"

#include "SigilInventorySystemComponent.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilAsyncAction_WaitInventorySystem)

USigilAsyncAction_WaitInventorySystem* USigilAsyncAction_WaitInventorySystem::WaitInventorySystem(UObject* WorldContext, AActor* TargetActor)
{
	return CreateWaitAction<USigilAsyncAction_WaitInventorySystem>(WorldContext, TargetActor, 0.5, -1);
}

void USigilAsyncAction_WaitInventorySystem::OnExecutionAction()
{
	AActor* Actor = GetActor();

	if (USigilInventorySystemComponent* Inventory = USigilInventorySystemComponent::GetInventorySystemComponent(Actor))
	{
		Complete();
	}
}

USigilAsyncAction_WaitInventorySystem* USigilAsyncAction_WaitInventorySystemInitialized::WaitInventorySystemInitialized(UObject* WorldContext, AActor* TargetActor)
{
	return CreateWaitAction<USigilAsyncAction_WaitInventorySystemInitialized>(WorldContext, TargetActor, 0.5, -1);
}

void USigilAsyncAction_WaitInventorySystemInitialized::OnExecutionAction()
{
	// Already found.
	USigilInventorySystemComponent* ExistingOne = InventorySysPtr.IsValid() ? InventorySysPtr.Get() : nullptr;
	if (IsValid(ExistingOne))
	{
		return;
	}

	AActor* Actor = GetActor();
	if (USigilInventorySystemComponent* InventorySys = USigilInventorySystemComponent::GetInventorySystemComponent(Actor))
	{
		if (InventorySys->IsInventoryInitialized())
		{
			Complete();
			return;
		}
		InventorySysPtr = InventorySys;
		InventorySysPtr->OnInventorySystemInitializedEvent.AddDynamic(this, &ThisClass::OnSystemInitialized);
	}
}

void USigilAsyncAction_WaitInventorySystemInitialized::Cleanup()
{
	Super::Cleanup();
	USigilInventorySystemComponent* ExistingOne = InventorySysPtr.IsValid() ? InventorySysPtr.Get() : nullptr;
	if (IsValid(ExistingOne))
	{
		ExistingOne->OnInventorySystemInitializedEvent.RemoveAll(this);
	}
}

void USigilAsyncAction_WaitInventorySystemInitialized::OnSystemInitialized()
{
	Complete();
}
