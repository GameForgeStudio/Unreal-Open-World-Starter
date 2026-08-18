// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "ChaosModularVehicle/ModularVehicleSkeletalPawn.h"
#include "SakuraVehicleOccupancyTypes.h"
#include "SakuraChaosVehicleDemoPawn.generated.h"

class UAnimInstance;
class UCameraComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
class UInputModifier_SakuraKinetiThrottle;
class USakuraChaosVehicleInputRouterComponent;
class USakuraVehicleSimCurveWheelComponent;
class USakuraVehicleSimIgnitionEngineComponent;
class USakuraVehicleSimSuspensionComponent;
class USkeletalMesh;
class USpringArmComponent;
class UVehicleSimChassisComponent;
class UVehicleSimClutchComponent;
class UVehicleSimTransmissionComponent;
struct FInputActionValue;

/** Construction-time A/B mode for the otherwise identical Chaos vehicle. */
UENUM(BlueprintType)
enum class ESakuraChaosVehicleDemoMode : uint8
{
	StockBaseline UMETA(DisplayName = "Stock Chaos Baseline"),
	SakuraHybrid UMETA(DisplayName = "Sakura Kineti + Curve Wheels")
};

/**
 * Asset-light runnable comparison pawn based on Epic's UE 5.8 modular sports car.
 * Both modes use one UModularVehicleBaseComponent and stock Chaos simulation
 * modules. SakuraHybrid only shapes throttle before Chaos input capture and adds
 * a baked lateral-slip curve to each stock FWheelSimModule's settings.
 */
UCLASS(Blueprintable)
class SAKURAVEHICLE_API ASakuraChaosVehicleDemoPawn
	: public AModularVehicleSkeletalPawn
{
	GENERATED_BODY()

public:
	ASakuraChaosVehicleDemoPawn(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PawnClientRestart() override;
	virtual void NotifyControllerChanged() override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Starts the parked engine without applying throttle; exiting still shuts it down. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Vehicle|Ignition")
	void RequestIgnitionStart();

	UFUNCTION(BlueprintPure, Category = "Sakura|Vehicle|Ignition")
	bool IsIgnitionRequested() const { return bIgnitionRequested; }

	/** Finds the in-range door most directly in front of the activating actor. */
	bool FindFacedDoor(
		const FVector& ActivatorLocation,
		const FVector& ActivatorForward,
		float MaximumDistance,
		FName& OutDoorId,
		float& OutDistance) const;

	/**
	 * Auto placement on prefers an available control seat only while no driver
	 * exists, then uses door-local fallbacks.
	 * Auto placement off considers only the door's first configured seat.
	 */
	bool SelectAvailableSeat(
		FName DoorId,
		bool bAutoPlacementEnabled,
		const AActor* EnteringActor,
		FName& OutSeatId) const;

	/** Generic registration seam for players, NPCs, animals, or cargo. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Vehicle|Occupancy")
	bool OccupySeat(FName SeatId, AActor* OccupantActor);

	UFUNCTION(BlueprintCallable, Category = "Sakura|Vehicle|Occupancy")
	bool ReleaseSeat(AActor* OccupantActor);
	bool GetSeatWorldTransform(FName SeatId, FTransform& OutTransform) const;
	bool GetDoorWorldTransform(FName DoorId, FTransform& OutTransform) const;
	bool IsControlSeat(FName SeatId) const;
	FName GetSeatForOccupant(const AActor* OccupantActor) const;
	bool HasControlSeatOccupant() const;
	/** Control-seat occupant owns recovery; otherwise the first occupied seat does. */
	bool IsRecoveryOwner(const AActor* OccupantActor) const;

	/** Construction default; the opt-in game mode selects the native stock subclass by URL. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sakura|Vehicle|Comparison")
	ESakuraChaosVehicleDemoMode ComparisonMode = ESakuraChaosVehicleDemoMode::SakuraHybrid;

	/** Priority used when installing Epic's example vehicle mapping context. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Input")
	int32 InputMappingPriority = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Input")
	TObjectPtr<USakuraChaosVehicleInputRouterComponent> InputRouter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category = "Sakura|Vehicle|Input")
	TObjectPtr<UInputModifier_SakuraKinetiThrottle> ThrottleShaper;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<UVehicleSimChassisComponent> Chassis;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimIgnitionEngineComponent> Engine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<UVehicleSimClutchComponent> Clutch;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<UVehicleSimTransmissionComponent> Transmission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimSuspensionComponent> FrontLeftSuspension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimSuspensionComponent> FrontRightSuspension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimSuspensionComponent> BackLeftSuspension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimSuspensionComponent> BackRightSuspension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimCurveWheelComponent> FrontLeftWheel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimCurveWheelComponent> FrontRightWheel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimCurveWheelComponent> BackLeftWheel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Simulation")
	TObjectPtr<USakuraVehicleSimCurveWheelComponent> BackRightWheel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Arbitrary per-vehicle seat topology; subclasses/assets can replace it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraVehicleSeatDefinition> SeatDefinitions;

	/** Arbitrary entry points and the seat order reachable from each one. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraVehicleDoorDefinition> DoorDefinitions;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_SeatOccupancy, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraVehicleSeatOccupancy> SeatOccupancy;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ApplyComparisonMode();

private:
	void ApplyExampleAssets();
	void ConfigureSuspension(
		USakuraVehicleSimSuspensionComponent* Suspension,
		FName BoneName,
		const FVector& RelativeLocation);
	void ConfigureWheel(
		USakuraVehicleSimCurveWheelComponent* Wheel,
		FName BoneName,
		bool bFrontWheel);
	void LoadInputAssets();
	void InstallInputMapping();
	void RemoveInputMapping();
	void ClearHeldInputs();
	void SetParkedState(bool bNewParked);
	void UpdateParkedState(float DeltaSeconds);
	void PublishVehicleControls(
		float Throttle,
		float Brake,
		float Steering,
		bool bHandbrake,
		bool bReverse,
		bool bIgnition);
	void AlignParkedVehicleToGround();
	void InitializeSeatOccupancy();
	const FSakuraVehicleSeatDefinition* FindSeatDefinition(FName SeatId) const;
	const FSakuraVehicleDoorDefinition* FindDoorDefinition(FName DoorId) const;
	bool IsSeatAvailable(FName SeatId, const AActor* EnteringActor) const;

	UFUNCTION()
	void OnRep_SeatOccupancy();

	void OnThrottleInput(const FInputActionValue& Value);
	void OnThrottleReleased(const FInputActionValue& Value);
	void OnBrakeInput(const FInputActionValue& Value);
	void OnBrakeReleased(const FInputActionValue& Value);
	void OnSteeringInput(const FInputActionValue& Value);
	void OnSteeringReleased(const FInputActionValue& Value);
	void OnHandbrakeInput(const FInputActionValue& Value);
	void OnHandbrakeReleased(const FInputActionValue& Value);
	void OnReverseInput(const FInputActionValue& Value);
	void OnReverseReleased(const FInputActionValue& Value);
	void OnLookInput(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<USkeletalMesh> ExampleSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftClassPtr<UAnimInstance> ExampleAnimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputMappingContext> ExampleInputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> ThrottleActionAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> BrakeActionAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> SteeringActionAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> HandbrakeActionAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> ReverseActionAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Example Assets")
	TSoftObjectPtr<UInputAction> LookActionAsset;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> LoadedInputMapping;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedThrottleAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedBrakeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedSteeringAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedHandbrakeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedReverseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedLookAction;

	/** Door must lie within this forward-facing cone to qualify for Activate. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Occupancy", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumDoorFacingDot = 0.45f;

	/** Maximum linear speed in cm/s at which a parked car may sleep. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Parking", meta = (ClampMin = "0.0"))
	float ParkingSleepLinearSpeed = 8.0f;

	/** Maximum angular speed in degrees/s at which a parked car may sleep. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Parking", meta = (ClampMin = "0.0"))
	float ParkingSleepAngularSpeed = 4.0f;

	/** Time under both speed limits before parking inputs are zeroed and the body sleeps. */
	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Vehicle|Parking", meta = (ClampMin = "0.0"))
	float ParkingSettleSeconds = 0.35f;

	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InstalledInputSubsystem;
	bool bInputMappingInstalled = false;
	float RawThrottleInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteeringInput = 0.0f;
	bool bHandbrakeInput = false;
	bool bReverseInput = false;
	bool bIgnitionRequested = false;
	/** Entry keeps a real parking hold until the first forward/reverse accelerator. */
	bool bDriveReleased = false;
	bool bParked = true;
	bool bParkingBrakeActive = true;
	float ParkingStableTime = 0.0f;
};

/** Native baseline pawn selected by the comparison game mode's URL option. */
UCLASS(NotBlueprintable)
class SAKURAVEHICLE_API ASakuraChaosVehicleStockDemoPawn
	: public ASakuraChaosVehicleDemoPawn
{
	GENERATED_BODY()

public:
	ASakuraChaosVehicleStockDemoPawn(const FObjectInitializer& ObjectInitializer);
};
