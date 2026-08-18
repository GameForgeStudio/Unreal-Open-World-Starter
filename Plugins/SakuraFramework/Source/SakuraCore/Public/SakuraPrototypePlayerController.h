#pragma once

#include "GameFramework/PlayerController.h"
#include "SakuraPrototypePlayerController.generated.h"

UCLASS()
class SAKURACORE_API ASakuraPrototypePlayerController final : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
