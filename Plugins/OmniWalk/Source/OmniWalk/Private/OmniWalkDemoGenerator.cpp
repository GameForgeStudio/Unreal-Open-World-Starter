// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "OmniWalkDemoGenerator.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AOmniWalkDemoGenerator::AOmniWalkDemoGenerator()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SceneRoot->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		CachedSphereMesh = SphereMeshAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		CachedCubeMesh = CubeMeshAsset.Object;
	}

	auto ConfigureCourseMesh = [this](UStaticMeshComponent* Mesh, const FVector& Location, const FVector& Scale)
	{
		Mesh->SetupAttachment(SceneRoot);
		Mesh->SetRelativeLocation(Location);
		Mesh->SetRelativeScale3D(Scale);
		Mesh->SetStaticMesh(CachedCubeMesh);
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Mesh->SetMobility(EComponentMobility::Static);
	};

	Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	ConfigureCourseMesh(Ground, FVector(0.0f, 0.0f, -100.0f), FVector(50.0f, 50.0f, 1.0f));
	StepOne = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StepOne"));
	ConfigureCourseMesh(StepOne, FVector(650.0f, 0.0f, 50.0f), FVector(2.5f, 3.0f, 2.5f));
	StepTwo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StepTwo"));
	ConfigureCourseMesh(StepTwo, FVector(1050.0f, 0.0f, 150.0f), FVector(2.5f, 3.0f, 3.5f));
	MantleBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MantleBlock"));
	ConfigureCourseMesh(MantleBlock, FVector(1500.0f, 0.0f, 250.0f), FVector(3.0f, 3.0f, 4.5f));
	TallBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TallBlock"));
	ConfigureCourseMesh(TallBlock, FVector(2400.0f, 0.0f, 500.0f), FVector(4.0f, 4.0f, 9.0f));

	Planet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Planet"));
	Planet->SetupAttachment(SceneRoot);
	Planet->SetRelativeLocation(FVector(-900.0f, 1300.0f, -500.0f));
	Planet->SetRelativeScale3D(FVector(10.0f));
	Planet->SetStaticMesh(CachedSphereMesh);
	Planet->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Planet->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Planet->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Planet->SetMobility(EComponentMobility::Static);
	Planet->SetGenerateOverlapEvents(false);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(SceneRoot);
	KeyLight->SetRelativeLocation(FVector(1600.0f, -1200.0f, 2200.0f));
	KeyLight->SetIntensity(250000.0f);
	KeyLight->SetAttenuationRadius(9000.0f);
	KeyLight->SetLightColor(FLinearColor(0.65f, 0.82f, 1.0f));
	KeyLight->SetMobility(EComponentMobility::Movable);

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);
	FillLight->SetRelativeLocation(FVector(-1800.0f, 1400.0f, 1000.0f));
	FillLight->SetIntensity(150000.0f);
	FillLight->SetAttenuationRadius(9000.0f);
	FillLight->SetLightColor(FLinearColor(1.0f, 0.50f, 0.22f));
	FillLight->SetMobility(EComponentMobility::Movable);

	Tags.Add(FName("OmniWalk.DemoPlanet"));
}
