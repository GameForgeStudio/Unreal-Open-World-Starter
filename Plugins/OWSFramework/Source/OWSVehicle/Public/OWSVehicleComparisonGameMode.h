// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "GameFramework/GameModeBase.h"
#include "OWSVehicleComparisonGameMode.generated.h"

/**
 * Opt-in vehicle demo mode. Select it with Unreal's ?game= URL option.
 * It spawns the OWS hybrid by default; append ?OWSVehicleMode=Stock
 * to spawn the otherwise identical stock-settings baseline.
 */
UCLASS()
class OWSVEHICLE_API AOWSVehicleComparisonGameMode final
	: public AGameModeBase
{
	GENERATED_BODY()

public:
	AOWSVehicleComparisonGameMode();

	virtual void InitGame(
		const FString& MapName,
		const FString& Options,
		FString& ErrorMessage) override;
};
