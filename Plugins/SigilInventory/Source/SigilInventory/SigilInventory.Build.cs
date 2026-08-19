// Copyright (c) 2026 Likeon. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SigilInventory : ModuleRules
{
	public SigilInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		bEnableNonInlinedGenCppWarnings = true;

		PublicIncludePaths.AddRange(
			new[]
			{
				Path.Combine(ModuleDirectory, "Public/Core"),
				Path.Combine(ModuleDirectory, "Public/Core/Items"),
				Path.Combine(ModuleDirectory, "Public/Core/Attributes"),
				Path.Combine(ModuleDirectory, "Public/Core/Collections"),
				Path.Combine(ModuleDirectory, "Public/Core/Fragments"),
				Path.Combine(ModuleDirectory, "Public/Equipping"),
				Path.Combine(ModuleDirectory, "Public/Crafting"),
				Path.Combine(ModuleDirectory, "Public/Exchange"),
				Path.Combine(ModuleDirectory, "Public/Serialization")
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"GameplayTags",
				"DeveloperSettings",
				"ModularGameplay"
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"SlateCore",
				"NetCore",
				"AssetRegistry"
				// ... add private dependencies that you statically link with here ...	
			}
		);

		SetupIrisSupport(Target);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}