// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Async/SigilAsyncAction_WaitEquipmentSystem.h"

#include "SigilEquipmentSystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilAsyncAction_WaitEquipmentSystem)

USigilAsyncAction_WaitEquipmentSystem* USigilAsyncAction_WaitEquipmentSystem::WaitEquipmentSystem(UObject* WorldContext, AActor* TargetActor)
{
	return CreateWaitAction<USigilAsyncAction_WaitEquipmentSystem>(WorldContext, TargetActor, 0.5, -1);
}

void USigilAsyncAction_WaitEquipmentSystem::OnExecutionAction()
{
	AActor* Actor = GetActor();

	if (USigilEquipmentSystemComponent* EquipmentSystem = USigilEquipmentSystemComponent::GetEquipmentSystemComponent(Actor))
	{
		Complete();
	}
}

USigilAsyncAction_WaitEquipmentSystem* USigilAsyncAction_WaitEquipmentSystemInitialized::WaitEquipmentSystemInitialized(UObject* WorldContext, AActor* TargetActor)
{
	return CreateWaitAction<USigilAsyncAction_WaitEquipmentSystemInitialized>(WorldContext, TargetActor, 0.5, -1);
}

void USigilAsyncAction_WaitEquipmentSystemInitialized::OnExecutionAction()
{
	// Already found.
	USigilEquipmentSystemComponent* ExistingOne = EquipmentSystemPtr.IsValid() ? EquipmentSystemPtr.Get() : nullptr;
	if (IsValid(ExistingOne))
	{
		return;
	}

	AActor* Actor = GetActor();
	if (USigilEquipmentSystemComponent* EquipmentSys = USigilEquipmentSystemComponent::GetEquipmentSystemComponent(Actor))
	{
		if (EquipmentSys->IsEquipmentSystemInitialized())
		{
			Complete();
			return;
		}
		EquipmentSystemPtr = EquipmentSys;
		EquipmentSystemPtr->OnEquipmentSystemInitializedEvent.AddDynamic(this, &ThisClass::OnSystemInitialized);
	}
}

void USigilAsyncAction_WaitEquipmentSystemInitialized::Cleanup()
{
	Super::Cleanup();
	USigilEquipmentSystemComponent* ExistingOne = EquipmentSystemPtr.IsValid() ? EquipmentSystemPtr.Get() : nullptr;

	if (IsValid(ExistingOne))
	{
		ExistingOne->OnEquipmentSystemInitializedEvent.RemoveAll(this);
	}
}

void USigilAsyncAction_WaitEquipmentSystemInitialized::OnSystemInitialized()
{
	Complete();
}
