using UnrealBuildTool;

public class SakuraUnrealPrototypeEditor : ModuleRules
{
	public SakuraUnrealPrototypeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SakuraUnrealPrototype"
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
