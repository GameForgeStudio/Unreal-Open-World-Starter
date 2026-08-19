#include "OWSMoverComparisonSaveSlot.h"

#include "OWSMoverSaveRestoreComponent.h"

UOWSMoverComparisonSaveSlot::UOWSMoverComparisonSaveSlot()
{
	ComponentFilter.AllowedClasses.Add(
		TSoftClassPtr<UObject>(UOWSMoverSaveRestoreComponent::StaticClass()));
}
