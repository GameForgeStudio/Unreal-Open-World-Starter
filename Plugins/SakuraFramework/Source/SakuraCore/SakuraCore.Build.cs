using UnrealBuildTool;

public class SakuraCore : ModuleRules
{
	public SakuraCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"SaveExtension",
			"SigilInventory",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"InputCore",
			"NetCore",
			"Slate",
			"SlateCore"
		});
	}
}
