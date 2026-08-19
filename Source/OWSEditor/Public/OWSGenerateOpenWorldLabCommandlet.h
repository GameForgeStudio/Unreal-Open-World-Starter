#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "OWSGenerateOpenWorldLabCommandlet.generated.h"

/**
 * Deterministically builds and validates OWS's project-owned 8.128 km
 * World Partition vehicle proving ground. Validation remains commandlet-safe;
 * generation can also be invoked by the editor module's guarded automation
 * path so Landscape receives a normal editor scene/render lifecycle.
 */
UCLASS()
class OWSEDITOR_API UOWSGenerateOpenWorldLabCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UOWSGenerateOpenWorldLabCommandlet();
	virtual int32 Main(const FString& Params) override;

	static bool GenerateEditorWorld(UWorld* World, uint32 Seed, FString& OutError);
};
