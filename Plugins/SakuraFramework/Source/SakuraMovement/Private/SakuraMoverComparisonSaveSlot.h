#pragma once

#include "SaveSlot.h"

#include "SakuraMoverComparisonSaveSlot.generated.h"

/** Runtime-only slot policy used exclusively by the isolated Mover comparison. */
UCLASS()
class USakuraMoverComparisonSaveSlot final : public USaveSlot
{
	GENERATED_BODY()

public:
	USakuraMoverComparisonSaveSlot();
};
