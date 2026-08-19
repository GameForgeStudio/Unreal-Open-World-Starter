using UnrealBuildTool;

public class OWSFrameworkEditor : ModuleRules
{
	public OWSFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"OWSCore",
			"SigilInventory"
		});
	}
}
