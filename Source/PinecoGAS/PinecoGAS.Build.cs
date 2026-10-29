// Copyright 2019-Present tarnishablec. All Rights Reserved.

using UnrealBuildTool;

public class PinecoGAS : ModuleRules
{
    public PinecoGAS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.Default;
        bLegacyParentIncludePaths = true;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTasks", "PinecoTrace"
        });
    }
}
