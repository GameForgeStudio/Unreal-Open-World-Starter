#pragma once

#include "GameFramework/GameModeBase.h"

#include "OWSTestLabGameMode.generated.h"

/** Full-stack lab mode: standard OWS character, vehicles, systems HUD, and lab controller. */
UCLASS()
class OWS_API AOWSTestLabGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOWSTestLabGameMode();

	virtual void StartPlay() override;
};

/** Isolated Epic Mover comparison using the same lab controller and persistent HUD. */
UCLASS()
class OWS_API AOWSTestLabMoverGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOWSTestLabMoverGameMode();
};
