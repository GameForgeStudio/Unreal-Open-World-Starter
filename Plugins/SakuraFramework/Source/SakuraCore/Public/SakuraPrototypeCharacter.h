#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "SaveExtensionInterface.h"
#include "SakuraPrototypeCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USakuraInventoryBridgeComponent;
class USakuraPrototypeAttributeSet;
class USakuraPrototypeEquipmentSystemComponent;
class USakuraPrototypeInventorySystemComponent;
class USigilItemInstance;
class USpringArmComponent;
struct FInputActionValue;

/** Asset-light network prototype host. The Pawn-owned ASC is intentional prototype scope. */
UCLASS(Blueprintable)
class SAKURACORE_API ASakuraPrototypeCharacter final : public ACharacter,
	public IAbilitySystemInterface,
	public ISaveExtensionInterface
{
	GENERATED_BODY()

public:
	ASakuraPrototypeCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnSaveFinished(const FSELevelFilter& Filter, bool bError) override;
	virtual void OnLoadFinished(const FSELevelFilter& Filter, bool bError) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype")
	void RequestEquipDemoItem();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype")
	void RequestUnequipDemoItem();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype")
	void RequestUseDemoItem();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype")
	void RequestActivateEquipmentAbility();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype|Save")
	void RequestSaveGroundedCheckpoint();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype|Save")
	void RequestLoadGroundedCheckpoint();

	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype|Save")
	void RequestAdjustPrototypeScalar(float Delta);

	UFUNCTION(BlueprintPure, Category="Sakura|Prototype|Save")
	float GetPrototypeScalar() const { return PrototypeScalar; }

	UFUNCTION(BlueprintPure, Category="Sakura|Prototype")
	FString GetPrototypeStatus() const { return PrototypeStatus; }

	UFUNCTION(BlueprintPure, Category="Sakura|Prototype")
	USakuraPrototypeAttributeSet* GetPrototypeAttributes() const { return AttributeSet; }

	/**
	 * Replicates the generic hidden-seated collision presentation used by any
	 * vehicle implementation without coupling SakuraCore to SakuraVehicle.
	 */
	UFUNCTION(BlueprintCallable, Category="Sakura|Prototype|Vehicle")
	void SetVehicleSeatedPresentation(bool bSeated);

	UFUNCTION(BlueprintPure, Category="Sakura|Prototype|Vehicle")
	bool IsVehicleSeatedPresentation() const
	{
		return bVehicleSeatedPresentation;
	}


protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void AddInputContexts();
	void InitializeAbilityActorInfo();
	void InitializePrototypeSystems();
	USigilItemInstance* FindFirstItem(bool bEquipped, bool bUseItem) const;
	void SaveGroundedCheckpointAuthority();
	void LoadGroundedCheckpointAuthority();
	void AdjustPrototypeScalarAuthority(float Delta);
	void IncreasePrototypeScalar();
	void DecreasePrototypeScalar();
	void SetPrototypeStatus(const FString& NewStatus);
	void ApplyVehicleSeatedPresentation();

	UFUNCTION()
	void OnRep_VehicleSeatedPresentation();


	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSaveGroundedCheckpoint();
	bool ServerSaveGroundedCheckpoint_Validate();
	void ServerSaveGroundedCheckpoint_Implementation();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLoadGroundedCheckpoint();
	bool ServerLoadGroundedCheckpoint_Validate();
	void ServerLoadGroundedCheckpoint_Implementation();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAdjustPrototypeScalar(float Delta);
	bool ServerAdjustPrototypeScalar_Validate(float Delta);
	void ServerAdjustPrototypeScalar_Implementation(float Delta);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|GAS", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|GAS", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USakuraPrototypeAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USakuraPrototypeInventorySystemComponent> InventorySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USakuraPrototypeEquipmentSystemComponent> EquipmentSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sakura|Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USakuraInventoryBridgeComponent> InventoryBridge;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category="Sakura|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(SaveGame, Replicated)
	FTransform GroundedCheckpoint;

	UPROPERTY(SaveGame, Replicated)
	bool bHasGroundedCheckpoint = false;

	UPROPERTY(SaveGame, Replicated)
	float PrototypeScalar = 1.0f;

	UPROPERTY(Replicated)
	FString PrototypeStatus = TEXT("Starting prototype...");

	/** Actor collision itself is not replicated by AActor, so seat state is. */
	UPROPERTY(ReplicatedUsing=OnRep_VehicleSeatedPresentation)
	bool bVehicleSeatedPresentation = false;

	static const FName PrototypeSaveSlot;
};
