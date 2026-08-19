using UnrealBuildTool;

public class OWSEditor : ModuleRules
{
	public OWSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"OWS"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Foliage",
			"Landscape",
			"RenderCore",
			"UnrealEd"
		});
	}
}
