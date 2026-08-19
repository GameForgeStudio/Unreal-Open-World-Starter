// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Exchange/Shops/SigilShopSystemComponent.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "SigilCurrencySystemComponent.h"
#include "SigilInventoryFunctionLibrary.h"
#include "SigilInventorySubsystem.h"
#include "SigilInventorySystemComponent.h"
#include "SigilItemCollection.h"
#include "SigilItemDefinition.h"
#include "SigilItemFragment_Shoppable.h"
#include "Items/SigilItemInstance.h"
#include "SigilInventoryLogChannels.h"
#include "Exchange/Shops/SigilShopCondition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilShopSystemComponent)


USigilShopSystemComponent::USigilShopSystemComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

USigilShopSystemComponent* USigilShopSystemComponent::GetShopSystemComponent(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<USigilShopSystemComponent>() : nullptr;
}

USigilInventorySystemComponent* USigilShopSystemComponent::GetInventory() const
{
	return OwningInventory;
}

bool USigilShopSystemComponent::BuyItem(USigilInventorySystemComponent* BuyerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo)
{
	return BuyItemInternal(BuyerInventory, CurrencySystem, ItemInfo);
}

bool USigilShopSystemComponent::SellItem(USigilInventorySystemComponent* SellerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo)
{
	return SellItemInternal(SellerInventory, CurrencySystem, ItemInfo);
}

bool USigilShopSystemComponent::CanBuyerBuyItem(USigilInventorySystemComponent* BuyerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo) const
{
	USigilItemCollection* TargetCollection = BuyerInventory->GetCollectionByTag(TargetItemCollectionToAddOnBuy);
	if (TargetCollection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "buyer's inventory missing collection named:%s", *TargetItemCollectionToAddOnBuy.ToString());
		return false;
	}

	FSigilItemInfo CanAddItemInfo;
	if (!TargetCollection->CanAddItem(ItemInfo, CanAddItemInfo))
	{
		SIGIL_INVENTORY_CLOG(Warning, "buyer's collection can't add this item(%s)", *ItemInfo.GetDebugString());
		return false;
	}

	for (int i = 0; i < BuyConditions.Num(); i++)
	{
		if (BuyConditions[i]->CanBuy(this, BuyerInventory, CurrencySystem, ItemInfo)) { continue; }
		SIGIL_INVENTORY_CLOG(Warning, "buy collection(%s) reject buying item(%s)", *BuyConditions[i].GetObject()->GetClass()->GetName(), *ItemInfo.GetDebugString());
		return false;
	}

	return CanBuyerBuyItemInternal(BuyerInventory, CurrencySystem, ItemInfo);
}

bool USigilShopSystemComponent::CanSellerSellItem(USigilInventorySystemComponent* SellerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo) const
{
	for (int i = 0; i < SellConditions.Num(); i++)
	{
		if (SellConditions[i]->CanSell(this, SellerInventory, CurrencySystem, ItemInfo)) { continue; }
		SIGIL_INVENTORY_CLOG(Warning, "sell collection(%s) reject selling item(%s)", *SellConditions[i].GetObject()->GetClass()->GetName(), *ItemInfo.GetDebugString());
		return false;
	}
	return CanSellerSellItemInternal(SellerInventory, CurrencySystem, ItemInfo);
}

bool USigilShopSystemComponent::IsItemBuyable(const FSigilItemInfo& ItemInfo) const
{
	if (OwningInventory == nullptr) { return false; }

	if (!ItemInfo.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid item to buy.");
		return false;
	}

	USigilItemCollection* ItemCollection = ItemInfo.Item->GetOwningCollection();
	if (ItemCollection == nullptr) { ItemCollection = OwningInventory->GetDefaultCollection(); }

	if (!ItemCollection->HasItem(ItemInfo.Item, 1))
	{
		SIGIL_INVENTORY_CLOG(Warning, "shop's inventory doesn't have item:%s", *ItemInfo.Item->GetDefinition()->GetName());
		return false;
	}
	const USigilItemFragment_Shoppable* Shoppable = ItemInfo.Item->FindFragmentByClass<USigilItemFragment_Shoppable>();
	if (Shoppable == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "item(%s) is not buyable, missing Shoppable fragment!", *ItemInfo.GetDebugString());
		return false;
	}
	if (Shoppable->BuyCurrencyAmounts.IsEmpty())
	{
		SIGIL_INVENTORY_CLOG(Warning, "item(%s) is not buyable, missing BuyCurrencyAmounts in shoppable fragment!", *ItemInfo.GetDebugString());
		return false;
	}
	return true;
}

bool USigilShopSystemComponent::IsItemSellable(const FSigilItemInfo& ItemInfo) const
{
	if (!ItemInfo.IsValid())
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid item to sell.");
		return false;
	}

	const USigilItemFragment_Shoppable* Shoppable = ItemInfo.Item->FindFragmentByClass<USigilItemFragment_Shoppable>();

	if (Shoppable == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "item(%s) is not sellable, missing Shoppable fragment!", *ItemInfo.GetDebugString());
		return false;
	}

	if (Shoppable->SellCurrencyAmounts.IsEmpty())
	{
		SIGIL_INVENTORY_CLOG(Warning, "item(%s) is not sellable, missing SellCurrencyAmounts in shoppable fragment!", *ItemInfo.GetDebugString());
		return false;
	}
	return true;
}

float USigilShopSystemComponent::GetBuyModifierForBuyer_Implementation(USigilInventorySystemComponent* BuyerInventory) const
{
	return 1 + BuyPriceModifier;
}

float USigilShopSystemComponent::GetSellModifierForSeller_Implementation(USigilInventorySystemComponent* SellerInventory) const
{
	return 1 + SellPriceModifier;
}

bool USigilShopSystemComponent::TryGetBuyValueForBuyer_Implementation(USigilInventorySystemComponent* Buyer, const FSigilItemInfo& ItemInfo, TArray<FSigilCurrencyEntry>& BuyValue) const
{
	if (!IsValid(ItemInfo.Item))
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid item to buy.");
		return false;
	}
	const USigilItemFragment_Shoppable* Shoppable = ItemInfo.Item->FindFragmentByClass<USigilItemFragment_Shoppable>();
	if (!IsValid(Shoppable))
	{
		SIGIL_INVENTORY_CLOG(Warning, "missing Shoppable fragment for item:%s!", *GetNameSafe(ItemInfo.Item->GetDefinition()));
		return false;
	}

	float Modifier = GetBuyModifierForBuyer(Buyer);
	BuyValue = USigilInventoryFunctionLibrary::MultiplyCurrencies(Shoppable->BuyCurrencyAmounts, Modifier * ItemInfo.Amount);

	return BuyValue.IsEmpty() == false;
}

bool USigilShopSystemComponent::TryGetSellValueForSeller_Implementation(USigilInventorySystemComponent* Seller, const FSigilItemInfo& ItemInfo, TArray<FSigilCurrencyEntry>& SellValue) const
{
	if (!IsValid(ItemInfo.Item))
	{
		SIGIL_INVENTORY_CLOG(Warning, "invalid item to sell.");
		return false;
	}

	const USigilItemFragment_Shoppable* Shoppable = ItemInfo.Item->FindFragmentByClass<USigilItemFragment_Shoppable>();
	if (!IsValid(Shoppable))
	{
		SIGIL_INVENTORY_CLOG(Warning, "missing Shoppable fragment for item:%s!", *GetNameSafe(ItemInfo.Item->GetDefinition()));
		return false;
	}

	float Modifier = GetSellModifierForSeller(Seller);

	SellValue = USigilInventoryFunctionLibrary::MultiplyCurrencies(Shoppable->SellCurrencyAmounts, Modifier * ItemInfo.Amount);

	return SellValue.IsEmpty() == false;
}

void USigilShopSystemComponent::BeginPlay()
{
	OwningInventory = USigilInventorySystemComponent::FindInventorySystemComponent(GetOwner());
	if (!OwningInventory)
	{
		SIGIL_INVENTORY_CLOG(Error, "Requires inventory system component!");
	}
	{
		TArray<UActorComponent*> Components = GetOwner()->GetComponentsByInterface(USigilShopBuyCondition::StaticClass());
		BuyConditions.Empty();
		for (const auto Component : Components)
		{
			BuyConditions.Add(Component);
		}
	}

	{
		TArray<UActorComponent*> Components = GetOwner()->GetComponentsByInterface(USigilShopSellCondition::StaticClass());
		SellConditions.Empty();
		for (const auto Component : Components)
		{
			SellConditions.Add(Component);
		}
	}

	Super::BeginPlay();
}

void USigilShopSystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool USigilShopSystemComponent::CanSellerSellItemInternal_Implementation(USigilInventorySystemComponent* SellerInventory, USigilCurrencySystemComponent* CurrencySystem,
                                                                        const FSigilItemInfo& ItemInfo) const
{
	USigilItemCollection* ItemCollection = ItemInfo.ItemCollection;
	if (ItemCollection == nullptr || ItemCollection->GetOwningInventory() == SellerInventory)
	{
		ItemCollection = SellerInventory->GetDefaultCollection();
	}

	if (ItemCollection == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Warning, "seller:%s doesn't have valid default collection.", *GetNameSafe(SellerInventory));
		return false;
	}

	//atleast has one.
	if (!ItemCollection->HasItem(ItemInfo.Item, 1))
	{
		return false;
	}

	return true;
}

bool USigilShopSystemComponent::CanBuyerBuyItemInternal_Implementation(USigilInventorySystemComponent* BuyerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo) const
{
	TArray<FSigilCurrencyEntry> BuyPrice;
	if (TryGetBuyValueForBuyer(BuyerInventory, ItemInfo, BuyPrice))
	{
		return CurrencySystem->HasCurrencies(BuyPrice);
	}
	return false;
}

bool USigilShopSystemComponent::SellItemInternal_Implementation(USigilInventorySystemComponent* Seller, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo)
{
	if (!IsValid(Seller) || !ItemInfo.IsValid() || !IsValid(CurrencySystem))
	{
		SIGIL_INVENTORY_CLOG(Warning, "passed invalid parameters!");
		return false;
	}
	if (!IsItemSellable(ItemInfo))
	{
		SIGIL_INVENTORY_CLOG(Warning, "item:%s is not sellable", *ItemInfo.GetDebugString());
		return false;
	}
	if (!CanSellerSellItem(Seller, CurrencySystem, ItemInfo))
	{
		SIGIL_INVENTORY_CLOG(Warning, "seller can sell this item:%s", *ItemInfo.GetDebugString());
		return false;
	}
	USigilItemCollection* ItemCollection = ItemInfo.ItemCollection;
	if (ItemCollection == nullptr || ItemCollection->GetOwningInventory() == Seller)
	{
		ItemCollection = Seller->GetDefaultCollection();
	}
	if (ItemCollection->RemoveItem(ItemInfo).Amount != ItemInfo.Amount)
	{
		SIGIL_INVENTORY_CLOG(Error, "Failed to remove item(%s) from inventory!", *ItemInfo.GetDebugString());
		return false;
	}

	TArray<FSigilCurrencyEntry> SellCurrencyAmount;
	if (!TryGetSellValueForSeller(Seller, ItemInfo, SellCurrencyAmount))
	{
		SIGIL_INVENTORY_CLOG(Error, "can't get sell value for item:%s", *ItemInfo.GetDebugString());
		return false;
	}

	CurrencySystem->AddCurrencies(SellCurrencyAmount);

	OwningInventory->AddItem(ItemInfo);
	return true;
}

bool USigilShopSystemComponent::BuyItemInternal_Implementation(USigilInventorySystemComponent* BuyerInventory, USigilCurrencySystemComponent* CurrencySystem, const FSigilItemInfo& ItemInfo)
{
	if (!IsValid(BuyerInventory) || !ItemInfo.IsValid() || !IsValid(CurrencySystem))
	{
		return false;
	}
	if (!IsItemBuyable(ItemInfo))
	{
		return false;
	}
	if (!CanBuyerBuyItem(BuyerInventory, CurrencySystem, ItemInfo))
	{
		return false;
	}

	USigilItemCollection* TargetCollection = BuyerInventory->GetCollectionByTag(TargetItemCollectionToAddOnBuy);
	if (TargetCollection == nullptr)
	{
		return false;
	}

	//remove currency from buyer's currency system.
	TArray<FSigilCurrencyEntry> BuyPrice;
	if (TryGetBuyValueForBuyer(BuyerInventory, ItemInfo, BuyPrice))
	{
		CurrencySystem->RemoveCurrencies(BuyPrice);
	}
	else
	{
		return false;
	}

	if (ItemInfo.Item->IsUnique())
	{
		for (int32 i = 0; i < ItemInfo.Amount; ++i)
		{
			USigilItemInstance* NewItem = USigilInventorySubsystem::Get(this)->CreateItem(BuyerInventory->GetOwner(), ItemInfo.Item->GetDefinition());
			TargetCollection->AddItem(NewItem, 1);
		}
	}
	else
	{
		TargetCollection->AddItem(ItemInfo);
	}

	// Remove item from shop's inventory.
	OwningInventory->RemoveItem(ItemInfo);

	// Add currency to shop's currency system.
	if (USigilCurrencySystemComponent* OwningCurrencySystem = USigilCurrencySystemComponent::GetCurrencySystemComponent(GetOwner()))
	{
		OwningCurrencySystem->AddCurrencies(BuyPrice);
	}

	return true;
}
