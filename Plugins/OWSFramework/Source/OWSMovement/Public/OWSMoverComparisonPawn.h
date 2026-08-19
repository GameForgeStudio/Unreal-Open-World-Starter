#pragma once

#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"

#include "OWSMovementTypes.h"
#include "OWSMoverComparisonPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UCharacterMoverComponent;
class UInputAction;
class UInputMappingContext;
class UOWSMoverSaveRestoreComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
struct FInputActionValue;

/**
 * Isolated native Mover comparison pawn.
 *
 * It is not the project's default pawn. Run it through
 * AOWSMoverComparisonGameMode (world override or ?game= URL) when comparing
 * Epic Mover with the standard Character-based OWS prototype.
 */
UCLASS(Blueprintable)
class OWSMOVEMENT_API AOWSMoverComparisonPawn final : public APawn,
	public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AOWSMoverComparisonPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UFUNCTION(BlueprintPure, Category = "OWS|Movement|Comparison")
	UCharacterMoverComponent* GetMoverComponent() const { return MoverComponent; }

	UFUNCTION(BlueprintPure, Category = "OWS|Movement|Comparison")
	UOWSMoverSaveRestoreComponent* GetSaveRestoreComponent() const { return SaveRestoreComponent; }

	UFUNCTION(BlueprintCallable, Category = "OWS|Movement|Comparison")
	void RequestSaveComparisonCheckpoint();

	UFUNCTION(BlueprintCallable, Category = "OWS|Movement|Comparison")
	void RequestLoadComparisonCheckpoint();

	UFUNCTION(BlueprintPure, Category = "OWS|Movement|Comparison")
	EOWSGroundedMovementResult GetLastCheckpointResult() const { return LastCheckpointResult; }

protected:
	virtual void BeginPlay() override;
	virtual void ProduceInput_Implementation(
		int32 SimTimeMs,
		FMoverInputCmdContext& InputCmdResult) override;

private:
	void AddInputContexts();
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpCompleted(const FInputActionValue& Value);
	void SaveComparisonCheckpointAuthority();
	void LoadComparisonCheckpointAuthority();
	void SetCheckpointResult(EOWSGroundedMovementResult NewResult, const TCHAR* Operation);
	void DisplayCheckpointResult(const TCHAR* Operation) const;

	UFUNCTION()
	void OnRep_LastCheckpointResult();

	UFUNCTION()
	void HandleAutomaticRestoreEvaluated(
		FName LoadedSlotName,
		EOWSGroundedMovementResult Result);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSaveComparisonCheckpoint();
	bool ServerSaveComparisonCheckpoint_Validate();
	void ServerSaveComparisonCheckpoint_Implementation();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLoadComparisonCheckpoint();
	bool ServerLoadComparisonCheckpoint_Validate();
	void ServerLoadComparisonCheckpoint_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterMoverComponent> MoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOWSMoverSaveRestoreComponent> SaveRestoreComponent;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "OWS|Movement|Comparison|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(ReplicatedUsing = OnRep_LastCheckpointResult, VisibleInstanceOnly, Category = "OWS|Movement|Comparison")
	EOWSGroundedMovementResult LastCheckpointResult =
		EOWSGroundedMovementResult::MissingCheckpoint;

	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	bool bJumpPressed = false;
	bool bJumpJustPressed = false;

	static const FName ComparisonSaveSlot;
};
