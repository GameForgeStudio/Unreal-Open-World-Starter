#pragma once

#include "GameFramework/GameModeBase.h"

#include "OWSMoverComparisonGameMode.generated.h"

/** Opt-in GameMode for the Mover comparison; never installed as the global default. */
UCLASS()
class OWSMOVEMENT_API AOWSMoverComparisonGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOWSMoverComparisonGameMode();
};
