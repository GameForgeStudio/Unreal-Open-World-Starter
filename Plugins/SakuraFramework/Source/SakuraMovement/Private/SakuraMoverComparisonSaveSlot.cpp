#include "SakuraMoverComparisonSaveSlot.h"

#include "SakuraMoverSaveRestoreComponent.h"

USakuraMoverComparisonSaveSlot::USakuraMoverComparisonSaveSlot()
{
	ComponentFilter.AllowedClasses.Add(
		TSoftClassPtr<UObject>(USakuraMoverSaveRestoreComponent::StaticClass()));
}
