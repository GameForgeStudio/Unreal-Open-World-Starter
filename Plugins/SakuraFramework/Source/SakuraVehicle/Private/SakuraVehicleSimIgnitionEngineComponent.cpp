// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

#include "SakuraVehicleSimIgnitionEngineComponent.h"

#include "SimModule/EngineModule.h"
#include "SimModule/ModuleInput.h"
#include "SimModule/SimModuleTree.h"
#include "VehicleUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SakuraVehicleSimIgnitionEngineComponent)

namespace Sakura::Vehicle::Ignition
{
	const FName ControlName(TEXT("Ignition"));

	/**
	 * Deliberately does not register a new Chaos simulation type. Keeping the
	 * inherited FEngineSimModule type means Epic's existing rewind state,
	 * factory, and FEngineOutputData remain valid on every peer.
	 */
	class FIgnitionEngineSimModule final : public Chaos::FEngineSimModule
	{
	public:
		explicit FIgnitionEngineSimModule(const Chaos::FEngineSettings& Settings)
			: FEngineSimModule(Settings)
		{
			EngineStarted = false;
			SetRPM(0.0f);
		}

		virtual void Simulate(
			const float DeltaTime,
			const Chaos::FAllInputs& Inputs,
			Chaos::FSimModuleTree& VehicleModuleSystem) override
		{
			EngineStarted = Inputs.GetControls().GetBool(ControlName);
			if (!EngineStarted)
			{
				// Clear the torque path before stopping the crank. TransmitTorque
				// pushes zero into the immediate child, whose normal simulation then
				// propagates it through clutch/transmission/wheels in this substep.
				SetDriveTorque(0.0f);
				SetLoadTorque(0.0f);
				SetBrakingTorque(0.0f);
				TransmitTorque(VehicleModuleSystem, 0.0f, 0.0f);
				SetDriveTorque(0.0f);
				SetLoadTorque(0.0f);
				SetBrakingTorque(0.0f);
				SetAngularVelocity(0.0f);
				SetAngularPosition(0.0f);
				return;
			}

			FEngineSimModule::Simulate(DeltaTime, Inputs, VehicleModuleSystem);
		}
	};
}

void USakuraVehicleSimIgnitionEngineComponent::OnOutputReady(
	const Chaos::FSimOutputData* OutputData)
{
	// UVehicleSimEngineComponent's UE 5.8 implementation is intentionally empty.
}

Chaos::ISimulationModuleBase*
USakuraVehicleSimIgnitionEngineComponent::CreateNewCoreModule() const
{
	Chaos::FEngineSettings Settings;
	Settings.MaxTorque = Chaos::TorqueMToCm(FMath::Max(MaxTorque, 0.0f));
	const int32 SafeMaxRPM = FMath::Clamp(MaxRPM, 0, 65535);
	Settings.MaxRPM = static_cast<uint16>(SafeMaxRPM);
	Settings.IdleRPM = static_cast<uint16>(FMath::Clamp(EngineIdleRPM, 0, SafeMaxRPM));
	Settings.EngineBrakeEffect = FMath::Max(EngineBrakeEffect, 0.0f);
	Settings.EngineInertia = FMath::Max(EngineInertia, SMALL_NUMBER);

	const FRichCurve* SourceCurve = TorqueCurve.GetRichCurveConst();
	float MinimumCurveValue = 0.0f;
	float MaximumCurveValue = 1.0f;
	if (SourceCurve != nullptr)
	{
		SourceCurve->GetValueRange(MinimumCurveValue, MaximumCurveValue);
	}
	const float CurveNormalization = FMath::Max(MaximumCurveValue, SMALL_NUMBER);
	constexpr int32 SampleCount = 20;
	for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float RPM = static_cast<float>(SafeMaxRPM) *
			static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const float CurveValue = SourceCurve != nullptr
			? SourceCurve->Eval(RPM) / CurveNormalization
			: 0.0f;
		Settings.TorqueCurve.AddNormalized(FMath::Max(CurveValue, 0.0f));
	}

	return new Sakura::Vehicle::Ignition::FIgnitionEngineSimModule(Settings);
}
