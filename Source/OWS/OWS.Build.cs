using UnrealBuildTool;

public class OWS : ModuleRules
{
	public OWS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"OWSCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Chaos",
			"ChaosModularVehicleEngine",
			"ChaosVehiclesCore",
			"EnhancedInput",
			"KinetiForge",
			"NetCore",
			"PhysicsCore",
			"Slate",
			"SlateCore"
		});

		SetupModulePhysicsSupport(Target);
		PrivateDefinitions.Add("CHAOS_INCLUDE_LEVEL_1=1");

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
