// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Exchange/SigilCurrencySystemComponent.h"
#include "Engine/World.h"
#include "SigilInventorySubsystem.h"
#include "GameFramework/Actor.h"
#include "UObject/Object.h"
#include "SigilInventoryTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencySystemComponent)

USigilCurrencySystemComponent::USigilCurrencySystemComponent(): Container(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bWantsInitializeComponent = true;
}

USigilCurrencySystemComponent* USigilCurrencySystemComponent::GetCurrencySystemComponent(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<USigilCurrencySystemComponent>() : nullptr;
}

void USigilCurrencySystemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Container);
}

void USigilCurrencySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return;
	}

	Container.OwningComponent = this;
}

TArray<FSigilCurrencyEntry> USigilCurrencySystemComponent::GetAllCurrencies() const
{
	TArray<FSigilCurrencyEntry> Ret;
	for (const FSigilCurrencyEntry& Item : Container.Entries)
	{
		Ret.Add(FSigilCurrencyEntry(Item.Definition, Item.Amount));
	}
	return Ret;
}

void USigilCurrencySystemComponent::SetCurrencies(const TArray<FSigilCurrencyEntry>& InCurrencyInfos)
{
	TArray<FSigilCurrencyEntry> NewEntries = InCurrencyInfos.FilterByPredicate([](const FSigilCurrencyEntry& Item)
	{
		return Item.Definition != nullptr && Item.Amount > 0;
	});

	Container.Entries.Empty();
	CurrencyMap.Empty();
	Container.Entries = NewEntries;
	for (const FSigilCurrencyEntry& NewItem : NewEntries)
	{
		CurrencyMap.Add(NewItem.Definition, NewItem.Amount);
	}
	Container.MarkArrayDirty();
}

void USigilCurrencySystemComponent::EmptyCurrencies()
{
	Container.Entries.Empty();
	CurrencyMap.Empty();
	Container.MarkArrayDirty();
}

bool USigilCurrencySystemComponent::GetCurrency(TSoftObjectPtr<const USigilCurrencyDefinition> CurrencyDefinition, FSigilCurrencyEntry& OutCurrencyInfo) const
{
	if (CurrencyDefinition.IsNull())
	{
		return false;
	}

	const USigilCurrencyDefinition* Definition = CurrencyDefinition.LoadSynchronous();

	return GetCurrencyInternal(Definition, OutCurrencyInfo);
}

bool USigilCurrencySystemComponent::GetCurrencies(TArray<TSoftObjectPtr<USigilCurrencyDefinition>> CurrencyDefinitions, TArray<FSigilCurrencyEntry>& OutCurrencyInfos) const
{
	TArray<TObjectPtr<const USigilCurrencyDefinition>> Definitions;
	for (TSoftObjectPtr<const USigilCurrencyDefinition> Currency : CurrencyDefinitions)
	{
		if (const USigilCurrencyDefinition* Definition = !Currency.IsNull() ? Currency.LoadSynchronous() : nullptr)
		{
			Definitions.AddUnique(Definition);
		}
	}
	return GetCurrenciesInternal(Definitions, OutCurrencyInfos);
}

bool USigilCurrencySystemComponent::AddCurrency(FSigilCurrencyEntry CurrencyInfo)
{
	return AddCurrencyInternal(CurrencyInfo);
}

bool USigilCurrencySystemComponent::RemoveCurrency(FSigilCurrencyEntry CurrencyInfo)
{
	return RemoveCurrencyInternal(CurrencyInfo);
}

bool USigilCurrencySystemComponent::HasCurrency(FSigilCurrencyEntry CurrencyInfo) const
{
	return HasCurrencyInternal(CurrencyInfo);
}

bool USigilCurrencySystemComponent::HasCurrencies(const TArray<FSigilCurrencyEntry>& CurrencyInfos)
{
	bool bOk = true;
	for (auto& Currency : CurrencyInfos)
	{
		if (!HasCurrencyInternal(Currency))
		{
			bOk = false;
			break;
		}
	}
	return bOk;
}

bool USigilCurrencySystemComponent::AddCurrencies(const TArray<FSigilCurrencyEntry>& CurrencyInfos)
{
	for (const FSigilCurrencyEntry& Currency : CurrencyInfos)
	{
		AddCurrencyInternal(Currency);
	}
	return true;
}

bool USigilCurrencySystemComponent::RemoveCurrencies(const TArray<FSigilCurrencyEntry>& CurrencyInfos)
{
	for (const FSigilCurrencyEntry& Currency : CurrencyInfos)
	{
		RemoveCurrencyInternal(Currency);
	}
	return true;
}

void USigilCurrencySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	AddInitialCurrencies();
}

void USigilCurrencySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USigilCurrencySystemComponent::OnCurrencyEntryAdded(const FSigilCurrencyEntry& Entry, int32 Idx)
{
	CurrencyMap.Add(Entry.Definition, Entry.Amount);
	OnCurrencyChangedEvent.Broadcast(Entry.Definition, 0, Entry.Amount);
}

void USigilCurrencySystemComponent::OnCurrencyEntryRemoved(const FSigilCurrencyEntry& Entry, int32 Idx)
{
	CurrencyMap.Remove(Entry.Definition);
	OnCurrencyChangedEvent.Broadcast(Entry.Definition, Entry.PrevAmount, 0);
}

void USigilCurrencySystemComponent::OnCurrencyEntryUpdated(const FSigilCurrencyEntry& Entry, int32 Idx, float OldAmount, float NewAmount)
{
	CurrencyMap.Emplace(Entry.Definition, NewAmount);
	OnCurrencyChangedEvent.Broadcast(Entry.Definition, OldAmount, NewAmount);
}


void USigilCurrencySystemComponent::OnCurrencyChanged(TObjectPtr<const USigilCurrencyDefinition> Currency, float OldValue, float NewValue)
{
	OnCurrencyChangedEvent.Broadcast(Currency, OldValue, NewValue);
}


void USigilCurrencySystemComponent::AddInitialCurrencies_Implementation()
{
	if (GetOwner()->HasAuthority())
	{
		//TODO check records from save game.
		if (!DefaultCurrencies.IsEmpty())
		{
			AddCurrencies(DefaultCurrencies);
		}
	}
}

bool USigilCurrencySystemComponent::GetCurrencyInternal(const TObjectPtr<const USigilCurrencyDefinition>& Currency, FSigilCurrencyEntry& OutCurrencyInfo) const
{
	if (CurrencyMap.Contains(Currency))
	{
		OutCurrencyInfo = FSigilCurrencyEntry(Currency, CurrencyMap[Currency]);
		return true;
	}
	return false;
}

bool USigilCurrencySystemComponent::GetCurrenciesInternal(const TArray<TObjectPtr<const USigilCurrencyDefinition>>& Currencies, TArray<FSigilCurrencyEntry>& OutCurrencies) const
{
	TArray<FSigilCurrencyEntry> Result;
	for (int32 i = 0; i < Currencies.Num(); i++)
	{
		FSigilCurrencyEntry Info;
		if (GetCurrencyInternal(Currencies[i], Info))
		{
			Result.Add(Info);
		}
	}

	OutCurrencies = Result;

	return !OutCurrencies.IsEmpty();
}

bool USigilCurrencySystemComponent::AddCurrencyInternal(const FSigilCurrencyEntry& CurrencyInfo, bool bNotify)
{
	if (!CurrencyInfo.Definition || CurrencyInfo.Amount <= 0)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid currency definition was passed."), ELogVerbosity::Warning);
		return false;
	}

	for (int32 i = 0; i < Container.Entries.Num(); i++)
	{
		// handle adding to existing value.
		FSigilCurrencyEntry& Entry = Container.Entries[i];
		if (Entry.Definition == CurrencyInfo.Definition)
		{
			const float OldValue = Entry.Amount;
			const float NewValue = Entry.Amount + CurrencyInfo.Amount;
			Entry.Amount = NewValue;
			OnCurrencyEntryUpdated(Entry, i, OldValue, NewValue);
			Container.MarkItemDirty(Entry);
			return true;
		}
	}

	int32 Idx = Container.Entries.AddDefaulted();
	Container.Entries[Idx] = CurrencyInfo;
	OnCurrencyEntryAdded(Container.Entries[Idx], Idx);
	Container.MarkItemDirty(Container.Entries[Idx]);
	return true;
}

bool USigilCurrencySystemComponent::RemoveCurrencyInternal(const FSigilCurrencyEntry& CurrencyInfo, bool bNotify)
{
	if (!CurrencyInfo.Definition || CurrencyInfo.Amount <= 0)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveItem"), ELogVerbosity::Warning);
		return false;
	}

	for (int32 i = 0; i < Container.Entries.Num(); i++)
	{
		FSigilCurrencyEntry& Entry = Container.Entries[i];
		if (Entry.Definition == CurrencyInfo.Definition)
		{
			if (Entry.Amount <= CurrencyInfo.Amount)
			{
				OnCurrencyEntryRemoved(Entry, i);
				Container.Entries.RemoveAt(i);
				Container.MarkArrayDirty();
			}
			else
			{
				const float OldValue = Entry.Amount;
				const float NewValue = Entry.Amount - CurrencyInfo.Amount;
				Entry.Amount = NewValue;
				OnCurrencyEntryUpdated(Entry, i, OldValue, NewValue);
				Container.MarkItemDirty(Entry);
			}
			return true;
		}
	}
	return false;
}

bool USigilCurrencySystemComponent::HasCurrencyInternal(const FSigilCurrencyEntry& CurrencyInfo) const
{
	if (CurrencyMap.Contains(CurrencyInfo.Definition) && CurrencyInfo.Amount > 0)
	{
		return CurrencyMap[CurrencyInfo.Definition] >= CurrencyInfo.Amount;
	}
	return false;
}
