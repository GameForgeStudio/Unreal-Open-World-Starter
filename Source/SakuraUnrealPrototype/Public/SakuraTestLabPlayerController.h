#pragma once

#include "GameFramework/PlayerController.h"

#include "SakuraTestLabPlayerController.generated.h"

class APawn;
class ASakuraPrototypeCharacter;
class USakuraStockVehicleInteractionComponent;

/**
 * Persistent input and possession owner for the test lab.
 *
 * Keeping the interaction keys on the controller makes them survive Pawn swaps.
 * Vehicle possession is performed only by authority after proximity/ownership
 * checks, while the same Character instance remains alive as the player's home
 * for inventory, GAS, and checkpoint state.
 */
UCLASS()
class SAKURAUNREALPROTOTYPE_API ASakuraTestLabPlayerController final
	: public APlayerController
{
	GENERATED_BODY()

public:
	ASakuraTestLabPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Character currently on foot, or the preserved Character while driving. */
	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	ASakuraPrototypeCharacter* GetHomeCharacterForHUD() const;

	/** The associated lab vehicle for a driver or passenger, otherwise null. */
	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	APawn* GetCurrentLabVehicle() const;

	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	FName GetCurrentVehicleSeatId() const { return OccupiedSeatId; }

	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	bool IsCurrentVehicleDriver() const;

	/** Persistent preference that the future settings-menu checkbox edits. */
	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	bool IsVehicleAutoPlacementEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	const FString& GetVehicleInteractionStatus() const
	{
		return VehicleInteractionStatus;
	}

	/** True when the replicated/auth game mode is the isolated Mover lab mode. */
	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	bool IsInMoverMode() const;

	/**
	 * Distance in centimeters to the nearest eligible lab vehicle.
	 * Returns -1 while already seated, in Mover mode, or when the faced door has
	 * no seat permitted by the current auto-placement preference.
	 */
	UFUNCTION(BlueprintPure, Category = "Sakura|Test Lab")
	float GetNearestEnterableVehicleDistance() const;

private:
	void RequestActivate();
	void RequestToggleVehicle();
	void RequestToggleMoverMode();
	void ToggleVehicleAuthority(bool bAutoPlacementEnabled);
	void ToggleMoverModeAuthority();

	UFUNCTION(Server, Reliable)
	void ServerToggleVehicle(bool bAutoPlacementEnabled);


	UFUNCTION(Server, Reliable)
	void ServerToggleMoverMode();

	APawn* FindNearestEnterableVehicle(
		bool bAutoPlacementEnabled,
		float* OutDistance = nullptr,
		FName* OutDoorId = nullptr,
		FName* OutSeatId = nullptr) const;
	bool EnterVehicleAuthority(
		ASakuraPrototypeCharacter& PrototypeCharacter,
		APawn& Vehicle,
		USakuraStockVehicleInteractionComponent& Interaction,
		FName DoorId,
		FName SeatId);
	bool EnterPassengerSeatAuthority(
		ASakuraPrototypeCharacter& PrototypeCharacter,
		APawn& Vehicle,
		USakuraStockVehicleInteractionComponent& Interaction,
		FName DoorId,
		FName SeatId);
	bool ExitVehicleAuthority(
		APawn& Vehicle,
		USakuraStockVehicleInteractionComponent& Interaction);
	bool ExitPassengerSeatAuthority(
		APawn& Vehicle,
		USakuraStockVehicleInteractionComponent& Interaction);
	bool PlaceHomeCharacterBesideVehicle(
		ASakuraPrototypeCharacter& PrototypeCharacter,
		const APawn& Vehicle,
		const USakuraStockVehicleInteractionComponent& Interaction,
		FName PreferredDoorId = NAME_None) const;
	void UpdateRecoveryCheckpoint(float DeltaTime);
	void RecoverFallenPawn();
	void RestoreAfterVehicleLossAuthority();

	/** Owner-visible reference needed by the HUD after the controller possesses a vehicle. */
	UPROPERTY(Transient, Replicated)
	TObjectPtr<ASakuraPrototypeCharacter> HomeCharacter;

	/** Owner-visible vehicle association for both drivers and passengers. */
	UPROPERTY(Transient, Replicated)
	TObjectPtr<APawn> OccupiedVehicle;

	UPROPERTY(Transient, Replicated)
	FName OccupiedSeatId = NAME_None;

	UPROPERTY(Transient, Replicated)
	FName EnteredDoorId = NAME_None;

	UPROPERTY(Transient, Replicated)
	FString VehicleInteractionStatus =
		TEXT("Face an in-range vehicle door and use Activate.");

	/** Server-authoritative maximum range for entering an unoccupied vehicle. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Vehicle", meta = (ClampMin = "0.0"))
	float MaximumVehicleEnterDistance = 500.0f;

	/** Clearance beyond the vehicle and Character bounds used for side exits. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Vehicle", meta = (ClampMin = "0.0"))
	float VehicleExitPadding = 75.0f;

	/** Absolute last-resort recovery height, deliberately below all proving-ground terrain. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Recovery")
	float HardRecoveryZ = -100000.0f;

	/** Recovery cannot begin above this height, so valleys and underpasses remain valid. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Recovery")
	float NoGroundRecoveryActivationZ = -30000.0f;

	/** Downward terrain probe used only after the pawn is below the playable-world envelope. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Recovery", meta = (ClampMin = "100.0"))
	float NoGroundTraceDistance = 10000.0f;

	/** Continuous falling/no-ground time required before an automatic recovery. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Recovery", meta = (ClampMin = "0.0"))
	float NoGroundRecoveryDelay = 1.5f;

	/** How often a grounded/upright pose is retained as a nearby recovery point. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Test Lab|Recovery", meta = (ClampMin = "0.1"))
	float RecoveryCheckpointInterval = 0.75f;

	FTransform LastSafeCharacterTransform = FTransform::Identity;
	bool bHasSafeCharacterTransform = false;
	float RecoveryCheckpointElapsed = 0.0f;
	float NoGroundRecoveryElapsed = 0.0f;
};
