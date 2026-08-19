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
			"OWSCore",
			"OWSMovement"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Chaos",
			"ChaosModularVehicleEngine",
			"ChaosVehiclesCore",
			"EnhancedInput",
			"GameplayAbilities",
			"InputCore",
			"KinetiForge",
			"NetCore",
			"PhysicsCore",
			"SaveExtension",
			"SigilInventory"
		});

		SetupModulePhysicsSupport(Target);
		PrivateDefinitions.Add("CHAOS_INCLUDE_LEVEL_1=1");
	}
}
