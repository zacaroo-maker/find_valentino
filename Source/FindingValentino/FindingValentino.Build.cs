using UnrealBuildTool;

public class FindingValentino : ModuleRules
{
	public FindingValentino(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Compile each .cpp on its own: several files share helper names in anonymous namespaces,
		// which clash when Unreal merges files together ("unity build") for packaged builds.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"AIModule", "NavigationSystem", "GameplayTasks"
		});
	}
}
