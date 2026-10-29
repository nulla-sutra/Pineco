// Copyright 2019-Present tarnishablec. All Rights Reserved.

using UnrealBuildTool;

public class PinecoAnimation : ModuleRules
{
    public PinecoAnimation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.Default;
        bLegacyParentIncludePaths = true;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "PinecoTrace"
        });
    }
}
