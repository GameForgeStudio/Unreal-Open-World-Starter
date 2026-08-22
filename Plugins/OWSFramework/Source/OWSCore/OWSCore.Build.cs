using UnrealBuildTool;

public class OWSCore : ModuleRules
{
	public OWSCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"InputCore",
			"Slate",
			"SlateCore"
		});
	}
}
