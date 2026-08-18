#pragma once

#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"

#include "SakuraMovementTypes.h"
#include "SakuraMoverComparisonPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UCharacterMoverComponent;
class UInputAction;
class UInputMappingContext;
class USakuraMoverSaveRestoreComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
struct FInputActionValue;

/**
 * Isolated native Mover comparison pawn.
 *
 * It is not the project's default pawn. Run it through
 * ASakuraMoverComparisonGameMode (world override or ?game= URL) when comparing
 * Epic Mover with the standard Character-based Sakura prototype.
 */
UCLASS(Blueprintable)
class SAKURAMOVEMENT_API ASakuraMoverComparisonPawn final : public APawn,
	public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	ASakuraMoverComparisonPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Comparison")
	UCharacterMoverComponent* GetMoverComponent() const { return MoverComponent; }

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Comparison")
	USakuraMoverSaveRestoreComponent* GetSaveRestoreComponent() const { return SaveRestoreComponent; }

	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Comparison")
	void RequestSaveComparisonCheckpoint();

	UFUNCTION(BlueprintCallable, Category = "Sakura|Movement|Comparison")
	void RequestLoadComparisonCheckpoint();

	UFUNCTION(BlueprintPure, Category = "Sakura|Movement|Comparison")
	ESakuraGroundedMovementResult GetLastCheckpointResult() const { return LastCheckpointResult; }

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
	void SetCheckpointResult(ESakuraGroundedMovementResult NewResult, const TCHAR* Operation);
	void DisplayCheckpointResult(const TCHAR* Operation) const;

	UFUNCTION()
	void OnRep_LastCheckpointResult();

	UFUNCTION()
	void HandleAutomaticRestoreEvaluated(
		FName LoadedSlotName,
		ESakuraGroundedMovementResult Result);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSaveComparisonCheckpoint();
	bool ServerSaveComparisonCheckpoint_Validate();
	void ServerSaveComparisonCheckpoint_Implementation();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLoadComparisonCheckpoint();
	bool ServerLoadComparisonCheckpoint_Validate();
	void ServerLoadComparisonCheckpoint_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterMoverComponent> MoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Movement|Comparison", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USakuraMoverSaveRestoreComponent> SaveRestoreComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Sakura|Movement|Comparison|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(ReplicatedUsing = OnRep_LastCheckpointResult, VisibleInstanceOnly, Category = "Sakura|Movement|Comparison")
	ESakuraGroundedMovementResult LastCheckpointResult =
		ESakuraGroundedMovementResult::MissingCheckpoint;

	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	bool bJumpPressed = false;
	bool bJumpJustPressed = false;

	static const FName ComparisonSaveSlot;
};
