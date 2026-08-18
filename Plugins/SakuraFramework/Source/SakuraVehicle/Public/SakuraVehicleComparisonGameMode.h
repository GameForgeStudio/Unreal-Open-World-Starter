// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "GameFramework/GameModeBase.h"
#include "SakuraVehicleComparisonGameMode.generated.h"

/**
 * Opt-in vehicle demo mode. Select it with Unreal's ?game= URL option.
 * It spawns the Sakura hybrid by default; append ?SakuraVehicleMode=Stock
 * to spawn the otherwise identical stock-settings baseline.
 */
UCLASS()
class SAKURAVEHICLE_API ASakuraVehicleComparisonGameMode final
	: public AGameModeBase
{
	GENERATED_BODY()

public:
	ASakuraVehicleComparisonGameMode();

	virtual void InitGame(
		const FString& MapName,
		const FString& Options,
		FString& ErrorMessage) override;
};
