// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)


#include "VehicleAeroBaseComponent.h"
#include "VehicleWheelCoordinatorComponent.h"
#include "VehicleUtilities.h"

// Sets default values for this component's properties
UVehicleAeroBaseComponent::UVehicleAeroBaseComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UVehicleAeroBaseComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	Chassis = UVehicleUtilities::FindPhysicalParent(this);
	if (UPrimitiveComponent* ChassisRaw = Chassis.Get())
	{
		this->AttachToComponent(ChassisRaw, FAttachmentTransformRules::KeepWorldTransform);

		WheelCoordinator = UVehicleWheelCoordinatorComponent::FindWheelCoordinator(Chassis.Get());
		if (UVehicleWheelCoordinatorComponent* CoordinatorRaw = WheelCoordinator.Get())
		{
			CoordinatorRaw->RegisterAero(this);
		}
	}
	
	LastVirtualPartLocalTransform = VirtualPartLocalTransform;
}

void UVehicleAeroBaseComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (UVehicleWheelCoordinatorComponent* CoordinatorRaw = WheelCoordinator.Get())
	{
		CoordinatorRaw->UnregisterAero(this);
		WheelCoordinator = nullptr;
	}
	Chassis = nullptr;

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}


// Called every frame
void UVehicleAeroBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UVehicleAeroBaseComponent::CalculateAerodynamics(
	const float PhysicsDeltaTime,
	const FVector& ParentCOMWorldLocation,
	const FVector& ParentWorldLinearVelocity,
	const FVector& ParentWorldAngularVelocity,
	const FTransform& ParentWorldTransform,
	const FVector& WindWorldVelocity,
	FVector& OutLinearForce,
	FVector& OutAngularTorque)
{
	// ==========================================
	// 1. 跨线程数据安全处理 (栈拷贝与防撕裂)
	// ==========================================
	// 在栈上复制一份会被 GameThread 修改的数据，并归一化四元数
	FTransform SafeVirtualPartLocalTransform = VirtualPartLocalTransform;
	SafeVirtualPartLocalTransform.NormalizeRotation();

	// 获取组件相对底盘的变换并归一化
	FTransform SafeRelativeTransform = GetRelativeTransform();
	SafeRelativeTransform.NormalizeRotation();

	// ==========================================
	// 2. 在物理线程中计算虚拟零件的局部速度差分
	// ==========================================
	FVector VirtualPartLocalLinearVelocity = FVector::ZeroVector;
	FVector VirtualPartLocalAngularVelocity = FVector::ZeroVector;

	if (bTeleportRequested)
	{
		// 如果游戏线程请求了瞬移，我们强制同步历史记录，并保持速度为 0
		LastVirtualPartLocalTransform = SafeVirtualPartLocalTransform;

		// 消费掉标记
		bTeleportRequested = false;
	}
	else if (PhysicsDeltaTime > KINDA_SMALL_NUMBER)
	{
		// 线速度差分
		VirtualPartLocalLinearVelocity = (SafeVirtualPartLocalTransform.GetLocation() - LastVirtualPartLocalTransform.GetLocation()) / PhysicsDeltaTime;

		// 角速度差分 (取最短路径)
		FQuat DeltaQuat = SafeVirtualPartLocalTransform.GetRotation() * LastVirtualPartLocalTransform.GetRotation().Inverse();
		DeltaQuat.EnforceShortestArcWith(FQuat::Identity);

		FVector Axis;
		float AngleRad;
		DeltaQuat.ToAxisAndAngle(Axis, AngleRad);
		VirtualPartLocalAngularVelocity = Axis * (AngleRad / PhysicsDeltaTime);
	}

	// 更新缓存供下一个物理步长使用
	LastVirtualPartLocalTransform = SafeVirtualPartLocalTransform;

	// ==========================================
	// 3. 坐标系合成与运动学解算
	// ==========================================
	// 气动组件的世界变换 = 组件相对变换 * 底盘世界变换
	FTransform CompWorldTransform = SafeRelativeTransform * ParentWorldTransform;

	// 虚拟零件的世界变换 = 虚拟零件相对变换 * 组件世界变换
	FTransform VirtualPartWorldTransform = SafeVirtualPartLocalTransform * CompWorldTransform;

	// 气动中心(CoP)的世界坐标
	FVector CoPWorldLoc = VirtualPartWorldTransform.TransformPosition(CenterOfPressureOffset);

	// 从底盘质心指向气动中心的力臂 (世界坐标系下)
	FVector RadiusFromChassis = CoPWorldLoc - ParentCOMWorldLocation;

	// 将局部的线速度和角速度转换到世界方向
	FVector ActiveLinearVelWorld = CompWorldTransform.TransformVectorNoScale(VirtualPartLocalLinearVelocity);
	FVector ActiveAngularVelWorld = CompWorldTransform.TransformVectorNoScale(VirtualPartLocalAngularVelocity);

	// 计算主动旋转带来的外缘线速度叠加: v = w x r
	FVector CoPOffsetWorld = VirtualPartWorldTransform.TransformVectorNoScale(CenterOfPressureOffset);
	FVector ActiveCoPVelWorld = ActiveLinearVelWorld + FVector::CrossProduct(ActiveAngularVelWorld, CoPOffsetWorld);

	// ==========================================
	// 4. 最终表观风速计算
	// ==========================================
	// 气动点绝对速度 = 底盘移动速度 + (底盘角速度 x 力臂) + 零件自身的相对绝对速度
	FVector PointVelocity = ParentWorldLinearVelocity +
		FVector::CrossProduct(ParentWorldAngularVelocity, RadiusFromChassis) +
		ActiveCoPVelWorld;

	// 表观风速
	FVector ApparentWindWorld = WindWorldVelocity - PointVelocity;

	// ==========================================
	// 5. 子类计算 
	// ==========================================
	FVector LocalForce = FVector::ZeroVector;
	FVector LocalTorque = FVector::ZeroVector;

	CalculateCustomAeroForces(ApparentWindWorld, ParentWorldAngularVelocity, VirtualPartWorldTransform, LocalForce, LocalTorque);

	OutLinearForce = LocalForce;
	OutAngularTorque = LocalTorque + FVector::CrossProduct(RadiusFromChassis, LocalForce);
}

void UVehicleAeroBaseComponent::SetVirtualPartLocalTransform(const FTransform& NewTransform, const bool bTeleport)
{
	VirtualPartLocalTransform = NewTransform;
	bTeleportRequested = bTeleport;	
}

