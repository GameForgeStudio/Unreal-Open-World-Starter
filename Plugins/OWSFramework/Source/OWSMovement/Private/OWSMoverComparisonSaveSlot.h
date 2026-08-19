#pragma once

#include "SaveSlot.h"

#include "OWSMoverComparisonSaveSlot.generated.h"

/** Runtime-only slot policy used exclusively by the isolated Mover comparison. */
UCLASS()
class UOWSMoverComparisonSaveSlot final : public USaveSlot
{
	GENERATED_BODY()

public:
	UOWSMoverComparisonSaveSlot();
};
