// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OWSTestLabEnvironment.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UTextRenderComponent;
class AOWSTestLabPhysicsProp;

/**
 * Asset-light, editor-visible geometry for the OWS integration test lab.
 *
 * Every mesh comes from /Engine/BasicShapes. The actor deliberately keeps the
 * course as native default subobjects so a placed instance is visible both in
 * the editor and during PIE without generated project content.
 */
UCLASS(Blueprintable)
class OWS_API AOWSTestLabEnvironment final : public AActor
{
	GENERATED_BODY()

public:
	AOWSTestLabEnvironment();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AOWSTestLabPhysicsProp* SpawnPhysicsProp(
		const FName& ShowcaseTag,
		UStaticMesh* Mesh,
		const FTransform& RelativeTransform,
		float MassInKg,
		const FLinearColor& Color,
		float Roughness,
		bool bUseContinuousCollisionDetection = true,
		float PhysicalFriction = 0.7f,
		float Restitution = 0.05f);

	void SpawnRuntimeShowcase();

	void ApplyColor(
		UInstancedStaticMeshComponent* Component,
		TObjectPtr<UMaterialInstanceDynamic>& MaterialSlot,
		const FLinearColor& Color,
		float Roughness);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> GroundGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> RoadGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> RoadMarkerGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> CharacterCourseGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> SaveMarkerGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> HybridGarageGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> StockGarageGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> CourseObstacleGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Geometry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> CourseConeGeometry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> HubSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> InventorySign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> SaveSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> CharacterCourseSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> HybridGarageSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> StockGarageSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> VehicleCourseSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> MassComparisonSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> BlockWallSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWS|Test Lab|Signs", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> RagdollSign;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BasicCubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BasicCylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BasicConeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BasicSphereMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedShowcaseActors;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RoadMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RoadMarkerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CharacterCourseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SaveMarkerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HybridGarageMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StockGarageMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CourseObstacleMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CourseConeMaterial;
};
