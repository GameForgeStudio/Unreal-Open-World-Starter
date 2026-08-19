// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Async/SigilAsyncAction_WaitItemFragmentDataChanged.h"
#include "Engine/Engine.h"
#include "UObject/Object.h"
#include "SigilItemFragment.h"
#include "SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilAsyncAction_WaitItemFragmentDataChanged)

USigilAsyncAction_WaitItemFragmentDataChanged* USigilAsyncAction_WaitItemFragmentDataChanged::WaitItemFragmentStateChanged(UObject* WorldContext, USigilItemInstance* ItemInstance,
                                                                                                                         TSoftClassPtr<USigilItemFragment> FragmentClass)
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

	if (!IsValid(ItemInstance))
	{
		SIGIL_INVENTORY_LOG(Warning, "invalid item instance");
		return nullptr;
	}

	TSubclassOf<USigilItemFragment> Class = FragmentClass.LoadSynchronous();
	if (Class == nullptr)
	{
		SIGIL_INVENTORY_LOG(Warning, "invalid fragment class");
		return nullptr;
	}

	USigilAsyncAction_WaitItemFragmentDataChanged* NewAction = NewObject<USigilAsyncAction_WaitItemFragmentDataChanged>(GetTransientPackage(), StaticClass());
	NewAction->ItemInstance = ItemInstance;
	NewAction->FragmentClass = Class;
	NewAction->RegisterWithGameInstance(World->GetGameInstance());
	return NewAction;
}

void USigilAsyncAction_WaitItemFragmentDataChanged::Activate()
{
	USigilItemInstance* Item = ItemInstance.IsValid() ? ItemInstance.Get() : nullptr;
	if (IsValid(Item))
	{
		Item->OnFragmentStateAddedEvent.AddDynamic(this, &ThisClass::OnFragmentStateChanged);
		Item->OnFragmentStateUpdatedEvent.AddDynamic(this, &ThisClass::OnFragmentStateChanged);
	}
}

void USigilAsyncAction_WaitItemFragmentDataChanged::Cancel()
{
	Super::Cancel();
	USigilItemInstance* Item = ItemInstance.IsValid() ? ItemInstance.Get() : nullptr;
	if (IsValid(Item))
	{
		Item->OnFragmentStateAddedEvent.RemoveAll(this);
		Item->OnFragmentStateUpdatedEvent.RemoveAll(this);
		ItemInstance.Reset();
		FragmentClass = nullptr;
	}
}

void USigilAsyncAction_WaitItemFragmentDataChanged::OnFragmentStateChanged(const USigilItemFragment* Fragment, const FInstancedStruct& State)
{
	if (ShouldBroadcastDelegates())
	{
		if (Fragment != nullptr && Fragment->GetClass() == FragmentClass)
		{
			OnStateChanged.Broadcast(Fragment, State);
		}
	}
}
