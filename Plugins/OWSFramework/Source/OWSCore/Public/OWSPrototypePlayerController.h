#pragma once

#include "GameFramework/PlayerController.h"
#include "OWSPrototypePlayerController.generated.h"

UCLASS()
class OWSCORE_API AOWSPrototypePlayerController final : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
