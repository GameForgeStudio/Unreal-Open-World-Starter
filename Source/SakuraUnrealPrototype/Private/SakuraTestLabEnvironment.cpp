// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraTestLabEnvironment.h"

#include "SakuraTestLabPhysicsProp.h"
#include "SakuraTestLabRagdollTarget.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace Sakura::TestLab
{
	constexpr float BasicShapeSize = 100.0f;
	constexpr float RoadSurfaceHeight = 10.0f;
	constexpr float TrafficConeDiameter = 55.0f;
	constexpr float TrafficConeHeight = 75.0f;
	constexpr float TrafficConeMassKg = 2.0f;

	FTransform BoxTransform(
		const FVector& Location,
		const FVector& Dimensions,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return FTransform(Rotation, Location, Dimensions / BasicShapeSize);
	}

	FTransform RoundTransform(
		const FVector& Location,
		const float Diameter,
		const float Height,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return FTransform(
			Rotation,
			Location,
			FVector(Diameter / BasicShapeSize, Diameter / BasicShapeSize, Height / BasicShapeSize));
	}

	void ConfigureGeometry(
		UInstancedStaticMeshComponent* Component,
		USceneComponent* Parent,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		const bool bEnableCollision = true)
	{
		check(Component);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(Mesh);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionEnabled(
			bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Component->SetCollisionProfileName(
			bEnableCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(bEnableCollision);
		Component->SetCastShadow(true);
		if (Material)
		{
			Component->SetMaterial(0, Material);
		}
	}

	void ConfigureSign(
		UTextRenderComponent* Sign,
		USceneComponent* Parent,
		const TCHAR* Text,
		const FVector& Location,
		const FRotator& Rotation,
		const FColor& Color,
		const float WorldSize)
	{
		check(Sign);
		Sign->SetupAttachment(Parent);
		Sign->SetMobility(EComponentMobility::Static);
		Sign->SetRelativeLocationAndRotation(Location, Rotation);
		Sign->SetText(FText::FromString(Text));
		Sign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Sign->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		Sign->SetWorldSize(WorldSize);
		Sign->SetTextRenderColor(Color);
		Sign->SetCastShadow(true);
		Sign->bAlwaysRenderAsText = true;
	}
}

ASakuraTestLabEnvironment::ASakuraTestLabEnvironment()
{
	using namespace Sakura::TestLab;

	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BasicCubeMesh = CubeFinder.Object;
	BasicCylinderMesh = CylinderFinder.Object;
	BasicConeMesh = ConeFinder.Object;
	BasicSphereMesh = SphereFinder.Object;
	BasicShapeMaterial = MaterialFinder.Object;

	GroundGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GroundGeometry"));
	RoadGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadGeometry"));
	RoadMarkerGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadMarkerGeometry"));
	CharacterCourseGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CharacterCourseGeometry"));
	SaveMarkerGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SaveMarkerGeometry"));
	HybridGarageGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HybridGarageGeometry"));
	StockGarageGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StockGarageGeometry"));
	CourseObstacleGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CourseObstacleGeometry"));
	CourseConeGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CourseConeGeometry"));

	ConfigureGeometry(GroundGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(RoadGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(RoadMarkerGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial, false);
	ConfigureGeometry(CharacterCourseGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(SaveMarkerGeometry, SceneRoot, BasicCylinderMesh, BasicShapeMaterial);
	ConfigureGeometry(HybridGarageGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(StockGarageGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(CourseObstacleGeometry, SceneRoot, BasicCubeMesh, BasicShapeMaterial);
	ConfigureGeometry(CourseConeGeometry, SceneRoot, BasicConeMesh, BasicShapeMaterial, false);
	CourseConeGeometry->SetHiddenInGame(true);

	// Overall foundation plus the four station pads around the central hub. The
	// foundation extends well below the surrounding terrain so it reads as an
	// embedded slab instead of casting a visible shadow from a floating edge.
	GroundGeometry->AddInstance(BoxTransform(FVector(1500.0, 0.0, -60.0), FVector(18000.0, 12000.0, 120.0)));
	GroundGeometry->AddInstance(BoxTransform(FVector(-450.0, 0.0, 12.5), FVector(1700.0, 1300.0, 25.0)));
	GroundGeometry->AddInstance(BoxTransform(FVector(-450.0, 1700.0, 15.0), FVector(1500.0, 1000.0, 30.0)));
	GroundGeometry->AddInstance(BoxTransform(FVector(-450.0, -1700.0, 15.0), FVector(1500.0, 1000.0, 30.0)));

	// Rectangular perimeter road and connectors from the central garage.
	RoadGeometry->AddInstance(BoxTransform(FVector(1500.0, -5200.0, 5.0), FVector(14800.0, 1200.0, 10.0)));
	RoadGeometry->AddInstance(BoxTransform(FVector(1500.0, 5200.0, 5.0), FVector(14800.0, 1200.0, 10.0)));
	RoadGeometry->AddInstance(BoxTransform(FVector(-5900.0, 0.0, 5.0), FVector(1200.0, 10400.0, 10.0)));
	RoadGeometry->AddInstance(BoxTransform(FVector(8900.0, 0.0, 5.0), FVector(1200.0, 10400.0, 10.0)));
	RoadGeometry->AddInstance(BoxTransform(FVector(2500.0, -2850.0, 5.0), FVector(1100.0, 3500.0, 10.0)));

	// Four broad, shallow transitions join the roughly Z=-50 landscape to the
	// Z=0 campus surface. Sharp half-meter slab faces stop a low sports car at
	// its bumper before the wheel can engage, so they are not valid road exits.
	constexpr float CampusTransitionPitch = 2.385944f; // atan(50 / 1200)
	const FVector TwoAndAHalfCarDriveway(1201.041f, 700.0f, 20.0f);
	const FVector BroadDriveway(1201.041f, 900.0f, 20.0f);
	// Two west-side parking-lot entrances line up with the vehicle bays and
	// leave an obvious curb between them instead of erasing the whole edge.
	RoadGeometry->AddInstance(BoxTransform(
		FVector(-8100.0, -1000.0, -35.0),
		TwoAndAHalfCarDriveway,
		FRotator(CampusTransitionPitch, 0.0, 0.0)));
	RoadGeometry->AddInstance(BoxTransform(
		FVector(-8100.0, 1000.0, -35.0),
		TwoAndAHalfCarDriveway,
		FRotator(CampusTransitionPitch, 0.0, 0.0)));
	RoadGeometry->AddInstance(BoxTransform(
		FVector(11100.0, 0.0, -35.0),
		BroadDriveway,
		FRotator(-CampusTransitionPitch, 0.0, 0.0)));
	RoadGeometry->AddInstance(BoxTransform(
		FVector(1500.0, -6600.0, -35.0),
		BroadDriveway,
		FRotator(CampusTransitionPitch, 90.0, 0.0)));
	RoadGeometry->AddInstance(BoxTransform(
		FVector(1500.0, 6600.0, -35.0),
		BroadDriveway,
		FRotator(-CampusTransitionPitch, 90.0, 0.0)));

	// Dashed center lines make the large circuit readable from both bays.
	for (float X = -5000.0f; X <= 8000.0f; X += 800.0f)
	{
		RoadMarkerGeometry->AddInstance(BoxTransform(FVector(X, -5200.0, 12.0), FVector(350.0, 18.0, 4.0)));
		RoadMarkerGeometry->AddInstance(BoxTransform(FVector(X, 5200.0, 12.0), FVector(350.0, 18.0, 4.0)));
	}
	for (float Y = -4400.0f; Y <= 4400.0f; Y += 800.0f)
	{
		RoadMarkerGeometry->AddInstance(BoxTransform(FVector(-5900.0, Y, 12.0), FVector(18.0, 350.0, 4.0)));
		RoadMarkerGeometry->AddInstance(BoxTransform(FVector(8900.0, Y, 12.0), FVector(18.0, 350.0, 4.0)));
	}

	// Two clearly separated vehicle bays. Cars are placed at X=1000, Y=+/-700.
	HybridGarageGeometry->AddInstance(BoxTransform(FVector(1000.0, -700.0, 15.0), FVector(850.0, 520.0, 30.0)));
	HybridGarageGeometry->AddInstance(BoxTransform(FVector(550.0, -700.0, 80.0), FVector(20.0, 520.0, 160.0)));
	StockGarageGeometry->AddInstance(BoxTransform(FVector(1000.0, 700.0, 15.0), FVector(850.0, 520.0, 30.0)));
	StockGarageGeometry->AddInstance(BoxTransform(FVector(550.0, 700.0, 80.0), FVector(20.0, 520.0, 160.0)));

	// Save Extension markers: save on one pad, move to another, then load.
	SaveMarkerGeometry->AddInstance(RoundTransform(FVector(-850.0, 1700.0, 25.0), 260.0f, 50.0f));
	SaveMarkerGeometry->AddInstance(RoundTransform(FVector(-450.0, 1700.0, 50.0), 260.0f, 100.0f));
	SaveMarkerGeometry->AddInstance(RoundTransform(FVector(-50.0, 1700.0, 75.0), 260.0f, 150.0f));

	// Curbs of increasing height.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Height = 8.0f + static_cast<float>(Index) * 7.0f;
		CharacterCourseGeometry->AddInstance(BoxTransform(
			FVector(-1500.0 - Index * 300.0, 650.0, Height * 0.5f),
			FVector(230.0, 550.0, Height)));
	}

	// Ten-step stair set with realistic 18 cm rises and 30 cm treads.
	for (int32 Step = 0; Step < 10; ++Step)
	{
		const float Height = 18.0f * static_cast<float>(Step + 1);
		CharacterCourseGeometry->AddInstance(BoxTransform(
			FVector(-1650.0 - Step * 30.0, 2250.0, Height * 0.5f),
			FVector(30.0, 850.0, Height)));
	}

	// Uneven stepping blocks and balance beam.
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const float Height = 35.0f + static_cast<float>((Index % 3) * 25);
		const float YOffset = (Index % 2 == 0) ? -120.0f : 120.0f;
		CharacterCourseGeometry->AddInstance(BoxTransform(
			FVector(-2600.0 - Index * 260.0, -200.0 + YOffset, Height * 0.5f),
			FVector(190.0, 190.0, Height)));
	}
	CharacterCourseGeometry->AddInstance(BoxTransform(FVector(-3700.0, 900.0, 55.0), FVector(1700.0, 90.0, 110.0)));

	// Dedicated 15-degree and 30-degree traversal ramps.
	CharacterCourseGeometry->AddInstance(BoxTransform(
		FVector(-3100.0, -1500.0, 196.0),
		FVector(1400.0, 700.0, 30.0),
		FRotator(15.0, 0.0, 0.0)));
	CharacterCourseGeometry->AddInstance(BoxTransform(
		FVector(-4850.0, -1500.0, 365.0),
		FVector(1400.0, 700.0, 30.0),
		FRotator(30.0, 0.0, 0.0)));

	// Acceleration/braking gates along the long south straight.
	for (float X = -4600.0f; X <= 7600.0f; X += 2000.0f)
	{
		const float ConeCenterZ = RoadSurfaceHeight + TrafficConeHeight * 0.5f;
		CourseConeGeometry->AddInstance(RoundTransform(
			FVector(X, -5580.0, ConeCenterZ),
			TrafficConeDiameter,
			TrafficConeHeight));
		CourseConeGeometry->AddInstance(RoundTransform(
			FVector(X, -4820.0, ConeCenterZ),
			TrafficConeDiameter,
			TrafficConeHeight));
	}

	// Alternating slalom on the north straight.
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const float X = -1000.0f + Index * 650.0f;
		const float Y = 5200.0f + ((Index % 2 == 0) ? -270.0f : 270.0f);
		CourseConeGeometry->AddInstance(RoundTransform(
			FVector(X, Y, RoadSurfaceHeight + TrafficConeHeight * 0.5f),
			TrafficConeDiameter,
			TrafficConeHeight));
	}

	// Speed bumps across the west leg.
	for (int32 Index = 0; Index < 7; ++Index)
	{
		CourseObstacleGeometry->AddInstance(BoxTransform(
			FVector(-5900.0, -3300.0 + Index * 320.0, 13.0),
			FVector(1000.0, 45.0, 26.0)));
	}

	// Dense low roughness blocks on the east leg exercise suspension response.
	for (int32 Row = 0; Row < 8; ++Row)
	{
		for (int32 Column = 0; Column < 4; ++Column)
		{
			const float Height = 8.0f + static_cast<float>((Row + Column) % 3) * 5.0f;
			CourseObstacleGeometry->AddInstance(BoxTransform(
				FVector(8500.0 + Column * 265.0, -1700.0 + Row * 240.0, Height * 0.5f + 10.0f),
				FVector(210.0, 170.0, Height)));
		}
	}

	// Wide vehicle ramps in the infield, with clear run-up and landing slabs.
	CourseObstacleGeometry->AddInstance(BoxTransform(
		FVector(3900.0, 500.0, 283.0),
		FVector(2100.0, 900.0, 45.0),
		FRotator(15.0, 0.0, 0.0)));
	CourseObstacleGeometry->AddInstance(BoxTransform(
		FVector(6500.0, 500.0, 547.0),
		FVector(2100.0, 900.0, 45.0),
		FRotator(30.0, 0.0, 0.0)));
	CourseObstacleGeometry->AddInstance(BoxTransform(FVector(5150.0, 500.0, 295.0), FVector(500.0, 900.0, 45.0)));
	CourseObstacleGeometry->AddInstance(BoxTransform(FVector(7750.0, 500.0, 560.0), FVector(500.0, 900.0, 45.0)));

	HubSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HubSign"));
	InventorySign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InventorySign"));
	SaveSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SaveSign"));
	CharacterCourseSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CharacterCourseSign"));
	HybridGarageSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HybridGarageSign"));
	StockGarageSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StockGarageSign"));
	VehicleCourseSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("VehicleCourseSign"));
	MassComparisonSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MassComparisonSign"));
	BlockWallSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BlockWallSign"));
	RagdollSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RagdollSign"));

	ConfigureSign(
		HubSign,
		SceneRoot,
		TEXT("SAKURA SYSTEMS TEST LAB\nON FOOT: F OR GAMEPAD X = ACTIVATE    M = CHARACTER / MOVER"),
		FVector(1800.0, 0.0, 480.0),
		FRotator(0.0, 180.0, 0.0),
		FColor(140, 235, 255),
		36.0f);
	ConfigureSign(
		InventorySign,
		SceneRoot,
		TEXT("SIGIL INVENTORY + GAS\nE EQUIP    Q UNEQUIP    U POTION    P ABILITY"),
		FVector(-450.0, -1350.0, 220.0),
		FRotator(0.0, 90.0, 0.0),
		FColor(180, 130, 255),
		40.0f);
	ConfigureSign(
		SaveSign,
		SceneRoot,
		TEXT("SAVE EXTENSION\nF5 SAVE    F9 LOAD    PAGE UP / DOWN SCALAR"),
		FVector(-450.0, 1350.0, 230.0),
		FRotator(0.0, -90.0, 0.0),
		FColor(120, 255, 150),
		40.0f);
	ConfigureSign(
		CharacterCourseSign,
		SceneRoot,
		TEXT("ON-FOOT MOVEMENT LAB\nCURBS    STAIRS    BLOCKS    15 DEG / 30 DEG RAMPS"),
		FVector(-1200.0, 3000.0, 250.0),
		FRotator(0.0, 0.0, 0.0),
		FColor(120, 220, 255),
		42.0f);
	ConfigureSign(
		HybridGarageSign,
		SceneRoot,
		TEXT("EPIC STOCK CHAOS MODULAR VEHICLE\nF / GAMEPAD X: ACTIVATE"),
		FVector(560.0, -700.0, 260.0),
		FRotator(0.0, 180.0, 0.0),
		FColor(70, 225, 255),
		36.0f);
	ConfigureSign(
		StockGarageSign,
		SceneRoot,
		TEXT("STOCK KINETIFORGE RWD SPORTS CAR\nF / GAMEPAD X: ACTIVATE"),
		FVector(560.0, 700.0, 260.0),
		FRotator(0.0, 180.0, 0.0),
		FColor(255, 175, 65),
		36.0f);
	ConfigureSign(
		VehicleCourseSign,
		SceneRoot,
		TEXT("VEHICLE DYNAMICS CIRCUIT\nRT ACCELERATE    LT BRAKE / REVERSE    LS STEER    A HANDBRAKE"),
		FVector(2400.0, -4450.0, 250.0),
		FRotator(0.0, -90.0, 0.0),
		FColor(255, 225, 100),
		44.0f);
	ConfigureSign(
		MassComparisonSign,
		SceneRoot,
		TEXT("CHAOS MASS + RESTITUTION\nCUBES: 10 / 100 / 1000 KG\nSPHERES: LOW BOUNCE / HIGH BOUNCE"),
		FVector(2400.0, 3700.0, 280.0),
		FRotator(0.0, -90.0, 0.0),
		FColor(150, 235, 255),
		32.0f);
	ConfigureSign(
		BlockWallSign,
		SceneRoot,
		TEXT("CHAOS RIGID-BODY WALL\n32 INDEPENDENT 8 KG BLOCKS"),
		FVector(4700.0, 3700.0, 280.0),
		FRotator(0.0, -90.0, 0.0),
		FColor(145, 190, 255),
		34.0f);
	ConfigureSign(
		RagdollSign,
		SceneRoot,
		TEXT("PHYSICS-ASSET RAGDOLLS\nVEHICLE IMPACT ACTIVATES FULL-BODY CHAOS"),
		FVector(7050.0, 3700.0, 280.0),
		FRotator(0.0, -90.0, 0.0),
		FColor(255, 150, 180),
		32.0f);
}

void ASakuraTestLabEnvironment::BeginPlay()
{
	Super::BeginPlay();
	CourseConeGeometry->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CourseConeGeometry->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	CourseConeGeometry->SetCanEverAffectNavigation(false);
	CourseConeGeometry->SetHiddenInGame(true);

	if (HasAuthority())
	{
		SpawnRuntimeShowcase();
	}
}

void ASakuraTestLabEnvironment::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		for (AActor* SpawnedActor : SpawnedShowcaseActors)
		{
			if (IsValid(SpawnedActor) && !SpawnedActor->IsActorBeingDestroyed())
			{
				SpawnedActor->Destroy();
			}
		}
	}
	SpawnedShowcaseActors.Reset();

	Super::EndPlay(EndPlayReason);
}

ASakuraTestLabPhysicsProp* ASakuraTestLabEnvironment::SpawnPhysicsProp(
	const FName& ShowcaseTag,
	UStaticMesh* Mesh,
	const FTransform& RelativeTransform,
	const float MassInKg,
	const FLinearColor& Color,
	const float Roughness,
	const bool bUseContinuousCollisionDetection,
	const float PhysicalFriction,
	const float Restitution)
{
	if (!GetWorld() || !Mesh)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform WorldTransform = RelativeTransform * GetActorTransform();
	ASakuraTestLabPhysicsProp* Prop = GetWorld()->SpawnActor<ASakuraTestLabPhysicsProp>(
		ASakuraTestLabPhysicsProp::StaticClass(),
		WorldTransform,
		SpawnParameters);
	if (!Prop)
	{
		return nullptr;
	}

	Prop->InitializeShowcaseProp(
		Mesh,
		MassInKg,
		Color,
		Roughness,
		ShowcaseTag,
		bUseContinuousCollisionDetection,
		PhysicalFriction,
		Restitution);

	SpawnedShowcaseActors.Add(Prop);
	return Prop;
}

void ASakuraTestLabEnvironment::SpawnRuntimeShowcase()
{
	using namespace Sakura::TestLab;

	SpawnedShowcaseActors.Reset();

	const float ConeCenterZ = RoadSurfaceHeight + TrafficConeHeight * 0.5f;
	for (float X = -4600.0f; X <= 7600.0f; X += 2000.0f)
	{
		SpawnPhysicsProp(
			TEXT("Sakura.Physics.Cone"),
			BasicConeMesh,
			RoundTransform(FVector(X, -5580.0, ConeCenterZ), TrafficConeDiameter, TrafficConeHeight),
			TrafficConeMassKg,
			FLinearColor(1.0f, 0.23f, 0.015f),
			0.62f);
		SpawnPhysicsProp(
			TEXT("Sakura.Physics.Cone"),
			BasicConeMesh,
			RoundTransform(FVector(X, -4820.0, ConeCenterZ), TrafficConeDiameter, TrafficConeHeight),
			TrafficConeMassKg,
			FLinearColor(1.0f, 0.23f, 0.015f),
			0.62f);
	}

	for (int32 Index = 0; Index < 12; ++Index)
	{
		const float X = -1000.0f + Index * 650.0f;
		const float Y = 5200.0f + ((Index % 2 == 0) ? -270.0f : 270.0f);
		SpawnPhysicsProp(
			TEXT("Sakura.Physics.Cone"),
			BasicConeMesh,
			RoundTransform(FVector(X, Y, ConeCenterZ), TrafficConeDiameter, TrafficConeHeight),
			TrafficConeMassKg,
			FLinearColor(1.0f, 0.23f, 0.015f),
			0.62f);
	}

	struct FMassComparisonProp
	{
		FVector Location;
		float MassInKg;
		FName Tag;
		FLinearColor Color;
		float Roughness;
	};

	const FMassComparisonProp MassProps[] =
	{
		{
			FVector(1600.0, 2850.0, 50.0),
			10.0f,
			TEXT("Sakura.Physics.Mass.Light"),
			FLinearColor(0.02f, 0.55f, 0.75f),
			0.48f,
		},
		{
			FVector(2400.0, 2850.0, 50.0),
			100.0f,
			TEXT("Sakura.Physics.Mass.Medium"),
			FLinearColor(0.9f, 0.78f, 0.16f),
			0.65f,
		},
		{
			FVector(3200.0, 2850.0, 50.0),
			1000.0f,
			TEXT("Sakura.Physics.Mass.Heavy"),
			FLinearColor(0.95f, 0.32f, 0.05f),
			0.55f,
		},
	};
	for (const FMassComparisonProp& Prop : MassProps)
	{
		SpawnPhysicsProp(
			Prop.Tag,
			BasicCubeMesh,
			BoxTransform(Prop.Location, FVector(100.0f)),
			Prop.MassInKg,
			Prop.Color,
			Prop.Roughness);
	}

	// Same geometry and mass, deliberately different physical restitution.
	// Driving into both isolates bounce response from visual shader roughness.
	SpawnPhysicsProp(
		TEXT("Sakura.Physics.Restitution.Low"),
		BasicSphereMesh,
		RoundTransform(FVector(1800.0f, 3400.0f, 50.0f), 100.0f, 100.0f),
		20.0f,
		FLinearColor(0.45f, 0.15f, 0.75f),
		0.45f,
		true,
		0.45f,
		0.02f);
	SpawnPhysicsProp(
		TEXT("Sakura.Physics.Restitution.High"),
		BasicSphereMesh,
		RoundTransform(FVector(3000.0f, 3400.0f, 50.0f), 100.0f, 100.0f),
		20.0f,
		FLinearColor(0.15f, 0.85f, 0.32f),
		0.45f,
		true,
		0.45f,
		0.88f);

	constexpr int32 WallColumns = 8;
	constexpr int32 WallRows = 4;
	constexpr float BlockWidth = 150.0f;
	constexpr float BlockDepth = 85.0f;
	constexpr float BlockHeight = 70.0f;
	constexpr float HorizontalGap = 5.0f;
	constexpr float VerticalGap = 0.0f;
	for (int32 Row = 0; Row < WallRows; ++Row)
	{
		for (int32 Column = 0; Column < WallColumns; ++Column)
		{
			const float X = 4700.0f
				+ (static_cast<float>(Column) - (WallColumns - 1) * 0.5f) * (BlockWidth + HorizontalGap);
			const float Z = BlockHeight * 0.5f + Row * (BlockHeight + VerticalGap);
			SpawnPhysicsProp(
				TEXT("Sakura.Physics.BlockWall"),
				BasicCubeMesh,
				BoxTransform(FVector(X, 2850.0, Z), FVector(BlockWidth, BlockDepth, BlockHeight)),
				8.0f,
				FLinearColor(0.08f, 0.38f, 0.58f),
				0.75f);
		}
	}

	if (GetWorld())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (const float X : {6650.0f, 7450.0f})
		{
			const FTransform RelativeTransform(
				FRotator(0.0f, -90.0f, 0.0f),
				FVector(X, 2850.0, 96.0f));
			ASakuraTestLabRagdollTarget* Target = GetWorld()->SpawnActor<ASakuraTestLabRagdollTarget>(
				ASakuraTestLabRagdollTarget::StaticClass(),
				RelativeTransform * GetActorTransform(),
				SpawnParameters);
			if (Target)
			{
				SpawnedShowcaseActors.Add(Target);
			}
		}
	}
}

void ASakuraTestLabEnvironment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep existing saved map instances aligned with the native lab contract.
	// TextRender default-subobject values can otherwise retain an older value
	// serialized when the original central map was generated.
	HubSign->SetText(FText::FromString(
		TEXT("SAKURA SYSTEMS TEST LAB\nON FOOT: F OR GAMEPAD X = ACTIVATE    M = CHARACTER / MOVER")));
	HybridGarageSign->SetText(FText::FromString(
		TEXT("EPIC STOCK CHAOS MODULAR VEHICLE\nF / GAMEPAD X: ACTIVATE")));
	StockGarageSign->SetText(FText::FromString(
		TEXT("STOCK KINETIFORGE RWD SPORTS CAR\nF / GAMEPAD X: ACTIVATE")));
	VehicleCourseSign->SetText(FText::FromString(
		TEXT("VEHICLE DYNAMICS CIRCUIT\nRT ACCELERATE    LT BRAKE / REVERSE    LS STEER    A HANDBRAKE")));
	MassComparisonSign->SetText(FText::FromString(
		TEXT("CHAOS MASS + RESTITUTION\nCUBES: 10 / 100 / 1000 KG\nSPHERES: LOW BOUNCE / HIGH BOUNCE")));
	BlockWallSign->SetText(FText::FromString(
		TEXT("CHAOS RIGID-BODY WALL\n32 INDEPENDENT 8 KG BLOCKS")));
	RagdollSign->SetText(FText::FromString(
		TEXT("PHYSICS-ASSET RAGDOLLS\nVEHICLE IMPACT ACTIVATES FULL-BODY CHAOS")));

	ApplyColor(GroundGeometry, GroundMaterial, FLinearColor(0.075f, 0.09f, 0.11f), 0.92f);
	ApplyColor(RoadGeometry, RoadMaterial, FLinearColor(0.055f, 0.06f, 0.07f), 0.97f);
	ApplyColor(RoadMarkerGeometry, RoadMarkerMaterial, FLinearColor(0.9f, 0.78f, 0.16f), 0.65f);
	ApplyColor(CharacterCourseGeometry, CharacterCourseMaterial, FLinearColor(0.08f, 0.38f, 0.58f), 0.75f);
	ApplyColor(SaveMarkerGeometry, SaveMarkerMaterial, FLinearColor(0.12f, 0.65f, 0.28f), 0.55f);
	ApplyColor(HybridGarageGeometry, HybridGarageMaterial, FLinearColor(0.02f, 0.55f, 0.75f), 0.48f);
	ApplyColor(StockGarageGeometry, StockGarageMaterial, FLinearColor(0.95f, 0.32f, 0.05f), 0.55f);
	ApplyColor(CourseObstacleGeometry, CourseObstacleMaterial, FLinearColor(0.38f, 0.22f, 0.08f), 0.88f);
	ApplyColor(CourseConeGeometry, CourseConeMaterial, FLinearColor(1.0f, 0.23f, 0.015f), 0.62f);
}

void ASakuraTestLabEnvironment::ApplyColor(
	UInstancedStaticMeshComponent* Component,
	TObjectPtr<UMaterialInstanceDynamic>& MaterialSlot,
	const FLinearColor& Color,
	const float Roughness)
{
	if (!Component || !BasicShapeMaterial)
	{
		return;
	}

	MaterialSlot = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
	if (!MaterialSlot)
	{
		return;
	}

	MaterialSlot->SetVectorParameterValue(TEXT("Color"), Color);
	MaterialSlot->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	Component->SetMaterial(0, MaterialSlot);
}
