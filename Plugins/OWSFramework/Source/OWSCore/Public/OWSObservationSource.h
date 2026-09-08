#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OWSObservationSource.generated.h"

/** Native observation contract, independent of the consuming game's module. */
UINTERFACE(MinimalAPI)
class UOWSObservationSource : public UInterface
{
	GENERATED_BODY()
};

class OWSCORE_API IOWSObservationSource
{
	GENERATED_BODY()
public:
	virtual FString CaptureObservation(bool bPrepareDestinations = false) = 0;
};
