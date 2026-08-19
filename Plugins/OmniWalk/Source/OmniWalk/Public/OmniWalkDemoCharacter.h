// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OmniWalkDemoCharacter.generated.h"

class UCameraComponent;
class UOmniWalkPro;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class OMNIWALK_API AOmniWalkDemoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOmniWalkDemoCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForwardPressed();
	void MoveForwardReleased();
	void MoveBackwardPressed();
	void MoveBackwardReleased();
	void MoveRightPressed();
	void MoveRightReleased();
	void MoveLeftPressed();
	void MoveLeftReleased();
	void LookYaw(float Value);
	void LookPitch(float Value);
	void StartJump();
	void StopJump();

	bool bMoveForwardPressed = false;
	bool bMoveBackwardPressed = false;
	bool bMoveRightPressed = false;
	bool bMoveLeftPressed = false;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UOmniWalkPro> SurfaceMobility;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> DemoBody;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FollowCamera;
};
