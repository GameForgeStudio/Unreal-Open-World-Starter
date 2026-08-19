// Copyright Aurora Angel Zeneva Zetanova. All Rights Reserved.

using UnrealBuildTool;

public class OWSVehicle : ModuleRules
{
	public OWSVehicle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"Chaos",
				"ChaosVehiclesCore",
				"ChaosModularVehicleEngine"
			}
		);

		SetupModulePhysicsSupport(Target);
		PrivateDefinitions.Add("CHAOS_INCLUDE_LEVEL_1=1");
	}
}
