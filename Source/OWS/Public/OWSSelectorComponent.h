#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"
#include "Templates/SharedPointer.h"

#include "OWSSelectorComponent.generated.h"

class AActor;
class APlayerController;
class UOWSInteractionTargetComponent;
class STextBlock;
class SWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOWSActivateTargetSignature,
	AActor*, TargetActor,
	UOWSInteractionTargetComponent*, InteractionTarget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOWSActivateTargetFailedSignature,
	AActor*, TargetActor,
	UOWSInteractionTargetComponent*, InteractionTarget,
	FText, FailureReason);

UENUM(BlueprintType)
enum class EOWSRangeSelectorShape : uint8
{
	Sphere,
	Cone
};

/** One editable range test in an OWS selector function's ordered stack. */
USTRUCT(BlueprintType)
struct OWS_API FOWSRangeSelector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	EOWSRangeSelectorShape Shape = EOWSRangeSelectorShape::Cone;

	/** Sphere radius in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector", meta=(ClampMin="1.0", EditCondition="Shape == EOWSRangeSelectorShape::Sphere", EditConditionHides))
	float Radius = 125.0f;

	/** Cone length in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector", meta=(ClampMin="1.0", EditCondition="Shape == EOWSRangeSelectorShape::Cone", EditConditionHides))
	float Length = 800.0f;

	/** Cone half-angle in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector", meta=(ClampMin="0.1", ClampMax="89.0", EditCondition="Shape == EOWSRangeSelectorShape::Cone", EditConditionHides))
	float HalfAngleDegrees = 30.0f;

	/** Collision object categories this stack entry can detect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	TArray<TEnumAsByte<ECollisionChannel>> DetectableObjectTypes;

	/** False is intended for reach selectors whose targets only need to be within reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	bool bRequiresFacing = true;
};

/** A separately bindable selector function containing its own modular range stack. */
USTRUCT(BlueprintType)
struct OWS_API FOWSSelectorFunction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	FName Name = TEXT("Activate");

	/** Input bindings that dispatch this selector function for the current target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	TArray<FKey> ActivationKeys;

	/** Ordered, addable, removable selector stack. At least one entry must remain enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Selector")
	TArray<FOWSRangeSelector> SelectorStack;
};

/**
 * OWS targeting component. The modular range stack passively discovers nearby
 * interactables for awareness/highlighting. The independent precision ray owns
 * the current target, debug readout, and Activate dispatch.
 */
UCLASS(ClassGroup=(OWS), meta=(BlueprintSpawnableComponent))
class OWS_API UOWSSelectorComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWSSelectorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Selector")
	TArray<FOWSSelectorFunction> SelectorFunctions;

	/** Independent precision-ray reach in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Selector|Precision Ray", meta=(ClampMin="1.0"))
	float PrecisionRayLength = 4000.0f;

	/** Collision object categories the precision ray can target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Selector|Precision Ray")
	TArray<TEnumAsByte<ECollisionChannel>> PrecisionRayObjectTypes;

	/** Fired for the target already resolved by the selector when Activate is pressed. */
	UPROPERTY(BlueprintAssignable, Category="OWS|Selector")
	FOWSActivateTargetSignature OnActivateTarget;

	/** Fired when the detected object has no usable target or its behavior rejects activation. */
	UPROPERTY(BlueprintAssignable, Category="OWS|Selector")
	FOWSActivateTargetFailedSignature OnActivateTargetFailed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Selector|Debug")
	bool bShowEditorVisualization = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Selector|Debug")
	FColor EditorVisualizationColor = FColor(40, 210, 255);

	UFUNCTION(BlueprintPure, Category="OWS|Selector")
	AActor* GetDetectedActor() const { return DetectedActor.Get(); }

	UFUNCTION(BlueprintPure, Category="OWS|Selector")
	UOWSInteractionTargetComponent* GetDetectedInteractionTarget() const
	{
		return DetectedInteractionTarget.Get();
	}

	/** Passive range-stack results reserved for awareness/highlighting. */
	UFUNCTION(BlueprintPure, Category="OWS|Selector")
	TArray<AActor*> GetAwarenessActors() const;

	/** Dispatches the current interaction point through the shared OWS contract. */
	UFUNCTION(BlueprintCallable, Category="OWS|Selector")
	bool ActivateCurrentTarget();

	/** Immediately reevaluates selection instead of waiting for the next tick. */
	UFUNCTION(BlueprintCallable, Category="OWS|Selector")
	AActor* RefreshSelection();

	UFUNCTION(BlueprintPure, Category="OWS|Selector")
	FText GetDetectedActorName() const;

	/** True when every selector function retains at least one enabled stack entry. */
	UFUNCTION(BlueprintPure, Category="OWS|Selector")
	bool HasValidSelectorStacks() const;

	/** Runtime diagnostic used by automated and manual smoke tests. */
	UFUNCTION(BlueprintPure, Category="OWS|Selector|Debug")
	bool IsDebugReadoutMounted() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsureDefaultConfiguration();
	void UpdateDetectedActor();
	void UpdateActivation(APlayerController& Controller);
	void EnsureDebugWidget(APlayerController& Controller);
	void SetDebugReadoutText(const FText& ActorName);
	void DrawEditorVisualization() const;
	bool IsInsideRange(const AActor& Candidate, const FOWSRangeSelector& Selector, const FVector& Origin, const FVector& Forward) const;
	static bool IsLocationInsideRange(const FVector& Location, float Radius, const FOWSRangeSelector& Selector, const FVector& Origin, const FVector& Forward);
	bool ResolvePrecisionRay(APlayerController& Controller, FVector& OutOrigin, FVector& OutDirection, float& OutLength) const;
	static bool IsActorOrOwnedBy(const AActor* HitActor, const AActor& Candidate);
	void UpdateAwarenessActors(
		const ACharacter& Character,
		const FVector& HeadOrigin,
		const FVector& HeadForward);

	UPROPERTY(Transient)
	TObjectPtr<AActor> DetectedActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UOWSInteractionTargetComponent> DetectedInteractionTarget = nullptr;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> AwarenessActors;

	TSharedPtr<SWidget> DebugReadoutContainer;
	TSharedPtr<STextBlock> DebugReadoutText;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastLoggedDetectedActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UOWSInteractionTargetComponent> LastLoggedDetectedInteractionTarget = nullptr;

	bool bActivationKeyWasDown = false;
};
