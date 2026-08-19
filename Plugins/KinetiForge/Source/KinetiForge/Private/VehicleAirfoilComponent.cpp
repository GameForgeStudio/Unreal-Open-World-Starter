// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)


#include "VehicleAirfoilComponent.h"

// Sets default values for this component's properties
UVehicleAirfoilComponent::UVehicleAirfoilComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UVehicleAirfoilComponent::SetConfig(const FVehicleAirfoilConfig& NewConfig)
{
	Config = NewConfig;
	// TODO: Bake LUTs
}

void UVehicleAirfoilComponent::CalculateCustomAeroForces(
	const FVector& ApparentWindWorld,
	const FVector& ChassisAngularVelWorld,
	const FTransform& CoPWorldTransform,
	FVector& OutAeroForce,
	FVector& OutAeroTorque)
{
	// 1. 滤除微小风速，避免归一化除零和无效计算
	// 1 cm/s = 0.01 m/s
	FVector ApparentWind_SI = ApparentWindWorld * 0.01f;

	float SpeedSq_SI = ApparentWind_SI.SizeSquared();
	if (SpeedSq_SI < KINDA_SMALL_NUMBER)
	{
		OutAeroForce = FVector::ZeroVector;
		OutAeroTorque = FVector::ZeroVector;
		return;
	}

	// 2. 将世界风速转换到气动中心的局部坐标系
	FVector LocalWind = CoPWorldTransform.InverseTransformVectorNoScale(ApparentWindWorld);

	// 3. 计算连续迎角 (AoA)
	// 设定：局部 X 为前，Z 为上。风从正前方吹来时，LocalWind 为 (-v, 0, 0)。
	// 为了让正迎角对应机翼上仰，传入 (-Z, -X) 到 Atan2。
	// FMath::Atan2 返回值范围是 [-PI, PI]，无缝且连续。
	float AoA_Rad = FMath::Atan2(-LocalWind.Z, -LocalWind.X);
	float AoA_Deg = FMath::RadiansToDegrees(AoA_Rad);

	// 4. 读取基础气动系数
	const FRichCurve* RawLiftCurve = Config.LiftCurve.GetRichCurveConst();
	const FRichCurve* RawDragCurve = Config.DragCurve.GetRichCurveConst();

	float CL = RawLiftCurve ? RawLiftCurve->Eval(AoA_Deg) : 0.0f;
	float CD_Parasitic = RawDragCurve ? RawDragCurve->Eval(AoA_Deg) : 0.0f;

	// 5. 计算诱导阻力 (翼尖涡流)
	// 展弦比 AR = b^2 / S
	float AspectRatio = (Config.Span * Config.Span) / Config.WingArea;
	// 防止除零保护
	float Denominator = PI * Config.OswaldEfficiency * AspectRatio;
	float CD_Induced = (Denominator > KINDA_SMALL_NUMBER) ? ((CL * CL) / Denominator) : 0.0f;

	// 总阻力系数
	float CD_Total = CD_Parasitic + CD_Induced;

	// 6. 计算动压 (Dynamic Pressure)
	// q = 0.5 * rho * v^2 * Area
	float DynamicPressureArea_SI = 0.5f * AirDensity * SpeedSq_SI * Config.WingArea;

	// 标量力大小。单位是标准的 牛顿 (N)
	float LiftMag_SI = DynamicPressureArea_SI * CL;
	float DragMag_SI = DynamicPressureArea_SI * CD_Total;

	// 7. 世界空间向量合成 (极致鲁棒性)
	FVector WindDirWorld = ApparentWindWorld.GetSafeNormal();

	// 机翼的局部 Y 轴 (右向) 在世界空间的方向
	FVector WingRightWorld = CoPWorldTransform.GetRotation().GetAxisY();

	// 升力方向必须同时垂直于“风向”和“机翼右向”
	// 叉乘 (右 x 风) 产生标准的向上升力方向。
	// 如果遇到极端纯侧风 (WindDir 和 WingRight 平行)，叉乘自动趋近于 0，升力平滑消失。
	FVector LiftDirWorld = FVector::CrossProduct(WingRightWorld, WindDirWorld).GetSafeNormal();

	// 8. 输出合力 (阻力永远顺着风，升力永远垂直于风)
	FVector AeroForce_SI = (LiftDirWorld * LiftMag_SI) + (WindDirWorld * DragMag_SI);
	OutAeroForce = AeroForce_SI * 100.0f;
	OutAeroTorque = FVector::ZeroVector; // 空力扭矩由 AeroBase 利用力臂计算，这里直接返回零
}