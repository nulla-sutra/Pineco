// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PinecoTraceExecutionTypes.generated.h"

class UPinecoTraceRule;

UENUM(BlueprintType)
enum class EPinecoTraceExecutionState : uint8
{
    Inactive,
    Active,
    Completed,
    Cancelled,
    Failed
};

UENUM(BlueprintType)
enum class EPinecoTraceError : uint8
{
    None,
    InvalidContext,
    InvalidConfiguration,
    SourceUnavailable,
    InvalidInterval,
    Discontinuous,
    EvaluationFailed
};

UENUM(BlueprintType)
enum class EPinecoTraceHitPolicy : uint8
{
    /** Preserve every accepted candidate, including repeats within and between evaluations. */
    AllResults,
    /** First accepted candidate wins; the provider owns any required body/target priority ordering. */
    OncePerActor,
    /** Skeletal bodies use Component + BoneName; missing bone identity falls back to the mesh. */
    OncePerBody,
    /** Once per actor in this evaluation; permits the next scheduled area pulse to hit again. */
    OncePerActorPerEvaluation
};

USTRUCT(BlueprintType)
struct PINECOTRACE_API FPinecoTraceExecutionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pineco|Trace")
    EPinecoTraceHitPolicy HitPolicy = EPinecoTraceHitPolicy::OncePerActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pineco|Trace")
    bool bIgnoreSourceActor = true;

    /** All predicates must accept; each execution creates its own instances. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pineco|Trace")
    TArray<TSubclassOf<UPinecoTraceRule>> RuleClasses;
};
