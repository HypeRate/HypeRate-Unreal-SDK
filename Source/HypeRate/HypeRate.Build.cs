using UnrealBuildTool;

public class HypeRate : ModuleRules
{
    public HypeRate(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "HTTP",
            "Json",
            "Slate",
            "SlateCore",
            "WebSockets"
        });
    }
}

