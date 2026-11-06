using UnrealBuildTool;

public class HypeRateEditor : ModuleRules
{
    public HypeRateEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "HypeRate",
            "LevelEditor",
            "Settings",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd"
        });
    }
}

