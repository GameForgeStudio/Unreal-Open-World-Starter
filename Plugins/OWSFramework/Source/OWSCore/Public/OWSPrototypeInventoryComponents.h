#pragma once

#include "Equipping/SigilEquipmentSystemComponent.h"
#include "SigilInventorySystemComponent.h"
#include "OWSPrototypeInventoryComponents.generated.h"

class USigilItemCollectionDefinition;
class USigilItemDefinition;
class USigilItemSlotCollectionDefinition;

/**
 * Prototype-only Sigil configuration seam.
 *
 * Collection and item definitions must be saved assets so their references are
 * stable during replication. This component wires assigned assets into Sigil's
 * protected configuration immediately before initialization; it does not create
 * transient definitions or call Sigil's recursive loadout server RPC.
 */
UCLASS(ClassGroup=(OWS), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class OWSCORE_API UOWSPrototypeInventorySystemComponent final : public USigilInventorySystemComponent
{
	GENERATED_BODY()

public:
	UOWSPrototypeInventorySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Adds assigned assets to Sigil's configuration and validates the demo contract. */
	UFUNCTION(BlueprintCallable, Category="OWS|Prototype|Inventory")
	bool PrepareAndValidatePrototypeConfiguration(FString& OutError);

	/** Seeds missing demo items after Sigil initialization without using its broken loadout RPC. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="OWS|Prototype|Inventory")
	bool SeedMissingPrototypeItems(FString& OutError);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory")
	TObjectPtr<const USigilItemCollectionDefinition> PrototypeMainCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory")
	TObjectPtr<const USigilItemSlotCollectionDefinition> PrototypeEquippedCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory", meta=(AllowedClasses="/Script/SigilInventory.SigilItemDefinition"))
	TSoftObjectPtr<USigilItemDefinition> PrototypeSwordDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory", meta=(AllowedClasses="/Script/SigilInventory.SigilItemDefinition"))
	TSoftObjectPtr<USigilItemDefinition> PrototypePotionDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory")
	bool bSeedPrototypeLoadout = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OWS|Prototype|Inventory", meta=(ClampMin="1"))
	int32 PrototypePotionCount = 3;
};

/** Equipment monitoring is deliberately delayed until the inventory is valid. */
UCLASS(ClassGroup=(OWS), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class OWSCORE_API UOWSPrototypeEquipmentSystemComponent final : public USigilEquipmentSystemComponent
{
	GENERATED_BODY()

public:
	UOWSPrototypeEquipmentSystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
