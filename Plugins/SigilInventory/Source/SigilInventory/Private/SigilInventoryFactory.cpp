// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilInventoryFactory.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "SigilCollectionContainer.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "Items/SigilItemDefinition.h"
#include "SigilItemFragment.h"
#include "SigilInventoryLogChannels.h"
#include "Items/SigilItemInstance.h"
#include "Misc/DataValidation.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilInventoryFactory)

USigilItemInstance* USigilInventoryFactory::DuplicateItem_Implementation(AActor* Owner, USigilItemInstance* SrcItem, bool bGenerateNewId)
{
	if (!IsValid(SrcItem) || SrcItem->GetDefinition() == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "Missing src item or src item doesn't have valid definition.");
		return nullptr;
	}
	USigilItemInstance* NewItem = DuplicateObject(SrcItem, Owner);
	if (bGenerateNewId)
	{
		NewItem->SetItemId(FGuid::NewGuid());
	}
	NewItem->OnItemDuplicated(SrcItem);
	return NewItem;
}

USigilItemCollection* USigilInventoryFactory::CreateCollection_Implementation(AActor* Owner, const USigilItemCollectionDefinition* Definition)
{
	if (!IsValid(Owner))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing owner.");
		return nullptr;
	}
	if (!IsValid(Definition))
	{
		SIGIL_INVENTORY_LOG(Error, "Cannot create collection with null collection Definition.");
		return nullptr;
	}
	TSubclassOf<USigilItemCollection> CollectionClass = Definition->GetCollectionInstanceClass();
	if (CollectionClass == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "definition(%s) doesn't specify valid item collection class.", *Definition->GetName())
		return nullptr;
	}
	USigilItemCollection* NewCollection = NewObject<USigilItemCollection>(Owner, CollectionClass);
	if (NewCollection == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "failed to create instance of %s", *GetNameSafe(Definition))
		return nullptr;
	}
	return NewCollection;
}

USigilInventoryFactory::USigilInventoryFactory()
{
	DefaultItemInstanceClass = USigilItemInstance::StaticClass();
}

USigilItemInstance* USigilInventoryFactory::CreateItem_Implementation(AActor* Owner, const USigilItemDefinition* ItemDefinition)
{
	if (!IsValid(Owner))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing owner.");
		return nullptr;
	}

	if (ItemDefinition == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "Cannot create Item with null Item Definition.");
		return nullptr;
	}

	TSubclassOf<USigilItemInstance> ItemInstanceClass = DefaultItemInstanceClass.LoadSynchronous();

	if (ItemInstanceClass == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "ItemDefinition: %s has invalid InstanceType.", *ItemDefinition->GetName());
		return nullptr;
	}

	USigilItemInstance* Item = NewObject<USigilItemInstance>(Owner, ItemInstanceClass);
	if (Item == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "ItemInstanceClass: %s create failed.", *ItemInstanceClass->GetName());
		return nullptr;
	}

	Item->SetItemId(FGuid::NewGuid());
	Item->SetDefinition(ItemDefinition);

	for (const USigilItemFragment* Fragment : ItemDefinition->Fragments)
	{
		if (Fragment == nullptr)
		{
			continue;
		}
		if (Fragment->GetCompatibleMixinDataType() != nullptr)
		{
			FInstancedStruct FragmentState;
			if (Fragment->MakeDefaultMixinData(FragmentState))
			{
				if (FragmentState.IsValid() && FragmentState.GetScriptStruct() == Fragment->GetCompatibleMixinDataType())
				{
					Item->SetFragmentStateByClass(Fragment->GetClass(), FragmentState);
				}
			}
		}
		Fragment->OnInstanceCreated(Item);
	}
	return Item;
}

USigilItemInstance* USigilInventoryFactory::DeserializeItem_Implementation(AActor* Owner, const FSigilItemRecord& Record)
{
	if (!IsValid(Owner))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing owner.");
		return nullptr;
	}

	const FSoftObjectPath ItemDefinitionAssetPath = FSoftObjectPath(Record.DefinitionAssetPath);
	const TSoftObjectPtr<USigilItemDefinition> ItemDefinitionReference = TSoftObjectPtr<USigilItemDefinition>(ItemDefinitionAssetPath);

	USigilItemDefinition* ItemDefinition = !ItemDefinitionReference.IsNull() ? ItemDefinitionReference.LoadSynchronous() : nullptr;
	if (!IsValid(ItemDefinition))
	{
		SIGIL_INVENTORY_LOG(Warning, "invalid item definition on path:%s", *ItemDefinitionAssetPath.ToString());
		return nullptr;
	}

	USigilItemInstance* ItemInstance = CreateItem(Owner, ItemDefinition);
	if (!IsValid(ItemInstance))
	{
		SIGIL_INVENTORY_LOG(Warning, "failed to create item instance from definition:%s", *GetNameSafe(ItemDefinition));
		return nullptr;
	}

	ItemInstance->SetItemId(Record.ItemId);
	ItemInstance->SetDefinition(ItemDefinition);

	TArray<FSigilMixin> ConvertedMixins = FSigilMixinContainer::ConvertRecordsToMixins(Record.FragmentStateRecords);
	ConvertedMixins = ConvertedMixins.FilterByPredicate([ItemDefinition](const FSigilMixin& Mixin)
	{
		return ItemDefinition->Fragments.Contains(Mixin.Target);
	});

	for (const FSigilMixin& ConvertedMixin : ConvertedMixins)
	{
		ItemInstance->SetFragmentStateByClass(ConvertedMixin.Target->GetClass(), ConvertedMixin.Data);
	}

	FMemoryReader Reader(Record.ByteData);
	FObjectAndNameAsStringProxyArchive Ar2(Reader, true);
	Ar2.ArIsSaveGame = true;
	ItemInstance->Serialize(Ar2);
	return ItemInstance;
}

bool USigilInventoryFactory::SerializeItem_Implementation(USigilItemInstance* Item, FSigilItemRecord& Record)
{
	if (!IsValid(Item))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing item.");
		return false;
	}

	Record.ItemId = Item->GetItemId();
	const FSoftObjectPath AssetPath = FSoftObjectPath(Item->GetDefinition());
	Record.DefinitionAssetPath = AssetPath.ToString();
	Record.FragmentStateRecords = Item->GetFragmentStates().GetSerializableMixinRecords();


	FMemoryWriter Writer(Record.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(Writer, true);
	Ar.ArIsSaveGame = true;
	Item->Serialize(Ar);
	return true;
}

bool USigilInventoryFactory::SerializeCollection_Implementation(USigilItemCollection* Collection, FSigilCollectionRecord& Record)
{
	if (!IsValid(Collection))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing collection.");
		return false;
	}

	Record.Tag = Collection->GetCollectionTag();
	Record.Id = Collection->GetCollectionId();
	const FSoftObjectPath AssetPath = FSoftObjectPath(Collection->GetDefinition());
	Record.DefinitionAssetPath = AssetPath.ToString();
	const TArray<FSigilItemStack>& ValidStacks = Collection->GetAllItemStacks().FilterByPredicate([](const FSigilItemStack& ItemStack)
	{
		return ItemStack.IsValidStack();
	});

	for (const FSigilItemStack& Stack : ValidStacks)
	{
		FSigilStackRecord StackRecord;
		StackRecord.Id = Stack.Id;
		StackRecord.CollectionId = Stack.Collection->GetCollectionId();
		StackRecord.ItemId = Stack.Item->GetItemId();
		StackRecord.Amount = Stack.Amount;
		Record.StackRecords.Add(StackRecord);
	}

	return Record.IsValid();
}

void USigilInventoryFactory::DeserializeCollection_Implementation(USigilInventorySystemComponent* InventorySystem, const FSigilCollectionRecord& Record, TMap<FGuid, USigilItemInstance*>& ItemsMap)
{
	if (!IsValid(InventorySystem))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing inventory system.");
		return;
	}
	const FSoftObjectPath DefinitionAssetPath = FSoftObjectPath(Record.DefinitionAssetPath);
	const TSoftObjectPtr<USigilItemCollectionDefinition> DefinitionReference = TSoftObjectPtr<USigilItemCollectionDefinition>(DefinitionAssetPath);
	USigilItemCollectionDefinition* Definition = DefinitionReference.LoadSynchronous();

	if (Definition == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "failed to load definition from path:%s", *DefinitionAssetPath.ToString());
		return;
	}

	USigilItemCollection* NewCollection = CreateCollection(InventorySystem->GetOwner(), Definition);
	if (NewCollection == nullptr)
	{
		SIGIL_INVENTORY_LOG(Error, "failed to create collection from definition:%s", *GetNameSafe(Definition));
		return;
	}

	FSigilCollectionEntry NewEntry;
	NewEntry.Id = Record.Id;
	NewEntry.Instance = NewCollection;
	NewEntry.Definition = Definition;
	InventorySystem->AddCollectionEntry(NewEntry);

	for (const FSigilStackRecord& StackRecord : Record.StackRecords)
	{
		FSigilItemInfo Info;
		Info.Item = ItemsMap[StackRecord.ItemId];
		Info.Amount = StackRecord.Amount;
		Info.ItemCollection = NewCollection;
		InventorySystem->AddItem(Info);
	}
}

bool USigilInventoryFactory::SerializeInventory_Implementation(USigilInventorySystemComponent* InventorySystem, FSigilInventoryRecord& Record)
{
	if (!IsValid(InventorySystem))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing inventory system.");
		return false;
	}

	TArray<FSigilCollectionRecord> CollectionRecords;
	TArray<FSigilItemRecord> ItemRecords;

	TArray<USigilItemCollection*> Collections = InventorySystem->GetItemCollections();
	TArray<USigilItemInstance*> Items;

	// build collection records.
	for (USigilItemCollection* Collection : Collections)
	{
		if (Collection->IsInitialized())
		{
			FSigilCollectionRecord CollectionRecord;
			if (SerializeCollection(Collection, CollectionRecord))
			{
				CollectionRecords.Add(CollectionRecord);
			}
			Items.Append(Collection->GetAllItems());
		}
	}

	ItemRecords.Reserve(Items.Num());
	for (USigilItemInstance* Item : Items)
	{
		FSigilItemRecord ItemRecord;
		if (SerializeItem(Item, ItemRecord))
		{
			ItemRecords.Add(ItemRecord);
		}
	}

	Record.ItemRecords = ItemRecords;
	Record.CollectionRecords = CollectionRecords;

	return true;
}

void USigilInventoryFactory::DeserializeInventory_Implementation(USigilInventorySystemComponent* InventorySystem, const FSigilInventoryRecord& InRecord)
{
	if (!IsValid(InventorySystem))
	{
		SIGIL_INVENTORY_LOG(Error, "Missing inventory system.");
		return;
	}

	TMap<FGuid, USigilItemInstance*> ItemsMap;
	for (const FSigilItemRecord& ItemRecord : InRecord.ItemRecords)
	{
		if (USigilItemInstance* Instance = DeserializeItem(InventorySystem->GetOwner(), ItemRecord))
		{
			ItemsMap.Emplace(Instance->GetItemId(), Instance);
		}
	}

	for (const FSigilCollectionRecord& CollectionRecord : InRecord.CollectionRecords)
	{
		DeserializeCollection(InventorySystem, CollectionRecord, ItemsMap);
	}

	if (!InventorySystem->IsDefaultCollectionCreated())
	{
		SIGIL_INVENTORY_OWNED_CLOG(InventorySystem, Warning,
		               "The default collection definitions is not match with collections restored from inventory record. That may be a problem as you changed the layout of inventory.")
	}
}

#if WITH_EDITOR
EDataValidationResult USigilInventoryFactory::IsDataValid(class FDataValidationContext& Context) const
{
	if (DefaultItemInstanceClass.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("Missing Default Item Instance Class")));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}
#endif
