using UnrealBuildTool;

public class FindingValentinoTarget : TargetRules
{
	public FindingValentinoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FindingValentino");
	}
}
