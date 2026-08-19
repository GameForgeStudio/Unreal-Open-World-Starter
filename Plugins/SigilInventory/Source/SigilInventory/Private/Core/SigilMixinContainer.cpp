// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilMixinContainer.h"
#include "Engine/World.h"
#include "AssetRegistry/AssetData.h"
#include "SigilItemFragment.h"
#include "SigilInventoryLogChannels.h"
#include "SigilMixinOwnerInterface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Items/SigilItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilMixinContainer)

uint32 GetTypeHash(const FSigilMixin& Entry)
{
	return HashCombine(GetTypeHash(Entry.Target), Entry.Timestamp);
}

int32 FSigilMixinContainer::IndexOfTarget(const UObject* Target) const
{
	if (!IsValid(Target))
	{
		return INDEX_NONE;
	}
	return AcceleratedMap.Contains(Target->GetClass()) ? AcceleratedMap[Target->GetClass()] : INDEX_NONE;
}

int32 FSigilMixinContainer::IndexOfTargetByClass(const TSubclassOf<UObject>& TargetClass) const
{
	check(IsValid(TargetClass));

	const int32* Idx = AcceleratedMap.Find(TargetClass);
	if (Idx != nullptr)
	{
		return *Idx;
	}

	return INDEX_NONE;
}

// bool FSigilMixinContainer::GetDataByTargetClass(const TSubclassOf<UObject>& TargetClass, FInstancedStruct& OutData) const
// {
// 	if (AcceleratedMap.Contains(TargetClass) && Mixins.IsValidIndex(AcceleratedMap[TargetClass]))
// 	{
// 		OutData = Mixins[AcceleratedMap[TargetClass]].Data;
// 		return true;
// 	}
// 	return false;
// }

bool FSigilMixinRecord::operator==(const FSigilMixinRecord& Other) const
{
	return TargetPath == Other.TargetPath && Data.GetScriptStruct() == Other.Data.GetScriptStruct();
}

bool FSigilMixinRecord::IsValid() const
{
	return !TargetPath.IsEmpty() && Data.IsValid();
}

bool FSigilMixinContainer::GetDataByTarget(const UObject* Target, FInstancedStruct& OutData) const
{
	if (AcceleratedMap.Contains(Target) && Mixins.IsValidIndex(AcceleratedMap[Target]))
	{
		OutData = Mixins[AcceleratedMap[Target]].Data;
		return true;
	}
	return false;
}

int32 FSigilMixinContainer::SetDataForTarget(const TObjectPtr<const UObject>& Target, const FInstancedStruct& Data)
{
	if (Target == nullptr || !Data.IsValid())
	{
		return INDEX_NONE;
	}

	if (!IsObjectLoadedFromDisk(Target))
	{
		return INDEX_NONE;
	}

	if (AcceleratedMap.Contains(Target))
	{
		return UpdateDataAt(AcceleratedMap[Target], Data);
	}

	// is not valid class -> data pair.
	if (!CheckCompatibility(Target, Data))
	{
		return INDEX_NONE;
	}

	int32 Idx = Mixins.AddDefaulted();
	FSigilMixin& NewMixin = Mixins[Idx];
	NewMixin.Target = Target;
	NewMixin.Data = Data;
	NewMixin.Timestamp = OwningObject->GetWorld()->GetTimeSeconds();
	if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
	{
		MixinOwner->OnMixinDataAdded(NewMixin.Target, NewMixin.Data);
	}
	MarkItemDirty(NewMixin);
	CacheMixins();

	return Idx;
}

bool FSigilMixinContainer::IsObjectLoadedFromDisk(const UObject* Object) const
{
	if (!IsValid(Object))
	{
		return false;
	}

	// 获取资源路径
	FSoftObjectPath AssetPath(Object);
	if (AssetPath.IsNull())
	{
		return false;
	}

	// 检查资产注册表
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(AssetPath);
	return AssetData.IsValid();
}

int32 FSigilMixinContainer::UpdateDataByTargetClass(const TSubclassOf<UObject>& TargetClass, const FInstancedStruct& Data)
{
	if (AcceleratedMap.Contains(TargetClass))
	{
		return UpdateDataAt(AcceleratedMap[TargetClass], Data);
	}
	return INDEX_NONE;
}

int32 FSigilMixinContainer::UpdateDataAt(const int32 Idx, const FInstancedStruct& Data)
{
	if (Idx == INDEX_NONE || !Mixins.IsValidIndex(Idx))
	{
		return INDEX_NONE;
	}

	FSigilMixin& Entry = Mixins[Idx];

	if (!CheckCompatibility(Entry.Target, Data))
	{
		return INDEX_NONE;
	}

	Entry.Data = Data;
	Entry.Timestamp = OwningObject->GetWorld()->GetTimeSeconds();
	if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
	{
		MixinOwner->OnMixinDataUpdated(Entry.Target, Entry.Data);
	}
	MarkItemDirty(Entry);
	CacheMixins();

	return Idx;
}

void FSigilMixinContainer::RemoveDataByTargetClass(const TSubclassOf<UObject>& TargetClass)
{
	const int32 Idx = AcceleratedMap.Contains(TargetClass) ? AcceleratedMap[TargetClass] : INDEX_NONE;

	if (Idx != INDEX_NONE)
	{
		const FSigilMixin& Entry = Mixins[Idx];
		if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
		{
			MixinOwner->OnMixinDataRemoved(Entry.Target, Entry.Data);
		}
		Mixins.RemoveAt(Idx);
		MarkArrayDirty();
	}

	CacheMixins();
}

bool FSigilMixinContainer::CheckCompatibility(const UObject* Target, const FInstancedStruct& Data) const
{
	if (!IsValid(Target) || !Data.IsValid())
	{
		return false;
	}
	if (const ISigilMixinTargetInterface* MixinTarget = Cast<ISigilMixinTargetInterface>(Target))
	{
		return MixinTarget->GetCompatibleMixinDataType() == Data.GetScriptStruct();
	}
	return false;
}

TArray<FInstancedStruct> FSigilMixinContainer::GetAllData() const
{
	TArray<FInstancedStruct> AllData;
	AllData.Reserve(Mixins.Num());

	for (const FSigilMixin& Mixin : Mixins)
	{
		AllData.Add(Mixin.Data);
	}

	return AllData;
}

TArray<FSigilMixin> FSigilMixinContainer::GetSerializableMixins() const
{
	return Mixins.FilterByPredicate([this](const FSigilMixin& Mixin)
	{
		if (const ISigilMixinTargetInterface* MixinTarget = Cast<ISigilMixinTargetInterface>(Mixin.Target))
		{
			return MixinTarget->IsMixinDataSerializable() && Mixin.Data.IsValid() && MixinTarget->GetCompatibleMixinDataType() == Mixin.Data.GetScriptStruct() && IsObjectLoadedFromDisk(Mixin.Target);
		}
		return false;
	});
}

TArray<FSigilMixinRecord> FSigilMixinContainer::GetSerializableMixinRecords() const
{
	TArray<FSigilMixinRecord> Records;
	TArray<FSigilMixin> FilteredMixins = GetSerializableMixins();

	for (const FSigilMixin& FilteredMixin : FilteredMixins)
	{
		FSigilMixinRecord Record;
		const FSoftObjectPath AssetPath = FSoftObjectPath(FilteredMixin.Target);
		Record.TargetPath = AssetPath.ToString();
		Record.Data = FilteredMixin.Data;
		Records.Add(Record);
	}

	return Records;
}

void FSigilMixinContainer::RestoreFromRecords(const TArray<FSigilMixinRecord>& Records)
{
	TArray<FSigilMixin> ConvertedMixins = ConvertRecordsToMixins(Records);
	for (const FSigilMixin& ConvertedMixin : ConvertedMixins)
	{
		SetDataForTarget(ConvertedMixin.Target, ConvertedMixin.Data);
	}
}

TArray<FSigilMixin> FSigilMixinContainer::ConvertRecordsToMixins(const TArray<FSigilMixinRecord>& Records)
{
	TArray<FSigilMixin> Ret;
	for (const FSigilMixinRecord& Record : Records)
	{
		if (!Record.IsValid())
		{
			continue;
		}
		const FSoftObjectPath TargetPath = FSoftObjectPath(Record.TargetPath);
		const TSoftObjectPtr<UObject> TargetObjectSoftPtr = TSoftObjectPtr<UObject>(TargetPath);
		const TObjectPtr<const UObject> TargetObject = !TargetObjectSoftPtr.IsNull() ? TargetObjectSoftPtr.LoadSynchronous() : nullptr;
		if (!IsValid(TargetObject))
		{
			continue;
		}
		const ISigilMixinTargetInterface* TargetInterface = Cast<ISigilMixinTargetInterface>(TargetObject);
		if (TargetInterface == nullptr)
		{
			continue;
		}
		if (!TargetInterface->IsMixinDataSerializable())
		{
			SIGIL_INVENTORY_LOG(Warning, "Skip restoring mixin's data, as target(%s,class:%s) existed in record no longer considered serializable!",
			        *GetNameSafe(TargetObject), *GetNameSafe(TargetObject->GetClass()));
			continue;
		}

		if (TargetInterface->GetCompatibleMixinDataType() != Record.Data.GetScriptStruct())
		{
			SIGIL_INVENTORY_LOG(Warning,
			        "Skip restoring mixin's data, as target(%s,class:%s)'s data type(%s) in record no longer compatible with the new type(%s).",
			        *GetNameSafe(TargetObject), *GetNameSafe(TargetObject->GetClass()),
			        *GetNameSafe(Record.Data.GetScriptStruct()), *GetNameSafe(TargetInterface->GetCompatibleMixinDataType()));
			continue;
		}
		FSigilMixin Mixin;
		Mixin.Target = TargetObject;
		Mixin.Data = Record.Data;
		Ret.Add(Mixin);
	}
	return Ret;
}

TArray<FInstancedStruct> FSigilMixinContainer::GetAllSerializableData() const
{
	TArray<FInstancedStruct> AllData;
	AllData.Reserve(Mixins.Num());

	for (const FSigilMixin& Mixin : Mixins)
	{
		if (const ISigilMixinTargetInterface* MixinTarget = Cast<ISigilMixinTargetInterface>(Mixin.Target))
		{
			if (MixinTarget->IsMixinDataSerializable())
			{
				AllData.Add(Mixin.Data);
			}
		}
	}

	return AllData;
}

void FSigilMixinContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		FSigilMixin& Mixin = Mixins[Index];
		if (Mixin.Timestamp != Mixin.LastReplicatedTimestamp)
		{
			Mixin.LastReplicatedTimestamp = Mixin.Timestamp;
			if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
			{
				MixinOwner->OnMixinDataAdded(Mixin.Target, Mixin.Data);
			}
		}
	}

	CacheMixins();
}

void FSigilMixinContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		FSigilMixin& Mixin = Mixins[Index];
		if (Mixin.Timestamp != Mixin.LastReplicatedTimestamp)
		{
			Mixin.LastReplicatedTimestamp = Mixin.Timestamp;
			if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
			{
				MixinOwner->OnMixinDataUpdated(Mixin.Target, Mixin.Data);
			}
		}
	}

	CacheMixins();
}

void FSigilMixinContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		const FSigilMixin& Mixin = Mixins[Index];
		if (ISigilMixinOwnerInterface* MixinOwner = Cast<ISigilMixinOwnerInterface>(OwningObject))
		{
			MixinOwner->OnMixinDataRemoved(Mixin.Target, Mixin.Data);
		}
	}

	CacheMixins();
}

void FSigilMixinContainer::CacheMixins()
{
	const uint32 MixinsHash = GetTypeHash(Mixins);
	if (MixinsHash == LastCachedHash)
	{
		// Same hash, no need to cache things again.
		return;
	}

	const int32 Size = Mixins.Num();
	AcceleratedMap.Empty(Size);

	for (int Idx = 0; Idx < Size; ++Idx)
	{
		const UObject* Target = Mixins[Idx].Target;
		AcceleratedMap.Add(Target, Idx);
	}

	LastCachedHash = GetTypeHash(Mixins);
}

bool FSigilMixinContainer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FastArrayDeltaSerialize<FSigilMixin, FSigilMixinContainer>(Mixins, DeltaParams, *this);
}
