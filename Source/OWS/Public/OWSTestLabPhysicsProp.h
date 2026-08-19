// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OWSTestLabPhysicsProp.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPhysicalMaterial;
class UStaticMesh;
class UStaticMeshComponent;

/** Replicated, individually simulated rigid body used by the test-lab stations. */
UCLASS(NotBlueprintable)
class OWS_API AOWSTestLabPhysicsProp final : public AActor
{
	GENERATED_BODY()

public:
	AOWSTestLabPhysicsProp();

	void InitializeShowcaseProp(
		UStaticMesh* Mesh,
		float MassInKg,
		const FLinearColor& Color,
		float Roughness,
		const FName& ShowcaseTag,
		bool bUseContinuousCollisionDetection,
		float PhysicalFriction = 0.7f,
		float Restitution = 0.05f);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_ShowcaseConfiguration();

	void ApplyShowcaseConfiguration();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Physics", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PropMesh;

	/** Asset identity is replicated so cones remain cones on remote clients. */
	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	TObjectPtr<UStaticMesh> ShowcaseMesh;

	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	float ShowcaseMassInKg = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	FLinearColor ShowcaseColor = FLinearColor::White;

	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	float ShowcaseRoughness = 0.8f;

	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	float ShowcasePhysicalFriction = 0.7f;

	UPROPERTY(ReplicatedUsing = OnRep_ShowcaseConfiguration)
	float ShowcaseRestitution = 0.05f;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> DynamicPhysicalMaterial;
};
