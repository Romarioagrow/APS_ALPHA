// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class APS_ALPHA : ModuleRules
{
	public APS_ALPHA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG"
			, "WorldScapeCore"
			, "WorldScapeCommon"
			, "WorldScapeNoise"
			, "WorldScapeVolume"
			, "WorldScapeFoliages"
			, "ProceduralMeshComponent" 
			, "AtmoScape"
		});

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("WorldScapeEditor");
		}
		
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "DirGravity", "EnhancedInput" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}