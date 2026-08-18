#pragma once

#include "GameFramework/GameModeBase.h"

#include "SakuraTestLabGameMode.generated.h"

/** Full-stack lab mode: standard Sakura character, vehicles, systems HUD, and lab controller. */
UCLASS()
class SAKURAUNREALPROTOTYPE_API ASakuraTestLabGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASakuraTestLabGameMode();

	virtual void StartPlay() override;
};

/** Isolated Epic Mover comparison using the same lab controller and persistent HUD. */
UCLASS()
class SAKURAUNREALPROTOTYPE_API ASakuraTestLabMoverGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASakuraTestLabMoverGameMode();
};
