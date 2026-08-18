#pragma once

#include "GameFramework/GameModeBase.h"

#include "SakuraMoverComparisonGameMode.generated.h"

/** Opt-in GameMode for the Mover comparison; never installed as the global default. */
UCLASS()
class SAKURAMOVEMENT_API ASakuraMoverComparisonGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASakuraMoverComparisonGameMode();
};
