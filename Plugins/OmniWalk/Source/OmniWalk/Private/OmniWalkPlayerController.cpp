// Copyright (c) 2026 GregOrigin. All Rights Reserved.
#include "OmniWalkPlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"

AOmniWalkPlayerController::AOmniWalkPlayerController() {}

void AOmniWalkPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			77,
			30.0f,
			FColor::Cyan,
			TEXT("OmniWalk sandbox | WASD: move | Mouse: look | Space: jump | Walk around the curved planet to test surface adhesion."));
	}
}

void AOmniWalkPlayerController::UpdateRotation(float DeltaTime)
{
	// Base logic for custom gimbal can go here
	Super::UpdateRotation(DeltaTime);
}

FVector AOmniWalkPlayerController::GetGravityRelativeDirection(FVector WorldDirection) const
{
	if (APawn* P = GetPawn())
	{
		return FRotationMatrix(P->GetActorRotation()).TransformVector(WorldDirection);
	}
	return WorldDirection;
}
