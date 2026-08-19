// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)


#include "VehicleAeroVolumeComponent.h"

void UVehicleAeroVolumeComponent::SetConfig(const FVehicleAeroVolumeConfig& NewConfig)
{
	Config = NewConfig;
}

void UVehicleAeroVolumeComponent::CalculateCustomAeroForces(
	const FVector& ApparentWindWorld,
	const FVector& ChassisAngularVelWorld,
	const FTransform& CoPWorldTransform,
	FVector& OutAeroForce,
	FVector& OutAeroTorque)
{
	// ==========================================
	// 1. 单位转换 (UE -> SI)
	// ==========================================
	FVector ApparentWind_SI = ApparentWindWorld * 0.01f;

	// 角速度通常引擎里是 弧度/秒(rad/s) 或 度/秒(deg/s)。
	// 假设 ChassisAngularVelWorld 是 rad/s，无需转换。如果是 deg/s，请乘以 FMath::DegreesToRadians(1.0f)
	FVector AngularVel_SI = ChassisAngularVelWorld;

	// ==========================================
	// 2. 将风速和角速度转换到局部空间
	// ==========================================
	// 把世界风速转到盒子的局部空间。
	// 注意：ApparentWind 是“风吹向盒子”的速度向量
	FVector LocalWind_SI = CoPWorldTransform.InverseTransformVectorNoScale(ApparentWind_SI);
	FVector LocalAngularVel_SI = CoPWorldTransform.InverseTransformVectorNoScale(AngularVel_SI);

	// ==========================================
	// 3. 计算三轴独立平移阻力 (完全无分支算法)
	// ==========================================
	// 物理公式: F = 0.5 * rho * C_d * A * |V| * V
	// 使用 V * |V| 可以完美保留力的正负号方向，无需判断前进还是后退
	FVector WindSpeedSquaredSigned = FVector(
		LocalWind_SI.X * FMath::Abs(LocalWind_SI.X),
		LocalWind_SI.Y * FMath::Abs(LocalWind_SI.Y),
		LocalWind_SI.Z * FMath::Abs(LocalWind_SI.Z)
	);

	// 对应分量相乘: (0.5 * 密度) * (系数向量) * (面积向量) * (平方速度向量)
	FVector LocalForce_SI = 0.5f * AirDensity
		* Config.DragCoefficients
		* Config.FrontalAreas
		* WindSpeedSquaredSigned;

	// ==========================================
	// 4. 计算三轴旋转阻力 (抑制高速震荡的神器)
	// ==========================================
	// 旋转空气阻力同样遵循平方律：Torque = -0.5 * rho * C_rot * |w| * w
	// 注意这里的负号，因为阻尼力矩总是对抗当前的角速度
	FVector AngularVelSquaredSigned = FVector(
		LocalAngularVel_SI.X * FMath::Abs(LocalAngularVel_SI.X),
		LocalAngularVel_SI.Y * FMath::Abs(LocalAngularVel_SI.Y),
		LocalAngularVel_SI.Z * FMath::Abs(LocalAngularVel_SI.Z)
	);

	// 三维旋转风阻公式中，扭矩与长度的立方（$L^3$）成正比
	FVector LengthCubed = FVector(
		FMath::Pow(Config.ReferenceLengths.X, 3.0f),
		FMath::Pow(Config.ReferenceLengths.Y, 3.0f),
		FMath::Pow(Config.ReferenceLengths.Z, 3.0f)
	);

	FVector LocalTorque_SI = -0.5f * AirDensity
		* Config.AngularDragCoefficients // 现在填 1.0 即可
		* Config.FrontalAreas            // 乘以面积 (m^2)
		* LengthCubed                    // 乘以后缀力臂 (m^3)
		* AngularVelSquaredSigned;       // 乘以角速度平方 (1/s^2)

	// ==========================================
	// 5. 转换回世界空间并换算回 UE 单位
	// ==========================================
	FVector ForceWorld_SI = CoPWorldTransform.TransformVectorNoScale(LocalForce_SI);
	FVector TorqueWorld_SI = CoPWorldTransform.TransformVectorNoScale(LocalTorque_SI);

	// 输出：力(N)转回 kg*cm/s^2 乘 100
	OutAeroForce = ForceWorld_SI * 100.0f;

	// 输出：扭矩(N*m)转回 kg*cm^2/s^2 乘 10000
	OutAeroTorque = TorqueWorld_SI * 10000.0f;
}
