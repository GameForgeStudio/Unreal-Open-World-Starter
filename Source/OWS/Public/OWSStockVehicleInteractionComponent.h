#pragma once

#include "Components/ActorComponent.h"
#include "OWSInteractionTargetComponent.h"

#include "OWSStockVehicleInteractionComponent.generated.h"

class AActor;
class APawn;
class UModularVehicleBaseComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class UVehicleDriveAssemblyComponent;
class UOWSInteractionTargetComponent;

USTRUCT(BlueprintType)
struct FOWSStockVehicleSeatDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FName SeatType = TEXT("Passenger");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	bool bControlsVehicle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FVector ClearanceExtent = FVector(35.0f, 35.0f, 50.0f);
};

USTRUCT(BlueprintType)
struct FOWSStockVehicleDoorDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FName DoorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	/** Character placement transform after a stopped exit through this door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	FTransform ExitRelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWS|Vehicle|Occupancy")
	TArray<FName> OrderedSeatIds;
};

USTRUCT(BlueprintType)
struct FOWSStockVehicleSeatOccupancy
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Vehicle|Occupancy")
	TObjectPtr<AActor> Occupant = nullptr;
};

/**
 * Project-side interaction and parked/off lifecycle for an otherwise
 * unmodified Epic stock vehicle. It does not replace or retune the stock
 * drivetrain, wheels, suspension, animation, or network-physics solver.
 */
UCLASS(ClassGroup = (OWS), meta = (BlueprintSpawnableComponent))
class OWS_API UOWSStockVehicleInteractionComponent final
	: public UActorComponent,
	  public IOWSInteractionTargetHandler
{
	GENERATED_BODY()

public:
	UOWSStockVehicleInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
	bool GetDoorExitWorldTransform(FName DoorId, FTransform& OutTransform) const;
	void GetDoorIds(TArray<FName>& OutDoorIds) const;
	bool IsControlSeat(FName SeatId) const;
	FName GetSeatForOccupant(const AActor* OccupantActor) const;
	bool HasControlSeatOccupant() const;
	bool IsRecoveryOwner(const AActor* OccupantActor) const;
	UOWSInteractionTargetComponent* GetDoorInteractionTarget(FName DoorId) const;

	virtual bool CanActivateOWSTarget_Implementation(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason) override;
	virtual bool ActivateOWSTarget_Implementation(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason) override;

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
	void CreateDoorInteractionTargets();
	void InitializeSeatOccupancy();
	const FOWSStockVehicleSeatDefinition* FindSeatDefinition(FName SeatId) const;
	const FOWSStockVehicleDoorDefinition* FindDoorDefinition(FName DoorId) const;
	bool IsSeatAvailable(FName SeatId, const AActor* EnteringActor) const;

	UPROPERTY(EditAnywhere, Category = "OWS|Vehicle|Occupancy")
	TArray<FOWSStockVehicleSeatDefinition> SeatDefinitions;

	UPROPERTY(EditAnywhere, Category = "OWS|Vehicle|Occupancy")
	TArray<FOWSStockVehicleDoorDefinition> DoorDefinitions;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "OWS|Vehicle|Occupancy")
	TArray<FOWSStockVehicleSeatOccupancy> SeatOccupancy;

	UPROPERTY(EditAnywhere, Category = "OWS|Vehicle|Occupancy", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumDoorFacingDot = 0.35f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOWSInteractionTargetComponent>> DoorInteractionTargets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOWSInteractionTargetComponent>> RuntimeCreatedDoorInteractionTargets;

	/** Bone physically locked to the world while the unoccupied car is parked. */
	UPROPERTY(Transient)
	bool bDriverPresent = false;
};
