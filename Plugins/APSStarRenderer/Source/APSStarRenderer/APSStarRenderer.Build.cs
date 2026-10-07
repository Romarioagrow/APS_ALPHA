// Rio 03.10: galaxy phase 3 (GPU star points + galaxy glow). Plain C++, no UHT types.

using UnrealBuildTool;

public class APSStarRenderer : ModuleRules
{
	public APSStarRenderer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Projects",
			"RenderCore",
			"Renderer",
			"RHI"
		});
	}
}
