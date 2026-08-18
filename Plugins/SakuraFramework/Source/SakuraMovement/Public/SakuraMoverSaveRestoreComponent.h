#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SakuraMovementTypes.h"

#include "SakuraMoverSaveRestoreComponent.generated.h"

class UMoverComponent;
class USaveManager;
class USaveSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSakuraGroundedRestoreEvaluated,
	FName,
	LoadedSlotName,
	ESakuraGroundedMovementResult,
	Result);

/**
 * Experimental bridge between Save Extension and Mover.
 *
 * Attach this component to an actor that already owns a UMoverComponent. The
 * active Save Extension slot MUST allow this component class in its Component
 * Filter, otherwise RequestGroundedSave is rejected before any save starts.
 *
 * This is intentionally a grounded comparison seam, not general Mover state
 * serialization. Capture accepts only an exact Walking state with no pending
 * mode transition, layered moves, or movement modifiers. Restore never writes
 * an Actor transform directly: it queues FTeleportEffect and then a zero-
 * velocity FApplyVelocityEffect forced to Walking.
 */
UCLASS(ClassGroup = (Sakura), meta = (BlueprintSpawnableComponent))
class SAKURAMOVEMENT_API USakuraMoverSaveRestoreComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	USakuraMoverSaveRestoreComponent();

	/** Captures the owner's current transform only when the strict grounded baseline is valid. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	ESakuraGroundedMovementResult CaptureGroundedCheckpoint();

	/** Validates, captures, and asks Save Extension to save the named slot. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	ESakuraGroundedMovementResult RequestGroundedSave(FName SlotName);

	/**
	 * Asks Save Extension to load a slot containing a grounded checkpoint.
	 * Current layered moves/modifiers must already be absent; this component
	 * never tries to serialize, cancel, or reconstruct them.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	ESakuraGroundedMovementResult RequestGroundedLoad(FName SlotName);

	/**
	 * Queues the saved transform through Mover, followed by zero velocity and
	 * Walking mode. Success means queued, not that teleport collision succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	ESakuraGroundedMovementResult RestoreGroundedCheckpoint();

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Grounded Save")
	bool CanCaptureGroundedCheckpoint(
		UPARAM(DisplayName = "Result") ESakuraGroundedMovementResult& OutResult) const;

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Grounded Save")
	bool HasGroundedCheckpoint() const
	{
		return GroundedCheckpoint.bIsValid;
	}

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Grounded Save")
	FSakuraGroundedMovementCheckpoint GetGroundedCheckpoint() const
	{
		return GroundedCheckpoint;
	}

	/** Re-attempts delegate binding if BeginPlay ran before Save Extension was available. */
	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	bool BindToSaveExtension();

	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Grounded Save")
	void UnbindFromSaveExtension();

	/** Broadcast after every automatic restore attempt made from OnGameLoaded. */
	UPROPERTY(BlueprintAssignable, Category = "Sakura|Movement|Grounded Save")
	FSakuraGroundedRestoreEvaluated OnGroundedRestoreEvaluated;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Sakura|Movement|Grounded Save")
	ESakuraGroundedMovementResult LastAutomaticRestoreResult =
		ESakuraGroundedMovementResult::MissingCheckpoint;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Requires Save Extension's component filter to include this class. */
	UPROPERTY(SaveGame, VisibleInstanceOnly, Category = "Sakura|Movement|Grounded Save")
	FSakuraGroundedMovementCheckpoint GroundedCheckpoint;

	UFUNCTION()
	void HandleSaveExtensionGameLoaded(USaveSlot* LoadedSlot);

	UMoverComponent* FindMoverComponent() const;
	USaveManager* FindSaveManager() const;

	ESakuraGroundedMovementResult ValidateCaptureState() const;
	ESakuraGroundedMovementResult ValidateRestoreState() const;
	ESakuraGroundedMovementResult ValidateRestoreEnvironment() const;
	ESakuraGroundedMovementResult ValidateSlotPersistsComponent(USaveSlot* Slot) const;
	bool IsOwnerPersistedBySlot(USaveSlot& Slot) const;

	bool HasTransientMoverState(const UMoverComponent& MoverComponent,
		bool& bOutHasLayeredMoves, bool& bOutHasMovementModifiers) const;
};
