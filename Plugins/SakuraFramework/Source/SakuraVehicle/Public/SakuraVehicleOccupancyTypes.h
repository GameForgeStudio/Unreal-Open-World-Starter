// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SakuraVehicleOccupancyTypes.generated.h"

class AActor;

/** One data-driven occupant position. SeatType is intentionally open-ended. */
USTRUCT(BlueprintType)
struct SAKURAVEHICLE_API FSakuraVehicleSeatDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	/** Examples: Driver, Passenger, Gunner, Operator, Pilot, or a project-defined type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName SeatType = TEXT("Passenger");

	/** The occupant of this seat is allowed to possess/control the vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	bool bControlsVehicle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	/** Half extents used to reject seats physically blocked by unregistered actors/cargo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FVector ClearanceExtent = FVector(35.0f, 35.0f, 50.0f);
};

/** A physical entry point and its ordered nearest-seat choices. */
USTRUCT(BlueprintType)
struct SAKURAVEHICLE_API FSakuraVehicleDoorDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FName DoorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	FTransform RelativeTransform = FTransform::Identity;

	/** Closest/reachable seats from this door, in priority order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sakura|Vehicle|Occupancy")
	TArray<FName> OrderedSeatIds;
};

/** Small replicated runtime state; definitions remain class/asset data. */
USTRUCT(BlueprintType)
struct SAKURAVEHICLE_API FSakuraVehicleSeatOccupancy
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	FName SeatId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Vehicle|Occupancy")
	TObjectPtr<AActor> Occupant = nullptr;
};
