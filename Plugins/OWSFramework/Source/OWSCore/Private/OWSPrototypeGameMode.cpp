#include "OWSPrototypeGameMode.h"

#include "OWSPrototypeCharacter.h"
#include "OWSPrototypeHUD.h"
#include "OWSPrototypePlayerController.h"

AOWSPrototypeGameMode::AOWSPrototypeGameMode()
{
	DefaultPawnClass = AOWSPrototypeCharacter::StaticClass();
	PlayerControllerClass = AOWSPrototypePlayerController::StaticClass();
	HUDClass = AOWSPrototypeHUD::StaticClass();
}
