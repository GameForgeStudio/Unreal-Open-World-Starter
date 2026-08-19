// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "OWSVehicleComparisonGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "OWSChaosVehicleDemoPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSVehicleComparisonGameMode)

AOWSVehicleComparisonGameMode::AOWSVehicleComparisonGameMode()
{
	DefaultPawnClass = AOWSChaosVehicleDemoPawn::StaticClass();
}

void AOWSVehicleComparisonGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString RequestedMode = UGameplayStatics::ParseOption(
		Options,
		TEXT("OWSVehicleMode"));
	DefaultPawnClass = RequestedMode.Equals(TEXT("Stock"), ESearchCase::IgnoreCase)
		? AOWSChaosVehicleStockDemoPawn::StaticClass()
		: AOWSChaosVehicleDemoPawn::StaticClass();
}
