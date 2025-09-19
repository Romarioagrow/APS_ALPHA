// APS_ALPHATarget.cs
using UnrealBuildTool;
using System.Collections.Generic;

public class APS_ALPHATarget : TargetRules
{
	public APS_ALPHATarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;              // ���� V2
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;   // ���� Unreal5_3
		ExtraModuleNames.Add("APS_ALPHA");
	}
}