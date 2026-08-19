#pragma once

#include "Components/ActorComponent.h"
#include "OWSInventoryBridgeComponent.generated.h"

class USigilInventorySystemComponent;
class USigilItemInstance;

/**
 * Small, server-authoritative bridge from local input/UI to Sigil and GAS.
 * Every RPC accepts only a replicated item already owned by this actor's
 * inventory, then repeats all semantic checks before mutating state.
 */
UCLASS(ClassGroup=(OWS), BlueprintType, meta=(BlueprintSpawnableComponent))
class OWSCORE_API UOWSInventoryBridgeComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWSInventoryBridgeComponent();

	UFUNCTION(BlueprintCallable, Category="OWS|Prototype|Inventory")
	void RequestEquipItem(USigilItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category="OWS|Prototype|Inventory")
	void RequestUnequipItem(USigilItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category="OWS|Prototype|Inventory")
	void RequestUseItem(USigilItemInstance* Item);

private:
	UFUNCTION(Server, Reliable)
	void ServerEquipItem(USigilItemInstance* Item);
	void ServerEquipItem_Implementation(USigilItemInstance* Item);

	UFUNCTION(Server, Reliable)
	void ServerUnequipItem(USigilItemInstance* Item);
	void ServerUnequipItem_Implementation(USigilItemInstance* Item);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(USigilItemInstance* Item);
	void ServerUseItem_Implementation(USigilItemInstance* Item);

	bool IsOwnedInventoryItem(const USigilItemInstance* Item) const;
	bool EquipItemAuthority(USigilItemInstance* Item) const;
	bool UnequipItemAuthority(USigilItemInstance* Item) const;
	bool UseItemAuthority(USigilItemInstance* Item) const;
	USigilInventorySystemComponent* GetInventory() const;
};
