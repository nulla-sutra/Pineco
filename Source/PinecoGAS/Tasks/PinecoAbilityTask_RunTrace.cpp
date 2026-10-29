// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "PinecoGAS/Tasks/PinecoAbilityTask_RunTrace.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "PinecoTrace/Execution/PinecoTraceExecution.h"
#include "PinecoTrace/Trace/PinecoTrace.h"

UPinecoAbilityTask_RunTrace* UPinecoAbilityTask_RunTrace::RunTrace(
    UGameplayAbility* OwningAbility, TSubclassOf<UPinecoTrace> InTraceClass,
    const FPinecoTraceExecutionSettings& InSettings, FName TaskInstanceName)
{
    if (!IsInGameThread() || !IsValid(OwningAbility) || !OwningAbility->IsInstantiated())
    {
        return nullptr;
    }

    auto* Task = NewAbilityTask<UPinecoAbilityTask_RunTrace>(OwningAbility, TaskInstanceName);
    Task->TraceClass = InTraceClass;
    Task->Settings = InSettings;
    return Task;
}

void UPinecoAbilityTask_RunTrace::Activate()
{
    const auto* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
    auto* World = GetWorld();
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !IsValid(World) || World->bIsTearingDown)
    {
        Finish(EPinecoTraceTaskResult::Failed, EPinecoTraceError::InvalidContext);
        return;
    }
    if (!HasAuthority())
    {
        // Local delegates do not imply replication. Predicting clients do not produce damage candidates.
        Finish(EPinecoTraceTaskResult::NotAuthority, EPinecoTraceError::None);
        return;
    }
    if (!ActorInfo->AvatarActor.IsValid())
    {
        Finish(EPinecoTraceTaskResult::Failed, EPinecoTraceError::SourceUnavailable);
        return;
    }

    SourceActor = ActorInfo->AvatarActor;
    Execution = UPinecoTraceExecution::Create(this, TraceClass, SourceActor.Get(), Settings);
    if (!Execution)
    {
        Finish(EPinecoTraceTaskResult::Failed, EPinecoTraceError::InvalidConfiguration);
        return;
    }

    bReady = true;
    if (OnReady.IsBound() && ShouldBroadcastAbilityTaskDelegates())
    {
        const TArray<FHitResult> EmptyResults;
        OnReady.Broadcast(EmptyResults, EPinecoTraceTaskResult::Running, EPinecoTraceError::None);
    }
}

bool UPinecoAbilityTask_RunTrace::Advance(double StartTimeSeconds, double EndTimeSeconds)
{
    if (!IsInGameThread() || !bReady || bFinished || bAdvancing)
    {
        return false;
    }
    if (!HasAuthority())
    {
        Finish(EPinecoTraceTaskResult::NotAuthority, EPinecoTraceError::None);
        return false;
    }
    if (!IsValid(Execution))
    {
        Finish(EPinecoTraceTaskResult::Failed, EPinecoTraceError::InvalidContext);
        return false;
    }

    const auto* ActorInfo = Ability->GetCurrentActorInfo();
    if (!SourceActor.IsValid() || ActorInfo->AvatarActor.Get() != SourceActor.Get())
    {
        Finish(EPinecoTraceTaskResult::Failed, EPinecoTraceError::SourceUnavailable);
        return false;
    }

    bAdvancing = true;
    ON_SCOPE_EXIT
    {
        bAdvancing = false;
        if (bFinished)
        {
            Results.Reset();
            Execution = nullptr;
            DispatchFinished();
        }
    };

    const auto bSucceeded = Execution->Advance(StartTimeSeconds, EndTimeSeconds, Results);
    if (bFinished)
    {
        return false;
    }
    if (!bSucceeded)
    {
        const auto State = Execution->GetState();
        if (State == EPinecoTraceExecutionState::Completed)
        {
            Finish(EPinecoTraceTaskResult::Completed, EPinecoTraceError::None);
        }
        else if (State == EPinecoTraceExecutionState::Cancelled)
        {
            Finish(EPinecoTraceTaskResult::Cancelled, EPinecoTraceError::None);
        }
        else
        {
            const auto Failure = Execution->GetError();
            Finish(EPinecoTraceTaskResult::Failed,
                Failure == EPinecoTraceError::None ? EPinecoTraceError::EvaluationFailed : Failure);
        }
        return false;
    }

    if (OnResults.IsBound() && ShouldBroadcastAbilityTaskDelegates())
    {
        OnResults.Broadcast(Results, EPinecoTraceTaskResult::Running, EPinecoTraceError::None);
    }
    return true;
}

void UPinecoAbilityTask_RunTrace::Complete()
{
    Finish(EPinecoTraceTaskResult::Completed, EPinecoTraceError::None);
}

void UPinecoAbilityTask_RunTrace::Cancel()
{
    Finish(EPinecoTraceTaskResult::Cancelled, EPinecoTraceError::None);
}

void UPinecoAbilityTask_RunTrace::ExternalConfirm(bool bEndTask)
{
    if (bEndTask)
    {
        Complete();
    }
}

void UPinecoAbilityTask_RunTrace::ExternalCancel()
{
    Cancel();
}

bool UPinecoAbilityTask_RunTrace::HasAuthority() const
{
    const auto* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
    return IsValid(Ability) && Ability->IsActive() && ActorInfo && ActorInfo->IsNetAuthority();
}

void UPinecoAbilityTask_RunTrace::Finish(EPinecoTraceTaskResult InResult, EPinecoTraceError InError)
{
    if (!IsInGameThread() || bFinished)
    {
        return;
    }
    Result = InResult;
    Error = InError;
    EndTask();
}

void UPinecoAbilityTask_RunTrace::OnDestroy(bool bInOwnerFinished)
{
    if (bFinished)
    {
        return;
    }

    CompletionAbility = Ability;
    bFinishNotificationPending = !bInOwnerFinished && OnFinished.IsBound()
        && ShouldBroadcastAbilityTaskDelegates();
    bFinished = true;
    bReady = false;
    if (bInOwnerFinished)
    {
        Result = EPinecoTraceTaskResult::AbilityEnded;
    }
    else if (Result == EPinecoTraceTaskResult::Running)
    {
        Result = EPinecoTraceTaskResult::Cancelled;
    }

    if (Execution)
    {
        if (Result == EPinecoTraceTaskResult::Completed)
        {
            Execution->Complete();
        }
        else
        {
            Execution->Cancel();
        }
    }

    // A listener may finish the task during Evaluate or OnResults; its borrowed batch stays intact.
    if (!bAdvancing)
    {
        Results.Reset();
        Execution = nullptr;
    }
    TraceClass = nullptr;
    Settings.RuleClasses.Reset();
    SourceActor.Reset();

    // Super clears Ability and marks the task finished before the terminal callback can reenter it.
    Super::OnDestroy(bInOwnerFinished);
    if (!bAdvancing)
    {
        DispatchFinished();
    }
}

void UPinecoAbilityTask_RunTrace::DispatchFinished()
{
    const auto OwningAbility = CompletionAbility;
    CompletionAbility.Reset();
    if (!bFinishNotificationPending)
    {
        return;
    }
    bFinishNotificationPending = false;
    // Defer until OnResults unwinds, including the stock Blueprint node's shared output pins.
    if (OwningAbility.IsValid() && OwningAbility->IsActive())
    {
        const TArray<FHitResult> EmptyResults;
        OnFinished.Broadcast(EmptyResults, Result, Error);
    }
}
