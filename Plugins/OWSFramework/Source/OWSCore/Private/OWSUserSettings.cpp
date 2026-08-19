// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "OWSUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSUserSettings)

void UOWSUserSettings::SetVehicleAutoPlacementEnabled(const bool bEnabled)
{
	bVehicleAutoPlacementEnabled = bEnabled;
	SaveSettings();
}
