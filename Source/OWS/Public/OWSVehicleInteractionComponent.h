#pragma once

#include "Components/ActorComponent.h"

#include "OWSVehicleInteractionComponent.generated.h"

class ACharacter;
class APawn;
class APlayerController;
class UOWSStockVehicleInteractionComponent;
class USkeletalMeshComponent;

/**
 * OWS-owned controller component for moving between an on-foot OWS character
 * and compatible vehicles. It survives possession changes because it belongs
 * to the PlayerController rather than either pawn.
 */
UCLASS(ClassGroup=(OWS), meta=(BlueprintSpawnableComponent))
class OWS_API UOWSVehicleInteractionComponent final
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UOWSVehicleInteractionComponent();

	/** Debug helper: enter the nearest faced vehicle (bypasses the key press). */
	UFUNCTION(BlueprintCallable, Category="OWS|Vehicle|Debug")
	bool DebugEnterNearestVehicle();

	/** Debug helper: exit the current vehicle (bypasses the key press). */
	UFUNCTION(BlueprintCallable, Category="OWS|Vehicle|Debug")
	bool DebugExitVehicle();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Polled every frame from TickComponent. Reading raw key state via
	// IsInputKeyDown is immune to Enhanced Input consuming the key, which the
	// previous BindKey approach was not.
	void UpdateInteraction(float DeltaTime);
	void ShowPrompt(const FString& Text) const;
	bool TryEnterVehicle();
	bool TryExitVehicle();
	void EjectCharacterWithPhysics(
		ACharacter& Character,
		const FVector& VehicleVelocity,
		const FVector& DoorDirection);
	/** Ticks the bail-out ragdoll and stands the character back up once settled. */
	void UpdateRagdollRecovery(float DeltaTime);
	void RecoverFromRagdoll();
	/** Moves mesh-parented camera components onto the capsule and back again. */
	void ParkCameraRigsOnCapsule(ACharacter& Character);
	void RestoreCameraRigs(ACharacter& Character);
	APawn* FindEnterableVehicle(
		ACharacter& Character,
		UOWSStockVehicleInteractionComponent*& OutInteraction,
		FName& OutDoorId,
		FName& OutSeatId) const;
	bool PlaceCharacterAtDriverDoor(
		ACharacter& Character,
		const APawn& Vehicle,
		const UOWSStockVehicleInteractionComponent& Interaction) const;
	FVector GetVehicleVelocity() const;
	float GetVehicleSpeedMph() const;
	void ApplyVehicleInputContext(bool bVehicleActive) const;
	/**
	 * Points the camera at the given actor. Possession alone does not move the
	 * view target here (the OWS controller keeps it on itself), which leaves the
	 * vehicle camera unused and the view stuck at the car's origin.
	 */
	void SetCameraTarget(AActor* Target, float BlendSeconds) const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> HomeCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OccupiedVehicle = nullptr;

	UPROPERTY(Transient)
	FName OccupiedSeatId = NAME_None;

	/**
	 * Vehicle the player bailed out of. Re-entry is refused until they have
	 * walked back out of range, so a bail-out cannot be instantly undone.
	 */
	UPROPERTY(Transient)
	TObjectPtr<APawn> ReentryBlockedVehicle = nullptr;

	/** Rising-edge tracker for the interact key so a hold triggers once. */
	bool bInteractKeyWasDown = false;

	/** Seconds the exit key has been held while the vehicle is moving. */
	float ExitHoldElapsed = 0.0f;

	/** Character currently ragdolling after a bail-out (outlives HomeCharacter). */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> RagdollCharacter = nullptr;

	/** True while the character is ragdolling after a bail-out. */
	bool bRagdollActive = false;

	/** Seconds since the bail-out ragdoll started. */
	float RagdollElapsed = 0.0f;

	/** Mesh state captured before ragdolling so it can be restored on recovery. */
	FTransform CachedMeshRelativeTransform = FTransform::Identity;
	FName CachedMeshCollisionProfile = NAME_None;

	/**
	 * Camera components normally parented to the skeletal mesh. They are moved
	 * onto the capsule for the duration of a ragdoll, because a camera bolted to
	 * the mesh tumbles with the body instead of framing it.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> ParkedCameraRigs;
	TArray<FTransform> ParkedCameraRigTransforms;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float MaximumEnterDistance = 500.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float ImmediateExitSpeedMph = 5.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.1"))
	float MovingExitHoldSeconds = 2.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float DriverDoorExitPadding = 75.0f;

	/**
	 * Extra upward velocity (cm/s) added on top of the vehicle's own velocity
	 * when the driver bails out while the car is still moving, so the ragdoll
	 * is thrown clear rather than dragged along the ground.
	 */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float EjectionUpwardBoost = 350.0f;

	/** Seconds to blend the camera when moving between character and vehicle. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|Camera", meta=(ClampMin="0.0"))
	float CameraBlendSeconds = 0.4f;

	/** Sideways shove (cm/s) out of the door when bailing from a moving car. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.0"))
	float EjectionSidePush = 450.0f;

	/** How far outside the door the body starts, so it never spawns in the car. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.0"))
	float BailOutDoorClearance = 110.0f;

	/** Earliest the ragdoll may stand back up after a bail-out. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.0"))
	float RagdollMinimumSeconds = 1.5f;

	/** Hard cap on ragdoll time, even if the body never settles. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.1"))
	float RagdollMaximumSeconds = 6.0f;

	/** Body speed (cm/s) below which the ragdoll counts as settled. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.0"))
	float RagdollSettleSpeed = 150.0f;

	/** Distance the player must retreat before a bailed-out car accepts them. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="0.0"))
	float ReentryReleaseDistance = 550.0f;

	/**
	 * Top speed (cm/s) the camera anchor may travel while chasing the tumbling
	 * body. The gameplay camera rig is tuned for locomotion, so dragging the
	 * anchor at the car's speed makes it lag and collide; capping it near a
	 * sprint keeps the framing identical to running around on foot.
	 */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="1.0"))
	float RagdollAnchorMaxSpeed = 700.0f;

	/** How close the anchor must get to the body before standing back up. */
	UPROPERTY(EditAnywhere, Category="OWS|Vehicle|BailOut", meta=(ClampMin="1.0"))
	float RagdollAnchorCatchUpDistance = 200.0f;
};
