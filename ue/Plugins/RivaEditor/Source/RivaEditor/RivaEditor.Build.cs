using UnrealBuildTool;

public class RivaEditor : ModuleRules
{
    public RivaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "RivaCore",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "UnrealEd",
                "ToolMenus",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "AppFramework",
                "TraceServices",
                "TraceLog",
                "TraceAnalysis",
                "DesktopPlatform"
            }
        );
    }
}
