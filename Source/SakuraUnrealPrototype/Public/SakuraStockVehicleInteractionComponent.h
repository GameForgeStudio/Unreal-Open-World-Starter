#pragma once

#include "Components/ActorComponent.h"

#include "SakuraStockVehicleInteractionComponent.generated.h"

class AActor;
class APawn;
class UModularVehicleBaseComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class UVehicleDriveAssemblyComponent;

USTRUCT(BlueprintType)
struct FSakuraStockVehicleSeatDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName SeatType = TEXT("Passenger");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	bool bControlsVehicle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FVector ClearanceExtent = FVector(35.0f, 35.0f, 50.0f);
};

USTRUCT(BlueprintType)
struct FSakuraStockVehicleDoorDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName DoorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	TArray<FName> OrderedSeatIds;
};

USTRUCT(BlueprintType)
struct FSakuraStockVehicleSeatOccupancy
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	TObjectPtr<AActor> Occupant = nullptr;
};

/**
 * Project-side interaction and parked/off lifecycle for an otherwise
 * unmodified Epic stock vehicle. It does not replace or retune the stock
 * drivetrain, wheels, suspension, animation, or network-physics solver.
 */
UCLASS(ClassGroup = (Sakura), meta = (BlueprintSpawnableComponent))
class SAKURAUNREALPROTOTYPE_API USakuraStockVehicleInteractionComponent final
	: public UActorComponent
{
	GENERATED_BODY()

public:
	USakuraStockVehicleInteractionComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	APawn* GetVehiclePawn() const;
	bool FindFacedDoor(
		const FVector& ActivatorLocation,
		const FVector& ActivatorForward,
		float MaximumDistance,
		FName& OutDoorId,
		float& OutDistance) const;
	bool SelectAvailableSeat(
		FName DoorId,
		bool bAutoPlacementEnabled,
		const AActor* EnteringActor,
		FName& OutSeatId) const;
	bool OccupySeat(FName SeatId, AActor* OccupantActor);
	bool ReleaseSeat(AActor* OccupantActor);
	bool GetSeatWorldTransform(FName SeatId, FTransform& OutTransform) const;
	bool GetDoorWorldTransform(FName DoorId, FTransform& OutTransform) const;
	bool IsControlSeat(FName SeatId) const;
	FName GetSeatForOccupant(const AActor* OccupantActor) const;
	bool HasControlSeatOccupant() const;
	bool IsRecoveryOwner(const AActor* OccupantActor) const;

	/**
	 * Applies the project lifecycle around Epic's otherwise untouched stock
	 * Chaos Modular Vehicle. Entering the driver seat does not start or release
	 * the parked vehicle; X or the first accelerator request does that.
	 */
	void SetDriverPresent(bool bPresent);
	UModularVehicleBaseComponent* GetVehicleSimulation() const;
	UVehicleDriveAssemblyComponent* GetKinetiForgeDriveAssembly() const;
	UPrimitiveComponent* GetVehiclePhysicsBody() const;
	USkeletalMeshComponent* GetVehicleMesh() const;

private:
	void InitializeSeatOccupancy();
	const FSakuraStockVehicleSeatDefinition* FindSeatDefinition(FName SeatId) const;
	const FSakuraStockVehicleDoorDefinition* FindDoorDefinition(FName DoorId) const;
	bool IsSeatAvailable(FName SeatId, const AActor* EnteringActor) const;

	UPROPERTY(EditAnywhere, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraStockVehicleSeatDefinition> SeatDefinitions;

	UPROPERTY(EditAnywhere, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraStockVehicleDoorDefinition> DoorDefinitions;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Sakura|Vehicle|Occupancy")
	TArray<FSakuraStockVehicleSeatOccupancy> SeatOccupancy;

	UPROPERTY(EditAnywhere, Category = "Sakura|Vehicle|Occupancy", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumDoorFacingDot = 0.35f;

	/** Bone physically locked to the world while the unoccupied car is parked. */
	UPROPERTY(Transient)
	bool bDriverPresent = false;
};
