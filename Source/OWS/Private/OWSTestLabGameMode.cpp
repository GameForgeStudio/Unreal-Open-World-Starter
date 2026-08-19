#include "OWSTestLabGameMode.h"

#include "OWSMoverComparisonPawn.h"
#include "OWSPrototypeCharacter.h"
#include "OWSStockVehicleInteractionComponent.h"
#include "OWSTestLabHUD.h"
#include "OWSTestLabPlayerController.h"
#include "SaveManager.h"
#include "EngineUtils.h"
#include "VehicleDriveAssemblyComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSTestLabGameMode)

AOWSTestLabGameMode::AOWSTestLabGameMode()
{
	DefaultPawnClass = AOWSPrototypeCharacter::StaticClass();
	PlayerControllerClass = AOWSTestLabPlayerController::StaticClass();
	HUDClass = AOWSTestLabHUD::StaticClass();
}

void AOWSTestLabGameMode::StartPlay()
{
	// Mover comparison intentionally installs a specialized component-filtering
	// slot. Reset before ordinary actors begin play so returning via server travel
	// cannot leave the standard Character checkpoint on that specialized policy.
	if (USaveManager* SaveManager = USaveManager::Get(this))
	{
		SaveManager->ResetActiveSlot();
	}

	// KinetiForge's shipped vehicles remain untouched plugin Blueprints. The
	// lab adds only its solver-agnostic door/seat/parking wrapper at runtime so
	// they can participate in the same walk-up A/B test as stock Chaos.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* VehiclePawn = *It;
		if (VehiclePawn != nullptr &&
			VehiclePawn->FindComponentByClass<UVehicleDriveAssemblyComponent>() != nullptr &&
			VehiclePawn->FindComponentByClass<UOWSStockVehicleInteractionComponent>() == nullptr)
		{
			UOWSStockVehicleInteractionComponent* Interaction =
				NewObject<UOWSStockVehicleInteractionComponent>(
					VehiclePawn,
					TEXT("OWSKinetiForgeInteraction"));
			VehiclePawn->AddInstanceComponent(Interaction);
			Interaction->RegisterComponent();
		}
	}

	Super::StartPlay();

	// KinetiForge's stock DefaultVehicle Blueprint starts its engine in
	// ReceiveBeginPlay. Reassert the lab's vehicle-agnostic unoccupied policy
	// after every actor has completed BeginPlay: engine off, inputs returned,
	// and parking lock engaged. No KinetiForge simulation setting is retuned.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* VehiclePawn = *It;
		if (VehiclePawn != nullptr &&
			VehiclePawn->FindComponentByClass<UVehicleDriveAssemblyComponent>() != nullptr)
		{
			if (UOWSStockVehicleInteractionComponent* Interaction =
				VehiclePawn->FindComponentByClass<UOWSStockVehicleInteractionComponent>())
			{
				Interaction->SetDriverPresent(false);
			}
		}
	}
}

AOWSTestLabMoverGameMode::AOWSTestLabMoverGameMode()
{
	DefaultPawnClass = AOWSMoverComparisonPawn::StaticClass();
	PlayerControllerClass = AOWSTestLabPlayerController::StaticClass();
	HUDClass = AOWSTestLabHUD::StaticClass();
}
