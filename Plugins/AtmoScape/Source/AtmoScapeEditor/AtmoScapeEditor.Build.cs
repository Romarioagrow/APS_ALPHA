// Copyright (c) 2021 - BenjaCorp at iolaCorp

using UnrealBuildTool;
using UnrealBuildTool.Rules;

public class AtmoScapeEditor : ModuleRules
{
	public AtmoScapeEditor(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

    PrivateDependencyModuleNames.AddRange(new string[] {
      "Core",
      "CoreUObject",
      "InputCore",
      "Engine",
      "Slate",
      "SlateCore",
      "Projects",
      "Blutility",
      "EditorScriptingUtilities",
      "UMG",
      "UnrealEd"
    });
    
  }
}
