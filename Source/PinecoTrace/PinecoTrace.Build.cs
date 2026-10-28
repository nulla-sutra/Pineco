// Copyright 2019-Present tarnishablec. All Rights Reserved.

using UnrealBuildTool;

public class PinecoTrace : ModuleRules
{
    public PinecoTrace(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.Default;
        bLegacyParentIncludePaths = true;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine"
        });
    }
}
