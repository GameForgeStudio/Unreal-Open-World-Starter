// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OmniWalkDemoGenerator.generated.h" // <--- MUST BE THE LAST INCLUDE

UCLASS(Blueprintable)
class OMNIWALK_API AOmniWalkDemoGenerator : public AActor
{
	GENERATED_BODY()

public:
	AOmniWalkDemoGenerator();

private:
	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> Planet;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> StepOne;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> StepTwo;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> MantleBlock;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UStaticMeshComponent> TallBlock;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UPointLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "OmniWalk|Demo")
	TObjectPtr<class UPointLightComponent> FillLight;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> CachedSphereMesh;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> CachedCubeMesh;
};
