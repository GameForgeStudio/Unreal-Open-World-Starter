// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "SakuraTestLabRagdollTarget.generated.h"

class UCapsuleComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;

/**
 * Standing impact target that switches from a simple blocking capsule to the
 * Manny physics asset when struck by a moving vehicle or other fast body.
 */
UCLASS(NotBlueprintable)
class SAKURAUNREALPROTOTYPE_API ASakuraTestLabRagdollTarget final : public AActor
{
	GENERATED_BODY()

public:
	ASakuraTestLabRagdollTarget();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleImpact(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivateRagdoll(FVector ImpactImpulse, FVector ImpactPoint);

	UFUNCTION()
	void OnRep_RagdollState();

	void ActivateRagdoll(const FVector& ImpactImpulse, const FVector& ImpactPoint);
	void ArmRagdollBeforeVehicleContact();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Test Lab|Ragdoll", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ImpactCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sakura|Test Lab|Ragdoll", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> TargetMesh;

	UPROPERTY(ReplicatedUsing = OnRep_RagdollState)
	bool bRagdollActive = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 ReplicatedImpactImpulse = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 ReplicatedImpactPoint = FVector::ZeroVector;

	bool bRagdollApplied = false;
};
