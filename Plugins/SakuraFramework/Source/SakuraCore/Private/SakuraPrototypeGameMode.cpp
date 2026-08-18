#include "SakuraPrototypeGameMode.h"

#include "SakuraPrototypeCharacter.h"
#include "SakuraPrototypeHUD.h"
#include "SakuraPrototypePlayerController.h"

ASakuraPrototypeGameMode::ASakuraPrototypeGameMode()
{
	DefaultPawnClass = ASakuraPrototypeCharacter::StaticClass();
	PlayerControllerClass = ASakuraPrototypePlayerController::StaticClass();
	HUDClass = ASakuraPrototypeHUD::StaticClass();
}
