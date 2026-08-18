// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraTestLabRagdollTarget.h"

#include "Animation/AnimationAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Net/UnrealNetwork.h"
#include "SakuraStockVehicleInteractionComponent.h"
#include "UObject/ConstructorHelpers.h"

ASakuraTestLabRagdollTarget::ASakuraTestLabRagdollTarget()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
	bReplicates = true;
	SetReplicateMovement(true);

	ImpactCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ImpactCapsule"));
	RootComponent = ImpactCapsule;
	ImpactCapsule->InitCapsuleSize(42.0f, 96.0f);
	ImpactCapsule->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	ImpactCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ImpactCapsule->SetCollisionObjectType(ECC_PhysicsBody);
	ImpactCapsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	ImpactCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ImpactCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	// A vehicle must not hit this kinematic standing capsule. We arm the real
	// skeletal ragdoll just before contact so the first blocking contact is
	// vehicle-versus-ragdoll rather than vehicle-versus-an-infinite-mass proxy.
	ImpactCapsule->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	ImpactCapsule->SetGenerateOverlapEvents(false);
	ImpactCapsule->SetNotifyRigidBodyCollision(true);
	ImpactCapsule->SetCanEverAffectNavigation(false);
	ImpactCapsule->SetMobility(EComponentMobility::Movable);
	// The standing target is a kinematic hit proxy, not a zero-gravity rigid body.
	// Simulating this capsule allowed an ordinary Character shove to tip the
	// entire actor sideways and leave it hovering indefinitely. A qualifying
	// vehicle/fast-body hit switches to the Manny physics asset below.
	ImpactCapsule->SetSimulatePhysics(false);
	ImpactCapsule->SetEnableGravity(true);
	ImpactCapsule->SetLinearDamping(1.25f);
	ImpactCapsule->SetAngularDamping(2.0f);
	ImpactCapsule->SetIsReplicated(true);

	TargetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TargetMesh"));
	TargetMesh->SetupAttachment(ImpactCapsule);
	TargetMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TargetMesh->SetGenerateOverlapEvents(false);
	TargetMesh->SetCanEverAffectNavigation(false);
	TargetMesh->SetIsReplicated(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		TargetMesh->SetSkeletalMeshAsset(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> IdleAnimationFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	if (IdleAnimationFinder.Succeeded())
	{
		TargetMesh->OverrideAnimationData(IdleAnimationFinder.Object, true, true);
	}

	ImpactCapsule->OnComponentHit.AddDynamic(this, &ASakuraTestLabRagdollTarget::HandleImpact);
	Tags.Add(TEXT("Sakura.Physics.RagdollTarget"));
}

void ASakuraTestLabRagdollTarget::BeginPlay()
{
	Super::BeginPlay();
}

void ASakuraTestLabRagdollTarget::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && !bRagdollActive)
	{
		ArmRagdollBeforeVehicleContact();
	}
}

void ASakuraTestLabRagdollTarget::ArmRagdollBeforeVehicleContact()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SakuraRagdollVehicleApproach), false, this);
	const FCollisionObjectQueryParams ObjectQuery(FCollisionObjectQueryParams::AllDynamicObjects);
	const FCollisionShape DetectionShape = FCollisionShape::MakeCapsule(135.0f, 125.0f);
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQuery,
		DetectionShape,
		QueryParams))
	{
		return;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		UPrimitiveComponent* OtherComponent = Overlap.GetComponent();
		if (!OtherActor || !OtherComponent
			|| !OtherActor->FindComponentByClass<USakuraStockVehicleInteractionComponent>())
		{
			continue;
		}

		const FVector VehicleVelocity = OtherComponent->GetComponentVelocity();
		if (VehicleVelocity.SizeSquared() < FMath::Square(25.0f))
		{
			continue;
		}

		bRagdollActive = true;
		ReplicatedImpactImpulse = FVector::ZeroVector;
		ReplicatedImpactPoint = TargetMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 80.0f);
		ForceNetUpdate();
		MulticastActivateRagdoll(ReplicatedImpactImpulse, ReplicatedImpactPoint);
		SetActorTickEnabled(false);
		return;
	}
}

void ASakuraTestLabRagdollTarget::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASakuraTestLabRagdollTarget, bRagdollActive);
	DOREPLIFETIME(ASakuraTestLabRagdollTarget, ReplicatedImpactImpulse);
	DOREPLIFETIME(ASakuraTestLabRagdollTarget, ReplicatedImpactPoint);
}

void ASakuraTestLabRagdollTarget::HandleImpact(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bRagdollActive || !HasAuthority() || !OtherActor || !OtherComponent)
	{
		return;
	}

	const FVector OtherVelocity = OtherComponent->GetComponentVelocity();
	const bool bVehicleImpact = OtherComponent->GetCollisionObjectType() == ECC_Vehicle;
	const bool bFastPhysicsImpact = OtherComponent->IsSimulatingPhysics()
		&& OtherVelocity.SizeSquared() >= FMath::Square(250.0f);
	if (!bVehicleImpact && !bFastPhysicsImpact)
	{
		return;
	}

	FVector RagdollImpulse = NormalImpulse;
	if (RagdollImpulse.IsNearlyZero())
	{
		RagdollImpulse = OtherVelocity.GetClampedToMaxSize(2500.0f) * 35.0f;
	}
	RagdollImpulse = RagdollImpulse.GetClampedToMaxSize(350000.0f);

	const FVector RawImpactPoint(Hit.ImpactPoint);
	const FVector ImpactPoint = RawImpactPoint.IsNearlyZero()
		? TargetMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 90.0f)
		: RawImpactPoint;
	bRagdollActive = true;
	ReplicatedImpactImpulse = RagdollImpulse;
	ReplicatedImpactPoint = ImpactPoint;
	ForceNetUpdate();
	MulticastActivateRagdoll(RagdollImpulse, ImpactPoint);
}

void ASakuraTestLabRagdollTarget::MulticastActivateRagdoll_Implementation(
	const FVector ImpactImpulse,
	const FVector ImpactPoint)
{
	ActivateRagdoll(ImpactImpulse, ImpactPoint);
}

void ASakuraTestLabRagdollTarget::OnRep_RagdollState()
{
	if (bRagdollActive)
	{
		ActivateRagdoll(ReplicatedImpactImpulse, ReplicatedImpactPoint);
	}
}

void ASakuraTestLabRagdollTarget::ActivateRagdoll(
	const FVector& ImpactImpulse,
	const FVector& ImpactPoint)
{
	if (bRagdollApplied)
	{
		return;
	}

	bRagdollApplied = true;
	SetActorTickEnabled(false);
	ImpactCapsule->SetSimulatePhysics(false);
	ImpactCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TargetMesh->Stop();
	TargetMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	TargetMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	TargetMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	TargetMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	TargetMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	TargetMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TargetMesh->SetSimulatePhysics(true);
	TargetMesh->SetAllBodiesSimulatePhysics(true);
	TargetMesh->SetAllBodiesPhysicsBlendWeight(1.0f);
	TargetMesh->WakeAllRigidBodies();
	TargetMesh->AddImpulseAtLocation(ImpactImpulse, ImpactPoint, TEXT("pelvis"));
}
