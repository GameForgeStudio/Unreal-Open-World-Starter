// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SakuraUserSettings)

void USakuraUserSettings::SetVehicleAutoPlacementEnabled(const bool bEnabled)
{
	bVehicleAutoPlacementEnabled = bEnabled;
	SaveSettings();
}
