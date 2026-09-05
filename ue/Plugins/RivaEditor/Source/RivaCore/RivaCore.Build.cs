using UnrealBuildTool;
using System.IO;

public class RivaCore : ModuleRules
{
    public RivaCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseRTTI = true;
        bEnableExceptions = true;
        CppStandard = CppStandardVersion.Cpp20;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Public")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Private")
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
            }
        );
    }
}
