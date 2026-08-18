// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraTestLabPhysicsProp.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

ASakuraTestLabPhysicsProp::ASakuraTestLabPhysicsProp()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BasicShapeMaterial = MaterialFinder.Object;

	PropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMesh"));
	PropMesh->SetMobility(EComponentMobility::Movable);
	PropMesh->SetStaticMesh(DefaultMeshFinder.Object);
	PropMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	PropMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// Modular Vehicle suspension traces currently use ECC_WorldDynamic. Props
	// must ignore that trace without ceasing to collide with cars or players.
	PropMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	PropMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	PropMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	PropMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	PropMesh->SetGenerateOverlapEvents(false);
	PropMesh->SetCanEverAffectNavigation(false);
	PropMesh->SetUseCCD(true);
	PropMesh->SetLinearDamping(0.15f);
	PropMesh->SetAngularDamping(0.2f);
	PropMesh->SetSimulatePhysics(true);
	PropMesh->SetIsReplicated(true);
	RootComponent = PropMesh;
}

void ASakuraTestLabPhysicsProp::InitializeShowcaseProp(
	UStaticMesh* Mesh,
	const float MassInKg,
	const FLinearColor& Color,
	const float Roughness,
	const FName& ShowcaseTag,
	const bool bUseContinuousCollisionDetection,
	const float PhysicalFriction,
	const float Restitution)
{
	ShowcaseMesh = Mesh;
	ShowcaseMassInKg = FMath::Max(0.1f, MassInKg);
	ShowcaseColor = Color;
	ShowcaseRoughness = FMath::Clamp(Roughness, 0.0f, 1.0f);
	ShowcasePhysicalFriction = FMath::Max(PhysicalFriction, 0.0f);
	ShowcaseRestitution = FMath::Clamp(Restitution, 0.0f, 1.0f);
	PropMesh->SetUseCCD(bUseContinuousCollisionDetection);
	Tags.AddUnique(ShowcaseTag);
	ApplyShowcaseConfiguration();
	PropMesh->PutRigidBodyToSleep();
	ForceNetUpdate();
}

void ASakuraTestLabPhysicsProp::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcaseMesh);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcaseMassInKg);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcaseColor);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcaseRoughness);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcasePhysicalFriction);
	DOREPLIFETIME(ASakuraTestLabPhysicsProp, ShowcaseRestitution);
}

void ASakuraTestLabPhysicsProp::OnRep_ShowcaseConfiguration()
{
	ApplyShowcaseConfiguration();
}

void ASakuraTestLabPhysicsProp::ApplyShowcaseConfiguration()
{
	if (ShowcaseMesh)
	{
		PropMesh->SetStaticMesh(ShowcaseMesh);
	}
	PropMesh->SetMassOverrideInKg(NAME_None, ShowcaseMassInKg, true);
	if (!DynamicPhysicalMaterial)
	{
		DynamicPhysicalMaterial = NewObject<UPhysicalMaterial>(this);
	}
	if (DynamicPhysicalMaterial)
	{
		DynamicPhysicalMaterial->Friction = ShowcasePhysicalFriction;
		DynamicPhysicalMaterial->Restitution = ShowcaseRestitution;
		DynamicPhysicalMaterial->bOverrideFrictionCombineMode = true;
		DynamicPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
		DynamicPhysicalMaterial->bOverrideRestitutionCombineMode = true;
		DynamicPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
		PropMesh->SetPhysMaterialOverride(DynamicPhysicalMaterial);
	}
	if (BasicShapeMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ShowcaseColor);
			DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), ShowcaseRoughness);
			PropMesh->SetMaterial(0, DynamicMaterial);
		}
	}
}
