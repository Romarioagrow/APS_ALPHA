// APS_ALPHAEditorTarget.cs
using UnrealBuildTool;
using System.Collections.Generic;

public class APS_ALPHAEditorTarget : TargetRules
{
	public APS_ALPHAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;              // было V2
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;   // было Unreal5_3
		ExtraModuleNames.Add("APS_ALPHA");
	}
}