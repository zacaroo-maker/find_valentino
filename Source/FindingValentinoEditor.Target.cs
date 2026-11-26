using UnrealBuildTool;

public class FindingValentinoEditorTarget : TargetRules
{
	public FindingValentinoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FindingValentino");
	}
}
