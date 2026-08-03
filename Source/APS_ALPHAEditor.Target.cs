// APS_ALPHAEditorTarget.cs
using UnrealBuildTool;
using System.Collections.Generic;

public class APS_ALPHAEditorTarget : TargetRules
{
	public APS_ALPHAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;       
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		bOverrideBuildEnvironment = true;
		bUndefinedIdentifierErrors = false;
		GlobalDefinitions.Add("__has_feature(x)=0");
		ExtraModuleNames.Add("APS_ALPHA");
	}
}
