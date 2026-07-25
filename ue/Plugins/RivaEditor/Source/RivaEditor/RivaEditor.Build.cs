using UnrealBuildTool;
using System.IO;

public class RivaEditor : ModuleRules
{
    public RivaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Private"),
                Path.GetFullPath(Path.Combine(PluginDirectory, "../../../include"))
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "UnrealEd",
                "ToolMenus",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "AppFramework"
            }
        );
    }
}
