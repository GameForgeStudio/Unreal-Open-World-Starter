// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "OmniWalkDemoGameMode.h"

#include "OmniWalkPlayerController.h"
#include "OmniWalkPro.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AOmniWalkDemoGameMode::AOmniWalkDemoGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> GaspManny(
		TEXT("/GASPALS/Blueprints/RetargetedCharacters/CBP_SandboxCharacter_Manny"));
	if (GaspManny.Succeeded())
	{
		DefaultPawnClass = GaspManny.Class;
	}
	PlayerControllerClass = AOmniWalkPlayerController::StaticClass();
}

void AOmniWalkDemoGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr;
	if (Pawn && !Pawn->FindComponentByClass<UOmniWalkPro>())
	{
		UOmniWalkPro* OmniWalk = NewObject<UOmniWalkPro>(Pawn, TEXT("OmniWalkSurfaceMobility"));
		OmniWalk->RegisterComponent();
	}
}
