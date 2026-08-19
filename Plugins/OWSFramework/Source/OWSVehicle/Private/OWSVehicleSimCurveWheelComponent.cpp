// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.
// Curve sampling behavior is adapted from KinetiForge Vehicles by Zhengyi Miao under the MIT License.

#include "OWSVehicleSimCurveWheelComponent.h"

#include "Curves/CurveFloat.h"
#include "OWSVehicleAlgorithms.h"
#include "SimModule/WheelModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWSVehicleSimCurveWheelComponent)

UOWSVehicleSimCurveWheelComponent::UOWSVehicleSimCurveWheelComponent()
{
	// Asset-light default: a linear normalized grip curve. With both tuning
	// multipliers at 1, this matches the stock wheel's linear force at the
	// configured normalized-slip angle, then gives designers an inline curve.
	FRichCurve* DefaultCurve = NormalizedLateralSlipCurve.GetRichCurve();
	if (DefaultCurve != nullptr && DefaultCurve->IsEmpty())
	{
		const FKeyHandle StartKey = DefaultCurve->AddKey(0.0f, 0.0f);
		const FKeyHandle EndKey = DefaultCurve->AddKey(1.0f, 1.0f);
		DefaultCurve->SetKeyInterpMode(StartKey, RCIM_Linear);
		DefaultCurve->SetKeyInterpMode(EndKey, RCIM_Linear);
	}
}

void UOWSVehicleSimCurveWheelComponent::OnOutputReady(
	const Chaos::FSimOutputData* OutputData)
{
	if (OutputData == nullptr)
	{
		return;
	}

	const Chaos::FWheelOutputData* WheelOutput =
		static_cast<const Chaos::FWheelOutputData*>(OutputData);

	for (const Chaos::FWheelTouchChangeEvent& Event : WheelOutput->WheelTouchEvents)
	{
		OnWheelTouchChangeNativeEvent.Broadcast(WheelOutput->ModuleGuid, Event.bIsInContact);
	}
}

Chaos::ISimulationModuleBase* UOWSVehicleSimCurveWheelComponent::CreateNewCoreModule() const
{
	Chaos::ISimulationModuleBase* CoreModule = Super::CreateNewCoreModule();
	if (CoreModule == nullptr)
	{
		return CoreModule;
	}

	if (!CoreModule->IsSimType<Chaos::FWheelSimModule>())
	{
		return CoreModule;
	}

	if (!bEnableLateralSlipCurve)
	{
		return CoreModule;
	}

	const FRichCurve* SourceCurve = NormalizedLateralSlipCurve.GetRichCurveConst();
	if (SourceCurve == nullptr || SourceCurve->IsEmpty())
	{
		return CoreModule;
	}

	Chaos::FWheelSimModule* WheelModule = static_cast<Chaos::FWheelSimModule*>(CoreModule);
	Chaos::FWheelSettings& WheelSettings = WheelModule->AccessSetup();
	const float StockForceAtNormalizedSlip =
		FMath::DegreesToRadians(FMath::Max(NormalizedSlipDegrees, 0.0f))
		* WheelSettings.CorneringStiffness;
	const float EffectiveForceScale =
		StockForceAtNormalizedSlip * FMath::Max(LateralForceScale, 0.0f);

	TArray<FVector2f> Samples;
	OWS::Vehicle::BuildSymmetricLateralSlipSamples(
		*SourceCurve,
		NormalizedSlipDegrees,
		EffectiveForceScale,
		Samples);

	if (Samples.IsEmpty())
	{
		return CoreModule;
	}

	WheelSettings.LateralSlipGraph.Empty();

	for (const FVector2f& Sample : Samples)
	{
		WheelSettings.LateralSlipGraph.Add(Chaos::FVec2(Sample.X, Sample.Y));
	}

	WheelSettings.LateralSlipGraphMultiplier = FMath::Max(LateralSlipGraphMultiplier, 0.0f);
	return CoreModule;
}
