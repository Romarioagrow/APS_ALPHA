// APS_ALPHATarget.cs
using UnrealBuildTool;
using System.Collections.Generic;

public class APS_ALPHATarget : TargetRules
{
	public APS_ALPHATarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;              
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;   
		bOverrideBuildEnvironment = true;
		bUndefinedIdentifierErrors = false;
		GlobalDefinitions.Add("__has_feature(x)=0");
		ExtraModuleNames.Add("APS_ALPHA");
	}
}
