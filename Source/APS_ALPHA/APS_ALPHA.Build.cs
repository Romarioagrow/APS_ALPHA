// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class APS_ALPHA : ModuleRules
{
	public APS_ALPHA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("Chaos"); // Opt-in fitted-hull spatial sweeps.
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "PhysicsCore", "InputCore", "UMG", "ModelViewViewModel"
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
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools", "MaterialEditor" });
			// Editor-only asynchronous stellar diagnostic readback; no runtime rendering change.
			PrivateDependencyModuleNames.Add("RHI");
		}
		
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "DirGravity", "EnhancedInput", "AssetRegistry", "RenderCore" });
		// Rio 09.10 (authored worlds, Core/Worlds/APSAuthoredWorlds): presets of the generator as JSON files under Content.
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
		// Rio 03.10 (galaxy phase 3): GPU star points and galaxy glow (Plugins/APSStarRenderer, enabled in the .uproject).
		PrivateDependencyModuleNames.Add("APSStarRenderer");

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
