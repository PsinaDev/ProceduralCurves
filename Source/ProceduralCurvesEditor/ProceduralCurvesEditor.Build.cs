using UnrealBuildTool;

public class ProceduralCurvesEditor : ModuleRules
{
	public ProceduralCurvesEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"Slate",
				"SlateCore",
				"PropertyEditor",
				"ProceduralCurves",
			}
		);
	}
}
