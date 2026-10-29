// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "PinecoTrace/Execution/PinecoTraceExecution.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "PinecoTrace/Rules/PinecoTraceRule.h"
#include "PinecoTrace/Trace/PinecoTrace.h"
#include "UObject/Class.h"

UPinecoTraceExecution* UPinecoTraceExecution::Create(
    UObject* WorldContextObject, TSubclassOf<UPinecoTrace> TraceClass, AActor* InSourceActor,
    const FPinecoTraceExecutionSettings& Settings)
{
    if (!IsInGameThread() || !IsValid(WorldContextObject) || !GEngine
        || !IsValid(TraceClass.Get())
        || TraceClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        return nullptr;
    }

    auto* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (!IsValid(World) || World->bIsTearingDown
        || (InSourceActor && (!IsValid(InSourceActor) || InSourceActor->GetWorld() != World)))
    {
        return nullptr;
    }

    switch (Settings.HitPolicy)
    {
    case EPinecoTraceHitPolicy::AllResults:
    case EPinecoTraceHitPolicy::OncePerActor:
    case EPinecoTraceHitPolicy::OncePerActorPerEvaluation:
    case EPinecoTraceHitPolicy::OncePerBody:
        break;
    default:
        return nullptr;
    }

    for (const auto& RuleClass : Settings.RuleClasses)
    {
        if (!IsValid(RuleClass.Get())
            || RuleClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            return nullptr;
        }
    }

    auto* Execution = NewObject<UPinecoTraceExecution>(WorldContextObject, NAME_None, RF_Transient);
    Execution->ExecutionWorld = World;
    Execution->SourceActor = InSourceActor;
    Execution->bRequiresSourceActor = InSourceActor != nullptr;
    Execution->HitPolicy = Settings.HitPolicy;
    Execution->bIgnoreSourceActor = Settings.bIgnoreSourceActor;
    Execution->Trace = NewObject<UPinecoTrace>(Execution, TraceClass, NAME_None, RF_Transient);
    Execution->bRequiresContinuousSampling = Execution->Trace->bRequiresContinuousSampling;
    Execution->Rules.Reserve(Settings.RuleClasses.Num());
    for (const auto& RuleClass : Settings.RuleClasses)
    {
        Execution->Rules.Add(NewObject<UPinecoTraceRule>(Execution, RuleClass, NAME_None, RF_Transient));
    }
    Execution->State = EPinecoTraceExecutionState::Active;
    return Execution;
}

bool UPinecoTraceExecution::EvaluateAt(double TimeSeconds, TArray<FHitResult>& OutResults)
{
    return Advance(TimeSeconds, TimeSeconds, OutResults);
}

bool UPinecoTraceExecution::Advance(
    double StartTimeSeconds, double EndTimeSeconds, TArray<FHitResult>& OutResults)
{
    // A rejected nested call must not erase the batch currently being evaluated.
    if (!IsInGameThread() || bAdvancing)
    {
        return false;
    }
    OutResults.Reset();
    if (State != EPinecoTraceExecutionState::Active)
    {
        return false;
    }

    auto* World = ExecutionWorld.Get();
    auto* Source = SourceActor.Get();
    if (!IsValid(World) || World->bIsTearingDown || !IsValid(Trace))
    {
        return Fail(EPinecoTraceError::InvalidContext);
    }
    if (bRequiresSourceActor && !IsValid(Source))
    {
        return Fail(EPinecoTraceError::SourceUnavailable);
    }
    if (Source && Source->GetWorld() != World)
    {
        return Fail(EPinecoTraceError::InvalidContext);
    }
    if (!FMath::IsFinite(StartTimeSeconds) || !FMath::IsFinite(EndTimeSeconds)
        || StartTimeSeconds > EndTimeSeconds
        || (bHasAdvanced && (EndTimeSeconds < LastEndTimeSeconds
            || (bRequiresContinuousSampling && EndTimeSeconds == LastEndTimeSeconds))))
    {
        return Fail(EPinecoTraceError::InvalidInterval);
    }

    // Permit floating-point roundoff, not a missing or replayed simulation interval.
    constexpr double ContinuityToleranceSeconds = 1.e-6;
    if (bRequiresContinuousSampling && bHasAdvanced
        && !FMath::IsNearlyEqual(StartTimeSeconds, LastEndTimeSeconds, ContinuityToleranceSeconds))
    {
        return Fail(EPinecoTraceError::Discontinuous);
    }

    bAdvancing = true;
    ON_SCOPE_EXIT
    {
        bAdvancing = false;
        if (State != EPinecoTraceExecutionState::Active)
        {
            ReleaseResources();
        }
    };

    const auto bSucceeded = Trace->Evaluate(World, Source, StartTimeSeconds, EndTimeSeconds, OutResults);
    if (State != EPinecoTraceExecutionState::Active)
    {
        OutResults.Reset();
        return false;
    }
    if (!bSucceeded)
    {
        OutResults.Reset();
        return Fail(EPinecoTraceError::EvaluationFailed);
    }

    if (HitPolicy == EPinecoTraceHitPolicy::OncePerActorPerEvaluation)
    {
        // Periodic areas reuse the execution and set allocation, accepting each actor once per pulse.
        SeenHits.Reset();
    }

    int32 WriteIndex = 0;
    const auto CandidateCount = OutResults.Num();
    for (int32 Index = 0; Index < CandidateCount; ++Index)
    {
        auto& Candidate = OutResults[Index];
        if (!Candidate.HasValidHitObjectHandle() || Candidate.Component.IsStale())
        {
            continue;
        }

        const auto* Target = Candidate.HitObjectHandle.GetCachedActor();
        const auto* Component = Candidate.GetComponent();
        if ((Target && Target->GetWorld() != World) || (Component && Component->GetWorld() != World)
            || (bIgnoreSourceActor && Source && Candidate.HitObjectHandle == Source))
        {
            continue;
        }

        FHitKey Key;
        if (HitPolicy != EPinecoTraceHitPolicy::AllResults)
        {
            Key = MakeHitKey(Candidate);
            if (SeenHits.Contains(Key))
            {
                continue;
            }
        }

        bool bAccepted = true;
        for (const auto& Rule : Rules)
        {
            bAccepted = Rule->AcceptHit(World, Source, EndTimeSeconds, Candidate);
            if (State != EPinecoTraceExecutionState::Active)
            {
                OutResults.Reset();
                return false;
            }
            if (!bAccepted)
            {
                break;
            }
        }
        if (!bAccepted)
        {
            continue;
        }

        if (HitPolicy != EPinecoTraceHitPolicy::AllResults)
        {
            SeenHits.Add(Key);
        }
        if (WriteIndex != Index)
        {
            OutResults[WriteIndex] = MoveTemp(Candidate);
        }
        ++WriteIndex;
    }
    OutResults.SetNum(WriteIndex, EAllowShrinking::No);
    LastEndTimeSeconds = EndTimeSeconds;
    bHasAdvanced = true;
    return true;
}

UPinecoTraceExecution::FHitKey UPinecoTraceExecution::MakeHitKey(const FHitResult& Hit) const
{
    FHitKey Key;
    if (const auto* Target = Hit.HitObjectHandle.GetCachedActor())
    {
        Key.Actor = FObjectKey(Target);
    }
    else
    {
        // Keep native instance identity without forcing a dehydrated target to spawn an actor.
        Key.Instance = Hit.HitObjectHandle;
    }

    if (HitPolicy == EPinecoTraceHitPolicy::OncePerBody)
    {
        const auto* Component = Hit.GetComponent();
        Key.Component = FObjectKey(Component);
        Key.BoneName = Hit.BoneName;
        if (Hit.BoneName.IsNone() && !Cast<USkeletalMeshComponent>(Component))
        {
            Key.Item = Hit.Item;
        }
    }
    return Key;
}

void UPinecoTraceExecution::Complete()
{
    if (!IsInGameThread() || State != EPinecoTraceExecutionState::Active)
    {
        return;
    }
    State = EPinecoTraceExecutionState::Completed;
    if (!bAdvancing)
    {
        ReleaseResources();
    }
}

void UPinecoTraceExecution::Cancel()
{
    if (!IsInGameThread() || State != EPinecoTraceExecutionState::Active)
    {
        return;
    }
    State = EPinecoTraceExecutionState::Cancelled;
    if (!bAdvancing)
    {
        ReleaseResources();
    }
}

bool UPinecoTraceExecution::Fail(EPinecoTraceError InError)
{
    Error = InError;
    State = EPinecoTraceExecutionState::Failed;
    if (!bAdvancing)
    {
        ReleaseResources();
    }
    return false;
}

void UPinecoTraceExecution::ReleaseResources()
{
    Trace = nullptr;
    Rules.Reset();
    SeenHits.Reset();
    SourceActor.Reset();
    ExecutionWorld.Reset();
}

UWorld* UPinecoTraceExecution::GetWorld() const
{
    return ExecutionWorld.Get();
}
