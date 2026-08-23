#pragma once

#include "Components/ActorComponent.h"
#include "OWSInteractionTargetComponent.h"

#include "OWSLevelButtonInteractionComponent.generated.h"

/**
 * Opt-in adapter for the six menu/style LevelButton instances in the OWS
 * course. It disables their legacy step trigger and routes selector Activate
 * through the Blueprint's existing SimulatePress event.
 */
UCLASS(ClassGroup=(OWS), meta=(BlueprintSpawnableComponent))
class OWS_API UOWSLevelButtonInteractionComponent final
	: public UActorComponent
	, public IOWSInteractionTargetHandler
{
	GENERATED_BODY()

public:
	UOWSLevelButtonInteractionComponent();

	virtual void BeginPlay() override;

	virtual bool CanActivateOWSTarget_Implementation(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason) override;

	virtual bool ActivateOWSTarget_Implementation(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason) override;

private:
	static const FName TriggerComponentName;
	static const FName SimulatePressFunctionName;

	bool OwnsTarget(const UOWSInteractionTargetComponent* Target) const;
};
