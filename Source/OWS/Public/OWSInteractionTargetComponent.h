#pragma once

#include "Components/SceneComponent.h"
#include "UObject/Interface.h"

#include "OWSInteractionTargetComponent.generated.h"

class AController;
class UOWSInteractionTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOWSInteractionActivatedSignature,
	AController*, Activator,
	UOWSInteractionTargetComponent*, Target);

/**
 * Optional behavior contract for an actor or actor component that owns OWS
 * interaction targets. Selection stays client-local; implementations that
 * mutate world state are responsible for forwarding the request to authority.
 */
UINTERFACE(BlueprintType)
class OWS_API UOWSInteractionTargetHandler : public UInterface
{
	GENERATED_BODY()
};

class OWS_API IOWSInteractionTargetHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="OWS|Interaction")
	bool CanActivateOWSTarget(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="OWS|Interaction")
	bool ActivateOWSTarget(
		UOWSInteractionTargetComponent* Target,
		AController* Activator,
		FText& OutFailureReason);
};

/**
 * One authored, reusable interaction point. Multiple targets may belong to one
 * actor (for example one target per vehicle door).
 */
UCLASS(Blueprintable, ClassGroup=(OWS), meta=(BlueprintSpawnableComponent))
class OWS_API UOWSInteractionTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UOWSInteractionTargetComponent();

	/** Stable identifier interpreted by the target's behavior handler. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Interaction")
	FName InteractionId = NAME_None;

	/** Optional user-facing label; actor label plus InteractionId is the fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Interaction")
	FText DisplayName;

	/** Disabled targets remain authorable but cannot be selected or activated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Interaction")
	bool bInteractionEnabled = true;

	/** Extra point radius used by the range stack, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Interaction", meta=(ClampMin="0.0"))
	float SelectionRadius = 75.0f;

	/** Observation/event hook for Blueprint-authored targets. */
	UPROPERTY(BlueprintAssignable, Category="OWS|Interaction")
	FOWSInteractionActivatedSignature OnActivated;

	UFUNCTION(BlueprintPure, Category="OWS|Interaction")
	FText GetInteractionDisplayName() const;

	UFUNCTION(BlueprintCallable, Category="OWS|Interaction")
	bool CanActivate(AController* Activator, FText& OutFailureReason) const;

	UFUNCTION(BlueprintCallable, Category="OWS|Interaction")
	bool TryActivate(AController* Activator, FText& OutFailureReason);

private:
	UObject* FindBehaviorHandler() const;
};
