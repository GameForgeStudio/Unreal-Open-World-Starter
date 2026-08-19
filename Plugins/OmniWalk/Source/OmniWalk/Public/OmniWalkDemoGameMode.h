// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OmniWalkDemoGameMode.generated.h"

UCLASS()
class OMNIWALK_API AOmniWalkDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOmniWalkDemoGameMode();
	virtual void RestartPlayer(AController* NewPlayer) override;
};
