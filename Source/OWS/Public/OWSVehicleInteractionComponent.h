#pragma once

#include "Components/ActorComponent.h"

#include "OWSVehicleInteractionComponent.generated.h"

class ACharacter;
class APawn;
class APlayerController;
class UInputComponent;
class UOWSStockVehicleInteractionComponent;

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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InstallInput();
	void RemoveInput();
	void HandleEnterPressed();
	void HandleExitPressed();
	void HandleExitReleased();
	void CompleteHeldExit();
	bool TryEnterVehicle();
	bool TryExitVehicle();
	APawn* FindEnterableVehicle(
		ACharacter& Character,
		UOWSStockVehicleInteractionComponent*& OutInteraction,
		FName& OutDoorId,
		FName& OutSeatId) const;
	bool PlaceCharacterAtDriverDoor(
		ACharacter& Character,
		const APawn& Vehicle,
		const UOWSStockVehicleInteractionComponent& Interaction) const;
	float GetVehicleSpeedMph() const;
	void ApplyVehicleInputContext(bool bVehicleActive) const;

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> InteractionInput = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> HomeCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OccupiedVehicle = nullptr;

	UPROPERTY(Transient)
	FName OccupiedSeatId = NAME_None;

	FTimerHandle ExitHoldTimer;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float MaximumEnterDistance = 500.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float ImmediateExitSpeedMph = 5.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.1"))
	float MovingExitHoldSeconds = 2.0f;

	UPROPERTY(EditAnywhere, Category="OWS|Vehicle", meta=(ClampMin="0.0"))
	float DriverDoorExitPadding = 75.0f;
};
