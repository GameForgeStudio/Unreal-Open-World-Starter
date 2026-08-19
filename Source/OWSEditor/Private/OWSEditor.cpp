#include "OWSGenerateOpenWorldLabCommandlet.h"

#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "HAL/PlatformMisc.h"
#include "LevelEditorViewport.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionEditorLoaderAdapter.h"

namespace
{
	constexpr uint32 DefaultOpenWorldSeed = 1396788053u;
	const TCHAR* const ExpectedOpenWorldMap = TEXT("/Game/OWSPrototype/Maps/L_OWSTestLab_WP");
	constexpr double OpenWorldHalfExtentCm = 406400.0;
	constexpr double EditorLoadPaddingCm = 5000.0;
}

class FOWSEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("OWSGenerateOpenWorldLabEditor")))
		{
			Seed = DefaultOpenWorldSeed;
			FString SeedString;
			if (FParse::Value(FCommandLine::Get(), TEXT("OWSOpenWorldSeed="), SeedString)
				&& !LexTryParseString(Seed, *SeedString))
			{
				UE_LOG(LogTemp, Error, TEXT("Invalid -OWSOpenWorldSeed value '%s'."), *SeedString);
				FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("OWS invalid open-world seed"));
				return;
			}

			GenerationTicker = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FOWSEditorModule::TickEditorGeneration));
			return;
		}

		OpenWorldVisibilityTicker = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FOWSEditorModule::TickOpenWorldVisibility));
	}

	virtual void ShutdownModule() override
	{
		if (GenerationTicker.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GenerationTicker);
			GenerationTicker.Reset();
		}
		if (OpenWorldVisibilityTicker.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(OpenWorldVisibilityTicker);
			OpenWorldVisibilityTicker.Reset();
		}
	}

private:
	bool TickOpenWorldVisibility(float)
	{
		if (!GEditor)
		{
			return true;
		}

		UWorld* World = GEditor->GetEditorWorldContext().World();
		UWorldPartition* WorldPartition = World ? World->GetWorldPartition() : nullptr;
		const bool bIsExpectedWorld = World
			&& World->GetOutermost()->GetName().Equals(ExpectedOpenWorldMap, ESearchCase::CaseSensitive)
			&& WorldPartition
			&& WorldPartition->IsInitialized();
		if (!bIsExpectedWorld)
		{
			LoadedOpenWorld.Reset();
			OpenWorldLoader.Reset();
			return true;
		}

		if (LoadedOpenWorld.Get() == World && OpenWorldLoader.IsValid())
		{
			return true;
		}

		const double LoadExtent = OpenWorldHalfExtentCm + EditorLoadPaddingCm;
		const FBox LoadBounds(
			FVector(-LoadExtent, -LoadExtent, -200000.0),
			FVector(LoadExtent, LoadExtent, 400000.0));
		UWorldPartitionEditorLoaderAdapter* Loader =
			WorldPartition->CreateEditorLoaderAdapter<FLoaderAdapterShape>(
				World,
				LoadBounds,
				TEXT("OWS Proving Ground - Full Editor Preview"));
		if (!Loader || !Loader->GetLoaderAdapter())
		{
			UE_LOG(LogTemp, Error, TEXT("Could not create the OWS proving-ground editor loading region."));
			return true;
		}

		Loader->GetLoaderAdapter()->SetUserCreated(true);
		Loader->GetLoaderAdapter()->Load();
		LoadedOpenWorld = World;
		OpenWorldLoader = Loader;

		const FVector OverviewLocation(-520000.0, -520000.0, 500000.0);
		const FRotator OverviewRotation = (FVector::ZeroVector - OverviewLocation).Rotation();
		for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
		{
			if (ViewportClient && ViewportClient->IsPerspective())
			{
				ViewportClient->SetViewLocation(OverviewLocation);
				ViewportClient->SetViewRotation(OverviewRotation);
				ViewportClient->Invalidate();
			}
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("OWS proving ground: loaded full 8.128 km editor preview and positioned overview camera."));
		return true;
	}

	bool TickEditorGeneration(float)
	{
		if (!GEditor)
		{
			return true;
		}

		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World
			|| !World->GetOutermost()->GetName().Equals(ExpectedOpenWorldMap, ESearchCase::CaseSensitive)
			|| !World->GetWorldPartition()
			|| !World->GetWorldPartition()->IsInitialized())
		{
			return true;
		}

		GenerationTicker.Reset();
		FString Error;
		const bool bSucceeded = UOWSGenerateOpenWorldLabCommandlet::GenerateEditorWorld(World, Seed, Error);
		if (bSucceeded)
		{
			UE_LOG(LogTemp, Display, TEXT("OWS_EDITOR_GENERATION_SUCCESS"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("OWS editor generation failed: %s"), *Error);
		}

		FPlatformMisc::RequestExitWithStatus(
			false,
			bSucceeded ? 0 : 1,
			TEXT("OWS editor open-world generation finished"));
		return false;
	}

	FTSTicker::FDelegateHandle GenerationTicker;
	FTSTicker::FDelegateHandle OpenWorldVisibilityTicker;
	TWeakObjectPtr<UWorld> LoadedOpenWorld;
	TWeakObjectPtr<UWorldPartitionEditorLoaderAdapter> OpenWorldLoader;
	uint32 Seed = DefaultOpenWorldSeed;
};

IMPLEMENT_MODULE(FOWSEditorModule, OWSEditor)
