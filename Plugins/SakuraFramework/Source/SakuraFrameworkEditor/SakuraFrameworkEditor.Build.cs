using UnrealBuildTool;

public class SakuraFrameworkEditor : ModuleRules
{
	public SakuraFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"SakuraCore",
			"SigilInventory"
		});
	}
}
