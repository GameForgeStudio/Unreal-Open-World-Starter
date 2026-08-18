#include "SakuraTestLabGameMode.h"

#include "SakuraMoverComparisonPawn.h"
#include "SakuraPrototypeCharacter.h"
#include "SakuraStockVehicleInteractionComponent.h"
#include "SakuraTestLabHUD.h"
#include "SakuraTestLabPlayerController.h"
#include "SaveManager.h"
#include "EngineUtils.h"
#include "VehicleDriveAssemblyComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SakuraTestLabGameMode)

ASakuraTestLabGameMode::ASakuraTestLabGameMode()
{
	DefaultPawnClass = ASakuraPrototypeCharacter::StaticClass();
	PlayerControllerClass = ASakuraTestLabPlayerController::StaticClass();
	HUDClass = ASakuraTestLabHUD::StaticClass();
}

void ASakuraTestLabGameMode::StartPlay()
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
			VehiclePawn->FindComponentByClass<USakuraStockVehicleInteractionComponent>() == nullptr)
		{
			USakuraStockVehicleInteractionComponent* Interaction =
				NewObject<USakuraStockVehicleInteractionComponent>(
					VehiclePawn,
					TEXT("SakuraKinetiForgeInteraction"));
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
			if (USakuraStockVehicleInteractionComponent* Interaction =
				VehiclePawn->FindComponentByClass<USakuraStockVehicleInteractionComponent>())
			{
				Interaction->SetDriverPresent(false);
			}
		}
	}
}

ASakuraTestLabMoverGameMode::ASakuraTestLabMoverGameMode()
{
	DefaultPawnClass = ASakuraMoverComparisonPawn::StaticClass();
	PlayerControllerClass = ASakuraTestLabPlayerController::StaticClass();
	HUDClass = ASakuraTestLabHUD::StaticClass();
}
