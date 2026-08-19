// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "GameFramework/GameUserSettings.h"
#include "OWSUserSettings.generated.h"

/** Persistent player preferences exposed now for the future settings menu. */
UCLASS(Config = GameUserSettings)
class OWSCORE_API UOWSUserSettings final : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "OWS|Settings|Vehicles")
	bool IsVehicleAutoPlacementEnabled() const
	{
		return bVehicleAutoPlacementEnabled;
	}

	/** The future checkbox calls this; the preference is persisted immediately. */
	UFUNCTION(BlueprintCallable, Category = "OWS|Settings|Vehicles")
	void SetVehicleAutoPlacementEnabled(bool bEnabled);

private:
	/**
	 * On: an empty control seat overrides door-local ordering.
	 * Off: only the faced door's primary seat may be entered; any occupied or
	 * physically blocked primary seat rejects entry with no fallback.
	 */
	UPROPERTY(Config)
	bool bVehicleAutoPlacementEnabled = true;
};
