// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "OmniWalkDemoCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "OmniWalkPro.h"
#include "UObject/ConstructorHelpers.h"

AOmniWalkDemoCharacter::AOmniWalkDemoCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SurfaceMobility = CreateDefaultSubobject<UOmniWalkPro>(TEXT("SurfaceMobility"));

	DemoBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DemoBody"));
	DemoBody->SetupAttachment(GetCapsuleComponent());
	DemoBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DemoBody->SetRelativeLocation(FVector(0.0f, 0.0f, -44.0f));
	DemoBody->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DemoBody->SetStaticMesh(CubeMesh.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->bUsePawnControlRotation = true;
	// The test world is curved. Do not let the boom retract into the planet.
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AOmniWalkDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AOmniWalkDemoCharacter::MoveForwardPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &AOmniWalkDemoCharacter::MoveForwardReleased);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &AOmniWalkDemoCharacter::MoveBackwardPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &AOmniWalkDemoCharacter::MoveBackwardReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AOmniWalkDemoCharacter::MoveRightPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &AOmniWalkDemoCharacter::MoveRightReleased);
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &AOmniWalkDemoCharacter::MoveLeftPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &AOmniWalkDemoCharacter::MoveLeftReleased);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AOmniWalkDemoCharacter::StartJump);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AOmniWalkDemoCharacter::StopJump);
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOmniWalkDemoCharacter::LookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOmniWalkDemoCharacter::LookPitch);
}

void AOmniWalkDemoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Controller)
	{
		const float ForwardValue = (bMoveForwardPressed ? 1.0f : 0.0f) - (bMoveBackwardPressed ? 1.0f : 0.0f);
		const float RightValue = (bMoveRightPressed ? 1.0f : 0.0f) - (bMoveLeftPressed ? 1.0f : 0.0f);
		if (!FMath::IsNearlyZero(ForwardValue))
		{
			AddMovementInput(GetActorForwardVector(), ForwardValue);
		}
		if (!FMath::IsNearlyZero(RightValue))
		{
			AddMovementInput(GetActorRightVector(), RightValue);
		}
	}
}

void AOmniWalkDemoCharacter::MoveForwardPressed() { bMoveForwardPressed = true; }
void AOmniWalkDemoCharacter::MoveForwardReleased() { bMoveForwardPressed = false; }
void AOmniWalkDemoCharacter::MoveBackwardPressed() { bMoveBackwardPressed = true; }
void AOmniWalkDemoCharacter::MoveBackwardReleased() { bMoveBackwardPressed = false; }
void AOmniWalkDemoCharacter::MoveRightPressed() { bMoveRightPressed = true; }
void AOmniWalkDemoCharacter::MoveRightReleased() { bMoveRightPressed = false; }
void AOmniWalkDemoCharacter::MoveLeftPressed() { bMoveLeftPressed = true; }
void AOmniWalkDemoCharacter::MoveLeftReleased() { bMoveLeftPressed = false; }

void AOmniWalkDemoCharacter::LookYaw(float Value) { AddControllerYawInput(Value); }
void AOmniWalkDemoCharacter::LookPitch(float Value) { AddControllerPitchInput(-Value); }

void AOmniWalkDemoCharacter::StartJump()
{
	Jump();
}

void AOmniWalkDemoCharacter::StopJump()
{
	StopJumping();
}
