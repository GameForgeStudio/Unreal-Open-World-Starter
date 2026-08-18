// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraVehicleComparisonGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "SakuraChaosVehicleDemoPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SakuraVehicleComparisonGameMode)

ASakuraVehicleComparisonGameMode::ASakuraVehicleComparisonGameMode()
{
	DefaultPawnClass = ASakuraChaosVehicleDemoPawn::StaticClass();
}

void ASakuraVehicleComparisonGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString RequestedMode = UGameplayStatics::ParseOption(
		Options,
		TEXT("SakuraVehicleMode"));
	DefaultPawnClass = RequestedMode.Equals(TEXT("Stock"), ESearchCase::IgnoreCase)
		? ASakuraChaosVehicleStockDemoPawn::StaticClass()
		: ASakuraChaosVehicleDemoPawn::StaticClass();
}
